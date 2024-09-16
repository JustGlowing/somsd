/*
  Contents: System specific functions.

  Author: Markus Hagenbuchner

  Comments and questions concerning this program package may be sent
  to 'markus@artificial-neural.net'
 */


/************/
/* Includes */
/************/
#define _BSD_SOURCE
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __CYGWIN__
#include <sys/sysinfo.h>
#endif
#include <sys/statvfs.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>
#include "common.h"
#include "system.h"
#include "utils.h"


/* Begin functions... */

/******************************************************************************
Description: Find the endian of system. FindEndian first checks if a system
             definition of the endian exists and will return this value if
             found. Otherwise, FindEndian attempts to make a good guess.

Return value: Type of endian which can be either LITTLE_ENDIAN, BIG_ENDIAN, 
              PDP_ENDIAN, or UNKNOWN.
******************************************************************************/
int FindEndian()
{
#ifdef __BYTE_ORDER //Rely on system definition if available
  #if __BYTE_ORDER == __LITTLE_ENDIAN
  return LITTLE_ENDIAN;
  #elif __BYTE_ORDER == __BIG_ENDIAN
  return BIG_ENDIAN;
  #elif __BYTE_ORDER == __PDP_ENDIAN
  return PDP_ENDIAN;
  #else
    #error: In function FindEndian():
    #error: System defines a byte-order which is unknown to us!
  #endif
#else
  union { int ival; char bytes[sizeof (int)]; } u;
  
  u.ival = 1;
  if (u.bytes[0])
    return LITTLE_ENDIAN;    //1234
  else if (u.bytes[sizeof (int) - 1])
    return BIG_ENDIAN;       //4321
  else if (u.bytes[2])
    return PDP_ENDIAN;       //3412
  else
    return UNKNOWN;
#endif /* __BYTE_ORDER */
}

/******************************************************************************
Description: Print datatype sorted by size.

Return value: Function returns void.
******************************************************************************/
void ListDataTypes()
{
  int i, count;

  for(i = 1; i < 16; i++){
    count = 0;
    if (sizeof(char) == i) count++;
    if (sizeof(short) == i) count++;
    if (sizeof(int) == i) count++;
    if (sizeof(float) == i) count++;
    if (sizeof(long int) == i) count++;
    if (sizeof(double) == i) count++;
    if (sizeof(long long int) == i) count++;
    if (sizeof(long double) == i) count++;

    if (count){
      fprintf(stderr, "\t%dbit", i*8);
      if (sizeof(char) == i)     fprintf(stderr, " char,");
      if (sizeof(short) == i)    fprintf(stderr, " short,");
      if (sizeof(int) == i)      fprintf(stderr, " int,");
      if (sizeof(float) == i)    fprintf(stderr, " float,");
      if (sizeof(long int) == i) fprintf(stderr, " long int,");
      if (sizeof(double) == i)   fprintf(stderr, " double,");
      if (sizeof(long long int) == i) fprintf(stderr, " long long int,");
      if (sizeof(long double) == i)   fprintf(stderr, " long double");
      fprintf(stderr, "\n");
    }
  }
}

/******************************************************************************
Description: Get the number of clock cycles since last reboot. This works on
             Pentium based systems ONLY!

Return value: Number of clock cycles since last reboot, or 0 if the system is
              not a Pentium PC.
******************************************************************************/
extern __inline__ unsigned long long int rdtsc()
{
#if #cpu(i386)
  unsigned long long int x;
  __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
  return x;
#else
  return 0LL;
#endif
}


/******************************************************************************
Description: Obtain approx. write-speed for the local disc in (KB/sec.)

Return value: Approximate write-speed for the local disc in (KB/sec.)
******************************************************************************/
int GetWriteSpeed()
{
  FILE *ofile;
  char buffer[1509949];  // this is 1.44MB, the size of a floppy
  float speed;
  struct timeval t1, t2;
  time_t tdiff;

  if ((ofile = fopen("delete.me_", "wb")) == NULL)
    return 0;

  fflush(ofile);   //Ensure that the file has actually been created
  gettimeofday(&t1, NULL);  //Starting time
  fwrite(buffer, 1509949, 1, ofile);  //Write buffer
  fflush(ofile);   //Ensure that the data has actually been written to disk
  gettimeofday(&t2, NULL);    //Current time
  fclose(ofile);
  remove("delete.me_");

  tdiff = (t2.tv_sec-t1.tv_sec)*CLOCKS_PER_SEC+t2.tv_usec-t1.tv_usec;
  speed = (float)1509949/((float)tdiff/CLOCKS_PER_SEC);

  return (int)speed/1024;
}

/******************************************************************************
Description: Obtain amount of free disc space on a given drive in bytes. If the
             drive is a null pointer, then the local drive identified by '.' is
             assumed.

Return value: Total number of free disc space (non root space) in kilo-bytes.
******************************************************************************/
unsigned long long FreeDiskSpace(char *drive_path)
{
  struct statvfs buf;
  int flag;

  if (drive_path == NULL)
    flag = statvfs(".", &buf);
  else
    flag = statvfs(drive_path, &buf);

  if (!flag)
    return (unsigned long long)buf.f_bsize * buf.f_bavail;
  else
    return 0LL;
}

/******************************************************************************
Description: GetMachineArch inquires on the type of machine architecture of the
             the local computer.

Return value: Pointer to a terminated string which contains the type of
              system architexture.
******************************************************************************/
char *GetMachineArch()
{
  static struct utsname mybox;

  uname(&mybox);
  return mybox.machine;
}

/******************************************************************************
Description: Return the network node hostname.

Return value: Pointer to a terminated string holding the network node hostname.
******************************************************************************/
char *GetNodeName()
{
  static struct utsname mybox;

  uname(&mybox);
  return mybox.nodename;
}

/******************************************************************************
Description: Return the name of the Operating system.

Return value: Pointer to a static string which contains the name of the
              operating system.
******************************************************************************/
char *GetOSName()
{
  static struct utsname mybox;

  uname(&mybox);
  return mybox.sysname;
}

/******************************************************************************
Description: GetSystemMemorySize tries to find the amount of physical memory
             available in the current system.

Return value: The amount of physical memory available in the system in MB.
******************************************************************************/
unsigned long int GetSystemMemorySize()
{
  unsigned long int npages = 0;
  unsigned long int pagesize = 512;  //default size of a memory page

  /* Obtain size of a page in memory */
#if defined(_SC_PAGESIZE)
  pagesize = (unsigned long int)sysconf(_SC_PAGESIZE);
#elif defined(_PAGESZ)
  pagesize = PAGESZ;
#elif defined(_SYS_SYSINFO_H)
  pagesize = (unsigned long int)getpagesize();
#else
#warning: In GetSystemMemorySize(): Unable to obtain size of memory page. Will use 512 bytes as a default value.
#endif

#if defined(_SC_PHYS_PAGES)
  npages = (unsigned long int)sysconf(_SC_PHYS_PAGES);
#elif defined(_SYS_SYSINFO_H)
  npages = (unsigned long int)get_phys_pages();
#else
#warning: In GetSystemMemorySize(): Unable to obtain number of memory pages. Will try to continue by returning zero.
  return 0L;
#endif

  return npages * pagesize / 1048576;  //Amount of memory in MB
}

/*****************************************************************************
Description: Return number of configured CPUs in system

Return value: Number of CPUs on local system.
*****************************************************************************/
unsigned GetNumCPU()
{
#if defined(_SC_NPROCESSORS_CONF)
  return (unsigned)sysconf(_SC_NPROCESSORS_CONF);
#elif defined(_SC_NPROC_CONF)
  return (unsigned)sysconf(_SC_NPROC_CONF);
#else
  return (unsigned)get_nprocs_conf();
#endif
}

/*****************************************************************************
Description: Print to ofile as much as we know about this software package.

Return value: This function does not return a value.
*****************************************************************************/
void PrintSoftwareInfo(FILE *ofile)
{
  if (ofile == NULL)  /* Print to open streams only */
    return;

  fprintf(ofile, "Program information:\n");
#ifdef PROG_VERSION
  fprintf(ofile, "\tThis is: %s\n", PROG_VERSION);
#endif
  fprintf(ofile, "\tWritten by: Markus Hagenbuchner\n");

  fprintf(ofile, "\tCompiled");
#ifdef __GNUC__  /* GNU-C defines these */
  fprintf(ofile, " using GCC %d.%d ", __GNUC__, __GNUC_MINOR__);
#endif
#ifdef __DATE__  /* if current compile date is defined */
  fprintf(ofile, " on %s", __DATE__);
#endif
#ifdef __TIME__  /* if current compile tile is defined */
  fprintf(ofile, " at %s", __TIME__);
#endif
  fprintf(ofile, "\n");

#ifdef COPT      /* If we know the compiler options */
  fprintf(ofile, "\tCompiler options: %s\n", COPT);
#endif
#ifdef DEBUG     /* If we compiled with DEBUG flag set */
  fprintf(ofile, "\tRUNNING IN DEBUG MODE!!\n");
#endif
#ifdef PVM_SOM   /* Experimental stuff!! */
  fprintf(ofile, "\tRUNNING IN PVM MODE!!\n");
#endif
#ifdef PTHREAD_SOM /* Experimental stuff!! */
  fprintf(ofile, "\tRUNNING IN PARALLEL MODE!!\n");
#endif
}



/*****************************************************************************
Description: Print a rather comprehensive overview of local system properties
             to the stream pointed to by ofile.

Return value: This function does not return a value.
*****************************************************************************/
void PrintSystemInfo(FILE *ofile)
{
  int endian;
  //  float *load;

  if (ofile == NULL)  /* Print to open streams only */
    return;

  fprintf(ofile, "System information:\n");
  fprintf(ofile, "\tThis is: %s\n", GetNodeName());
  fprintf(ofile, "\t%s system running %s.\n", GetMachineArch(), GetOSName());

  if (GetNumCPU() == 1)
    fprintf(ofile, "\tSingle-CPU, %d-bit system.\n", (int)(sizeof(long)*8));
  else if (GetNumCPU() == 2)
    fprintf(ofile, "\tDual-CPU, %d-bit system.\n", (int)(sizeof(long)*8));
  else
    fprintf(ofile, "\t%d-CPU, %d-bit system.\n",GetNumCPU(),(int)sizeof(long)*8);
  //  if ((load = GetSystemLoad()) != NULL)
  //    fprintf(ofile, "\tCurrent system load: %f\n", load[0]);

  endian = FindEndian();

  if (endian == BIG_ENDIAN)
    fprintf(ofile, "\tBIG ENDIAN system\n");
  else if (endian == LITTLE_ENDIAN)
    fprintf(ofile, "\tLITTLE ENDIAN system\n");
  else
    fprintf(ofile, "\tPDP ENDIAN system\n");
#ifdef __SSE__
  fprintf(ofile, "\tSSE enabled.\n");
#endif
#ifdef __SSE2__
  fprintf(ofile, "\tSSE2 enabled.\n");
#endif
#ifdef __SSE3__
  fprintf(ofile, "\tSSE3 enabled.\n");
#endif

  if (GetSystemMemorySize())
    fprintf(ofile, "\t%luMB system memory\n", GetSystemMemorySize());

  fprintf(ofile, "\t%lluMB free disc space\n",  FreeDiskSpace(".")/1048576LL);

#if defined(USE_LONG_DOUBLE_PRECISION)
  fprintf(ofile, "\tUsing long double precision\n");
#elif defined(USE_DOUBLE_PRECISION)
  fprintf(ofile, "\tUsing double precision\n");
#else
  fprintf(ofile, "\tUsing single precision\n");
#endif

  fprintf(ofile, "\n");
}

#ifndef getloadavg  //This is always false since the def of functions is a linker issue
/*****************************************************************************
Description: A gnu replacement of the getloadavg function which attempts to
             obtain the system load through a system query. It is to
             return the number of processes in the system run queue averaged
             over various periods of time. Up to nelem samples are retrieved
             and assigned to successive elements of loadavg[]. The system
             imposes a maximum of 3 samples, representing averages over the
             last 1, 5, and 15 minutes, respectively.

Return value: If the load average was unobtainable, -1 is returned; otherwise,
              the number of samples actually retrieved is returned.
*****************************************************************************/
int getloadavg(double loadavg[], int nelem)
{
  int n;
  char buffer[256], *cptr;
  FILE *ifile;

  if ((ifile = popen("w | head -n 1", "r")) == NULL)
    return -1;

  if (nelem == 0)
    return 0;
  else if (nelem > 3)
    nelem = 3;

  fgets(buffer, 256, ifile);
  if ((cptr = strstr(buffer, "load average:")) == NULL)
    return -1;

  cptr+= strlen("load average:");
  n = 0;
  if (nelem == 1)
    n = sscanf(cptr, "%lf", &loadavg[0]);
  else if (nelem == 2)
    n = sscanf(cptr, "%lf,%lf", &loadavg[0], &loadavg[1]);
  else if (nelem == 3)
    n = sscanf(cptr, "%lf,%lf,%lf", &loadavg[0], &loadavg[1], &loadavg[2]);

  fclose(ifile);
  return n;
}
#endif

/*****************************************************************************
Description: Sleep while system load is high.

Return value: This function does not return a value.
*****************************************************************************/
void SleepOnHiLoad()
{
  double maxload;
  double loadavg;
  int i;
  char msg[] = " Hi system load: Sleeping";

  maxload = 1.2*GetNumCPU();             /* Max load = number of CPUs +20% */
  getloadavg(&loadavg, 1);               /* Get latest system load    */
  while(loadavg > maxload){              /* While system load is high */
    fprintf(stderr, "%s", msg);          /* Show that we are sleeping */
    sleep(60);                           /* Sleep for a while         */
    getloadavg(&loadavg, 1);             /* Get latest system load    */
    for (i = 0; i < strlen(msg); i++)    /* Remove sleep statement    */
      fprintf(stderr, "\b \b"); 
  }
}
