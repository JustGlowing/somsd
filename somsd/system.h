#ifndef SYSTEM_H_DEFINED
#define SYSTEM_H_DEFINED

/* Package information */
#define PROG_VERSION "somsd 1.4.0"   /* Program version */

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1234
#endif
#ifndef BIG_ENDIAN
#define BIG_ENDIAN    4321
#endif
#ifdef __CYGWIN__
#undef PDP_ENDIAN
#endif
#ifndef PDP_ENDIAN
#define PDP_ENDIAN    3412
#endif

int FindEndian();       /* Find endian type of system */
void ListDataTypes();   /* List size of common data types */
extern __inline__ unsigned long long int rdtsc();/* Clock cycles since reboot*/
int GetWriteSpeed();    /* Approximate write speed to local disc */
unsigned long long FreeDiskSpace(char *); /* Get user usable free disk space */
char *GetMachineArch(); /* Get processor type */
char *GetNodeName();    /* Get network node hostname */
char *GetOSName();      /* Get operating system */
unsigned long int GetSystemMemorySize(); /* Get size of physical memory */
unsigned GetNumCPU();   /* Get number of CPUs on local system        */
void PrintSoftwareInfo(FILE *ofile); /* Prints current software info */
void PrintSystemInfo(FILE *ofile);   /* Prints local hardware info   */
void SleepOnHiLoad();   /* Sleep while system load is high           */

#endif
