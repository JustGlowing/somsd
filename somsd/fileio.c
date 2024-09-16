/*
  Contents: Functions related to file input and output.

  Author: Markus Hagenbuchner

  Comments and questions concerning this program package may be sent
  to 'markus@artificial-neural.net'

  ChangeLog:
    24/10/2006
      - Added support to read from gzip compressed files.
 */


/************/
/* Includes */
/************/
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zlib.h>
#include "common.h"
#include "data.h"
#include "fileio.h"
#include "system.h"
#include "utils.h"

/**************************/
/* Locally used constants */
/**************************/
/* Field names found in a datafile */
#define MAXFIELDS   10 /* Maximum number of fields in a datafile        */
#define NODELABEL   1  /* Node label                                    */
#define CHILDSTATE  2  /* Network state for a nodes' offsprings         */
#define PARENTSTATE 3  /* Network state for the parents of a node       */
#define TARGET      4  /* Target vector of a node                       */
//#define GRAPHNO     5  /* Number of graph to which this node belongs   */
#define NODENO      6  /* Number of a node in the graph                 */
#define DEPTH       7  /* Depth of the node in a graph (leafs:depth=0)  */
#define LINKS       8  /* A node's (out-)link definitions               */
#define LABEL       9  /* A symbolic class label for the node           */
#define STATE       10 /* State of neighbors of a node (undirected gph) */


/* Begin functions... */

/******************************************************************************
Description: Open a file stream named by fname, and initialize a FileInfo
             structure. This function has some support for compressed files.
             This is realized by 'abusing' the FILE pointer and setting the
             appropriate ctype flag.

Return value: Pointer to a properly initialized FileInfo structure, or
              NULL if the file could not be opened.
******************************************************************************/
struct FileInfo* OpenFile(char *fname, char *mode)
{
  struct FileInfo* fileinfo;
  FILE *fptr = NULL;
  int ctype = RAW;

  if (fname == NULL || mode == NULL)    /* If no file name given       */
    return NULL;

  if (*mode == 'r')                         /* If to open in read mode,    */
    ctype = GetCompressStatus(fname, mode); /* check if file is compressed */

  switch(ctype){
  case RAW:                                  /* It is an uncompressed file */
    if ((fptr = fopen(fname, mode)) == NULL){/* try to open it             */
      AddError("Unable to open RAW file.");
      return NULL;
    }
    break;
  case GZIP:                                         /* gzip compressed file */
    if ((fptr = (FILE*)gzopen(fname, mode)) == NULL){/* try to open it */
      AddError("Unable to open GZIP file.");
      return NULL;
    }
    break;
  case BZIP:
    AddError("Bzip2 compressed files not yet suppored by som-sd.");
    break;
  default:
    AddError("Unknown file type reported by function GetCompressStatus().");
  }
  /* Store file info in structure */
  fileinfo = (struct FileInfo*)MyCalloc(1, sizeof(struct FileInfo));
  fileinfo->fname = fname;
  fileinfo->fptr = fptr;
  fileinfo->ctype = ctype;

  return fileinfo;
}

/******************************************************************************
Description: Close a file stream and free memory used by the structure pointed
             to by finfo.

Return value: This function does not return a value.
******************************************************************************/
void CloseFile(struct FileInfo *finfo)
{
  if (finfo == NULL)      /* If no data given        */
    return;               /* then return immediately */

  if (finfo->fptr){       /* If file is open and    */
    switch(finfo->ctype){ /* depending on whether or how it was compressed */
    case RAW:            
      fclose(finfo->fptr);/* close RAW file        */
      break;
    case GZIP:
      gzclose((gzFile)finfo->fptr);/* close gzip compressed file */
      break;
    case BZIP:            /* close bzip2 compressed file (if supported) */
      break;
    default:              /* all other file types are unsupported and ignored*/
      break;
    }
  }

  free(finfo);            /* Free up memory used by the structure */
}

/******************************************************************************
Description: Similar to fgetc but can read a compressed stream depending on
             the ctype (compression type) of the file.

Return value: An unsigned char cast to an int containing the next character
              read from stream, or EOF on end of file or error.
******************************************************************************/
inline int zgetc(struct FileInfo *finfo)
{
  switch(finfo->ctype){     /* depending on whether or how it was compressed */
  case RAW:     
    return fgetc(finfo->fptr);     /* read from RAW file, or                 */
  case GZIP:
    return gzgetc((gzFile)finfo->fptr);/* read from gzip compressed file, or */
  case BZIP:               /* read from bzip2 compressed file (if supported) */
    break;
  }
  return 0;
}

/******************************************************************************
Description: Similar to fungetc but can push c back into a compressed stream
             depending on the ctype (compression type) of the file.

Return value: c on success, or EOF on error.
******************************************************************************/
inline int zungetc(int c, struct FileInfo *finfo)
{
  switch(finfo->ctype){     /* depending on whether or how it was compressed */
  case RAW:     
    return ungetc(c, finfo->fptr); /* push back into RAW file, or            */
  case GZIP:
    return gzungetc(c, (gzFile)finfo->fptr);/* back to gzip compressed file, */
  case BZIP:            /* or back into bzip2 compressed file (if supported) */
    break;
  }
  return 0;
}

/******************************************************************************
Description: An auxillary function which reads a value from a possibly
             compressed ASC-II data stream. This function was written since
             there is no fscanf equivalent in the zlib or bzlib library. Thus
             this function is to behaves similarly to fscanf but allows only
             at most one! format character which can be either f or d.

Return value: An unsigned char cast to an int containing the next character
              read from stream, or EOF on end of file or error.
******************************************************************************/
int zscanf(struct FileInfo *finfo, const char *format, void *target)
{
  switch(finfo->ctype){     /* depending on whether or how it was compressed */
  case RAW:     
    return fscanf(finfo->fptr, format, target);      /* Read from RAW stream */
  case GZIP:/* read from gzip compressed file, or     */
    {
    char buffer[256];
    int c, pos = 0;

    for(pos = 0; ; pos++){
      c = gzgetc((gzFile)finfo->fptr);
      if ((c == '-' && pos == 0) || isdigit(c) || c == '.')
	buffer[pos] = c;
      else{
	gzungetc(c, (gzFile)finfo->fptr);/* back into gzip compressed file */
	buffer[pos] = '\0';
	break;
      }
    }
    return sscanf(buffer, format, target);  /* Read from buffer */
    }
  case BZIP:               /* read from bzip2 compressed file (if supported) */
    break;
  }
  return 0;
}

/******************************************************************************
Description: Reads a line of text from a given input file stream. The text is
             stored in a dynamically allocated and zero terminated string. The
             newline character is read but not stored.

Return value: Pointer to the next line of text read from the given file, or
              NULL if no data could be read.
******************************************************************************/
char *ReadLine(struct FileInfo *finfo)
{
  static char *cptr = NULL;
  static unsigned clen = 0;
  unsigned count = 0;
  int cval;

  /* Clear buffer and return NULL pointer if no more data can be read */
  if (finfo == NULL || finfo->fptr == NULL || (cval = zgetc(finfo)) == EOF){
    free(cptr);
    cptr = NULL;
    clen = 0;
    return NULL;
  }

  finfo->lineno++;
  while(1){
    if (clen <= count){          //Dynamically increase buffer size as needed
      clen = clen + 64;
      cptr = MyRealloc(cptr, clen);
    }
    if (cval == '\n' || cval < 0){  // At the end of line or end of file
      cptr[count] = '\0';           // close string and exit the loop
      break;
    }
    cptr[count++] = (char)cval;     // Store character in buffer then
    cval = zgetc(finfo);            // read next character from file
  }

  return cptr;  // Return pointer to buffer
}

/******************************************************************************
Description: Reads a text word from a given input file stream. The text is
             stored in a dynamically allocated and zero terminated string.
             Leading and trailing whitespaces as well as the newline character
             define word boundaries, are read but not stored in the word.

Return value: Pointer to the text word read from the given file, or NULL if no
              data could be read.
******************************************************************************/
char *ReadWord(struct FileInfo *finfo)
{
  char *cptr = NULL;
  unsigned clen = 0;
  unsigned count = 0;
  int cval;

  /* Return NULL pointer if no more data can be read */
  if (finfo == NULL || finfo->fptr==NULL || (cval = zgetc(finfo)) == EOF)
    return NULL;

  while (cval != EOF && isspace(cval)){/* Find the first non-white space char*/
    if (cval == '\n'){         /* End of line reached - no word left to read */
      zungetc(cval, finfo);    /* return the whitespace to the stream */
      return NULL;             /* return NULL */
    }
    cval = zgetc(finfo);       /* Get next character */
  }

  if (cval < 0)  /* End of file reached - no word there to read, return NULL */
    return NULL;

  while(1){
    if (clen <= count){        /* Dynamically increase buffer size as needed */
      clen = clen + 4;         /* Increase by boundary size (32bit) */
      cptr = MyRealloc(cptr, clen);
    }
    if (isspace(cval)){          /* At whitespace or the end of line    */
      zungetc(cval,finfo);       /* return the whitespace to the stream */
      cptr[count] = '\0';        /* close string and exit the loop      */
      break;
    }
    else if (cval < 0){       /* At end of of file */
      cptr[count] = '\0';     /* close string and exit the loop  */
      break;
    }
    cptr[count++] = (char)cval;  /*Store character in buffer then */
    cval = zgetc(finfo);         /*read next character from file  */
  }

  return cptr;  /* Return pointer to word */
}

/****************************************************************************
Description: Like fread but considers the byte order and performs an 
             appropriate convertion if necessary, and is able to read from
             a compressed stream.

Return value: Same as fread (the number of bytes read)
****************************************************************************/
size_t bo_fread(void *ptr, size_t size, size_t nmemb, struct FileInfo *fi)
{
  static int endian = 0;
  char *cptr, cval;
  size_t i,j;
  size_t res;
  FILE *fp = fi->fptr;

  if (!endian)
    endian = FindEndian();

  //Fill the buffer
  switch(fi->ctype){
  case RAW:
    res = fread(ptr, size, nmemb, fp);
    break;
  case GZIP:
    res = gzread((gzFile)fp, ptr, size*nmemb)/size;
    break;
  case BZIP:
    res = 0;
    break;
  default:
    res = 0;
  }

  /* Return immediately if the byteorder in file is the same as on machine,
     or if single bytes are to be read */
  if (endian == fi->byteorder || size == 1)
    return res;

  if (size & 1 || size > 16){ //Odd-sized types or larger 128bit not supported.
    fprintf(stderr, "Internal error: Unsupported datatype in bo_fread\n");
    exit(0);
  }

  //Do the actual byte swapping
  cptr = (char*)ptr;
  if ((endian == BIG_ENDIAN && fi->byteorder == LITTLE_ENDIAN) ||
      (endian == LITTLE_ENDIAN && fi->byteorder == BIG_ENDIAN)){
    //Converting between BIG_ENDIAN and LITTLE_ENDIAN means to mirror bytes
    for (i = 0; i < nmemb; i++){
      for (j = 0; j < size/2; j++){
	cval = cptr[j];
	cptr[j] = cptr[size-j-1];
	cptr[size-j-1] = cval;
      }
      cptr += size;
    }   
  }
  else{  // Either target or source is PDP_ENDIAN. This means we need to swap
         // byte pairs
    if (endian == BIG_ENDIAN || fi->byteorder == BIG_ENDIAN){
      for (i = 0; i < nmemb; i++){
	for (j = 0; j < size; j+=2){
	  cval = cptr[j];
	  cptr[j] = cptr[j+1];
	  cptr[j+1] = cval;
	}
	cptr += size;
      }
    }
    else if (endian == LITTLE_ENDIAN || fi->byteorder == LITTLE_ENDIAN){
      for (i = 0; i < nmemb; i++){
	for (j = 0; j < size; j+=2){
	  cptr[j] = cptr[size-j-2];
	  cptr[j+1] = cptr[size-j-1];
	}
	cptr += size;
      }   
    }
  }

  return res;
}

/******************************************************************************
Description: Reads and ignores all characters in ifile starting from the 
             current file position until the end-of-line character. This
             function is particularly useful to skip over comment lines.

Return value: 
******************************************************************************/
void GotoEndOfLine(struct FileInfo *finfo)
{
  int cval;

  cval = zgetc(finfo);
  while(cval >= 0 && cval != '\n')
    cval = zgetc(finfo);
  if (cval == '\n')
    finfo->lineno++;
}

/******************************************************************************
Description: Returns the next character in ifile which is not a white-space.
             The character is returned to the stream so that the file pointer
             is set to the position of that character.

Return value: The next non-white-space character in ifile, or '\n' if the
              end of a line was reached, or a negative value if the end-of-file
              was reached.
******************************************************************************/
int ReadAhead(struct FileInfo *finfo)
{
  int cval;

  if (finfo == NULL || finfo->fptr == NULL)
    return -1;

  cval = zgetc(finfo);
  while(cval >= 0 && isspace(cval) && cval != '\n')
    cval = zgetc(finfo);

  if (cval >= 0)
    zungetc(cval, finfo); /* Push the char back into the stream */

  return cval;
}

/******************************************************************************
Description: Verify that more data is available in the current line in a
             stream pointed to by ifile. If no more data is available, an 
             error is raised, and 0 returned.

Return value: 1 if more data is available, 0 otherwise (end of line or end of
              file reached).
******************************************************************************/
int SetErrorIfDataUnavailable(struct FileInfo *finfo)
{
  int cval;

  cval = ReadAhead(finfo);

  if (cval == '\n'){
    AddError("Unexpected end of line.");
    return 0;            /* No data available */
  }
  else if (cval < 0){
    AddError("Unexpected end of file.");
    return 0;            /* No data available */
  }
  return 1;
}

/******************************************************************************
Description: Verify that no more data is available in the current line in a
             stream pointed to by ifile. If more data is available, an 
             error is raised, and 0 returned.

Return value: 1 if no more data is available (end of line or end of file
              reached), 0 otherwise (an error is raised).
******************************************************************************/
int SetErrorIfDataAvailable(struct FileInfo *finfo)
{
  int cval;

  cval = ReadAhead(finfo);

  if (cval == '\n'){      /* If end of line reached then move  */
    cval = zgetc(finfo);  /* filepointer to start of next line */
    finfo->lineno++;      /* Keep track of the line number     */
    return 1;
  }
  else if (cval < 0)  /* End of file reached */
    return 1;

  if (cval == '#'){       /* Start of a comment               */
    GotoEndOfLine(finfo); /* Ignore everything until new line */
    return 1;
  }
  AddError("Unexpected trailing data found in file.");
  return 0;               /* Data available */
}

/******************************************************************************
Description: Verify that no more data is available in the given stream pointed
             to by ifile. If more data is available, an error is raised, and 0
             returned.

Return value: 1 if no more data is available (end of file reached),
              0 otherwise (and an error is raised).
******************************************************************************/
int SetErrorIfAnyDataAvailable(struct FileInfo *finfo)
{
  int cval;

  cval = zgetc(finfo);       /* Try to get a data */
  if (cval >= 0){            /* If successful reading a data, then */
    zungetc(cval, finfo);    /* put it back into the stream and    */
    AddError("Unexpected trailing data found in file."); /* raise an error.  */
    return 0;
  }

  return 1;                            /* Data unavailable */
}

/******************************************************************************
Description: Add information about the current file status to the list of
             errors if errors were raised in the past.

Return value: No value is returned.
******************************************************************************/
void AddFileInfoOnError(struct FileInfo *finfo)
{
  if (CheckErrors() > 0){
    char cptr[4096];

    if (finfo->byteorder){
      if (finfo->fname)
	snprintf(cptr, 4096, "Error occured when reading binary file '%s'.", finfo->fname);
      else
	snprintf(cptr, 4096, "Error occured when reading from binary stream.");
    }
    else{
      if (finfo->fname)
	snprintf(cptr, 4096, "Error occured in line %d of file '%s'.", (int)finfo->lineno, finfo->fname);
      else
	snprintf(cptr, 4096, "Error occured when reading line %d.", (int)finfo->lineno);
    }
    AddError(cptr);
  }
}

/******************************************************************************
Description: Read nmemb of binary floats from file pointed to by finfo and
             store in ptr.

Return value: The number of floats read successfully.
******************************************************************************/
size_t ReadBinaryVector(FLOAT *ptr, size_t nmemb, struct FileInfo *finfo)
{
  size_t nval;

  nval = bo_fread(ptr, sizeof(FLOAT), nmemb, finfo); /* Read the vector */
  if (nval != nmemb)                             /* Check if successful */
    AddError("Unexpected end of file.");

  return nval;
}

/******************************************************************************
Description: Write nmemb of binary floats to file pointed to by finfo.

Return value: The number of floats written successfully.
******************************************************************************/
size_t WriteBinaryVector(FLOAT *ptr, size_t nmemb, struct FileInfo *finfo)
{
  size_t nval;

  nval = fwrite(ptr, sizeof(FLOAT), nmemb, finfo->fptr); /* Write the vector */
  if (nval != nmemb)                             /* Check if successful */
    AddError("Unable to write data. File system full?");

  return nval;
}

/******************************************************************************
Description: Read nmemb of float values from file pointed to by finfo and
             store in ptr.

Return value: The number of floats read successfully.
******************************************************************************/
size_t ReadAscIIVector(FLOAT *ptr, size_t nmemb, struct FileInfo *finfo)
{
  size_t i;
  float fval;

  for (i = 0; i < nmemb; i++){
    if (!SetErrorIfDataUnavailable(finfo)) /* Check if data is available*/
      return i;

    if (zscanf(finfo, "%f", &fval) != 1){  /* read next value */
      AddError("File seems corrupted or does not contain expected data.");
      return i;            /* Return the number of values read */
    }
    ptr[i] = (FLOAT)fval;  /* Store value in vector */
  }
  return i;                /* Return the number of values read */
}

/******************************************************************************
Description: Write nmemb of float values to file pointed to by finfo.

Return value: The number of floats written successfully.
******************************************************************************/
size_t WriteAscIIVector(FLOAT *ptr, size_t nmemb, struct FileInfo *finfo)
{
  size_t i;

  for (i = 0; i < nmemb; i++){
    if (ptr[i] == (int)ptr[i]){
      if (!fprintf(finfo->fptr, "%d ", (int)ptr[i])){ /* write next value */
	AddError("Unable to write data. File system full?");
	break;               /* Don't try to write any further   */
      }
    }
    else if (!fprintf(finfo->fptr, "%.9f ", ptr[i])){  /* write next value */
      AddError("Unable to write data. File system full?");
      break;               /* Don't try to write any further   */
    }
  }
  return i;                /* Return the number of values read */
}

/******************************************************************************
Description: Read a single integer from a given file stream in binary format.

Return value: The integer value if successful, 0 otherwise.
******************************************************************************/
int ReadBinaryInt(struct FileInfo *finfo)
{
  int ival;

  if (bo_fread(&ival, sizeof(int), 1, finfo) != 1){ /* Read the integer */
    AddError("Unexpected end of file.");            /* In case of error */
    return 0;
  }
  return ival;
}

/******************************************************************************
Description: Read a single integer from a given file stream in AscII format.

Return value: The integer value, or 0 on error (an error is raised)
******************************************************************************/
int ReadAscIIInt(struct FileInfo *finfo)
{
  int ival;

  if (!SetErrorIfDataUnavailable(finfo)) /* Check that data is available*/
    return 0;

  if (zscanf(finfo, "%d", &ival) != 1){  /* read the integer */
    AddError("File seems corrupted or does not contain expected data.");
    return 0;            /* Return 0 on error */
  }
  return ival;           /* Return the the integer value read */
}

/******************************************************************************
Description: Find if 'line' is in the form "key=value" and return a pointer to
             value.

Return value: Return a pointer to the value of key if the `line` was in the
              specified form, or NULL otherwise.
******************************************************************************/
char *GetFileOption(char *line, char *key)
{
  char *valptr;

  if (line == NULL || key == NULL)
    return NULL;

  valptr = strnspc(line);           //Find first non-white space character
  if (!strncasecmp(valptr, key, strlen(key))){    //Check for keyword
    valptr += strlen(key);          //Jump over keyword
    if (isalnum((int)*valptr))      //Verify that key is a not a substring
      return NULL;
    valptr = strpbrk(valptr, "=:"); //Look for '=' or ':'
    if (valptr != NULL){            //If we found a '=' or ':', then
      for (valptr++; *valptr != '\0' && (*valptr == ':' || *valptr == '='); valptr++);                             //skip all occurences of '=' or ':', and
      valptr = strnspc(valptr);     //find start of value after '=', or ':'
      if (*valptr == '\0')          //No value there?
	valptr = NULL;
    }
    return valptr;
  }
  return NULL;
}

/******************************************************************************
Description: Auxilary function used to read num outlink information from the 
             binary file finfo. The information is stored in an array pointed
             to by links. The outlink section of the file is a sequence of
             positive integers which give the ID number of the child node.

Return value: The maximum value of an ID number found in the link section
             which can be -1 for a missing child.
******************************************************************************/
int ReadLinksBinary(int *links, UNSIGNED num, struct FileInfo *finfo)
{
  int max = -1;
  UNSIGNED i;

  for (i = 0; i < num; i++){
    links[i] = ReadBinaryInt(finfo);
    if (CheckErrors() > 0)
      break;
    if (links[i] > max)
      max = links[i];
  }
  return max;
}

/******************************************************************************
Description: Auxilary function used to read num outlink information from file
             finfo. The information is stored in an array pointed to by links.
             The outlink section of the file is a sequence of positive integers
             which give the ID number of the child node. Missing children are
             identified by placeholders in the file which is any sequence of
             non-white space and non-digit characters. Missing children are
             stored with value -1 in the array.

Return value: The maximum value of an ID number found in the link section, or
              -1 if there were no valid IDs.
******************************************************************************/
int ReadLinksAscII(int *links, UNSIGNED num, struct FileInfo *finfo)
{
  char *cptr, *chead;
  int max = -1;
  UNSIGNED i;

  for (i = 0; i < num; i++){
    chead = ReadWord(finfo);
    cptr = chead;
    if (cptr && isdigit((int)*cptr)){
      links[i] = atoi(cptr);
      if (links[i] > max)
	max = links[i];
    }
    else
      links[i] = -1;
    free(chead);
    if (CheckErrors() > 0)
      break;
  }
  return max;
}

/******************************************************************************
Description: Auxilary function used to ensure that unique items are added to
             the data format array are added.

Return value: 1 if everything went well, 0 if the was an error such as buffer
              overflow or duplicate item found.
******************************************************************************/
int AddUnique(UNSIGNED *array, UNSIGNED item)
{
  UNSIGNED num;

  for (num = 0; num < MAXFIELDS && array[num] != 0 && array[num]!=item; num++);
  if (num < MAXFIELDS && array[num] == 0){
    array[num] = item;
    return 1;
  }
  else{
    AddError("Duplicate item in format string found.");
    return 0;
  }
}

/******************************************************************************
Description: Auxilary function used to find whether a given integer item is in
             an array of integer values. The array is of a given size.

Return value: 0 if the item was not found in the array, or a value greater
              than zero indicating the position (plus one) in which the item
              was found.
******************************************************************************/
int FindInt(UNSIGNED *array, UNSIGNED size, int item)
{
  UNSIGNED num;

  for (num = 0; num < size; num++){
    if (array[num] == item)
      return num+1;
  }
  return 0;
}

/******************************************************************************
Description: Initialize nmemb elements of the float vector pointed to by fptr
             with value.

Return value: This function does not return a value.
******************************************************************************/
void InitFloatVector(FLOAT *fptr, size_t nmemb, FLOAT value)
{
  UNSIGNED i;

  for (i = 0; i < nmemb; i++)
    fptr[i] = value;
}

/******************************************************************************
Description: An auxilary function used to extract file format information from
             a given text line pointed to by cptr. The text line is expected to
             contain a list of comma separated keywords which indicate the
             format of the file.

Return value: The number of keywords found in the given line.
******************************************************************************/
int GetDataFormat(char *cptr, UNSIGNED *dformat)
{
  UNSIGNED num = 0;

  if (cptr == NULL)                    /* No data given, return 0 */
    return 0;

  memset(dformat, 0, sizeof(unsigned)*MAXFIELDS); /* Reset the format string */

  while (cptr != NULL && CheckErrors() == 0){
    num++;
    if (!strncasecmp(cptr, "nodelabel", 9)){            /*Check for keyword  */
      cptr = strnstr(strpbrk(cptr+9, " ,:;+"), " ,:;+");/*Jump to next field */
      AddUnique(dformat, NODELABEL);          /* Record occurence of keyword */
    }
    else if (!strncasecmp(cptr, "childstate", 10)){      /*Check for keyword */
      cptr = strnstr(strpbrk(cptr+10, " ,:;+"), " ,:;+");/*Jump to next field*/
      AddUnique(dformat, CHILDSTATE);         /* Record occurence of keyword */
    }
    else if (!strncasecmp(cptr, "parentstate", 11)){     /*Check for keyword */
      cptr = strnstr(strpbrk(cptr+11, " ,:;+"), " ,:;+");/*Jump to next field*/
      AddUnique(dformat, PARENTSTATE);        /* Record occurence of keyword */
    }
    else if (!strncasecmp(cptr, "target", 6)){          /*Check for keyword  */
      cptr = strnstr(strpbrk(cptr+6, " ,:;+"), " ,:;+");/*Jump to next field */
      AddUnique(dformat, TARGET);             /* Record occurence of keyword */
    }
    else if (!strncasecmp(cptr, "noden", 5)){           /*Check for keyword  */
      cptr = strnstr(strpbrk(cptr+5, " ,:;+"), " ,:;+");/*Jump to next field */
      AddUnique(dformat, NODENO);             /* Record occurence of keyword */
    }
    else if (!strncasecmp(cptr, "depth", 5)){           /*Check for keyword  */
      cptr = strnstr(strpbrk(cptr+5, " ,:;+"), " ,:;+");/*Jump to next field */
      AddUnique(dformat, DEPTH);              /* Record occurence of keyword */
    }
    else if (!strncasecmp(cptr, "links", 5)){           /*Check for keyword  */
      cptr = strnstr(strpbrk(cptr+5, " ,:;+"), " ,:;+");/*Jump to next field */
      AddUnique(dformat, LINKS);              /* Record occurence of keyword */
    }
    else if (!strncasecmp(cptr, "label", 5)){           /*Check for keyword  */
      cptr = strnstr(strpbrk(cptr+5, " ,:;+"), " ,:;+");/*Jump to next field */
      AddUnique(dformat, LABEL);              /* Record occurence of keyword */
    }
    else if (!strncasecmp(cptr, "state", 5)){           /*Check for keyword  */
      cptr = strnstr(strpbrk(cptr+5, " ,:;+"), " ,:;+");/*Jump to next field */
      AddUnique(dformat, STATE);              /* Record occurence of keyword */
    }
    else
      AddError("Unrecognized item in format string found.");
  }
  return num;
}

/******************************************************************************
Description: A helper function used to read a symbolic label from a binary
             stream. 

Return value: Pointer to the symbolic label or NULL if the label coult not be
              read successfully.
******************************************************************************/
char *ReadBinaryLabel(struct FileInfo *finfo)
{
  UNSIGNED clen = 0;  /* Length of the label  */
  char *cptr = NULL;  /* Pointer to the label */

  if (bo_fread(&clen, sizeof(UNSIGNED), 1, finfo) != 1)/*Read length of label*/
    AddError("Unexpected end of file.");
  else if (clen != 0){               /* Read label if length is greater zero */
    cptr = MyCalloc(clen+1, sizeof(char));      /* Allocate memory for label */
    if (fread(cptr, sizeof(char), clen, finfo->fptr) != clen){ /* read label */
      AddError("Unexpected end of file.");  /* if we couldn't read the label */
      free(cptr);                   /* free allocated memory and return NULL */
      cptr = NULL;
    }
  }
  
  return cptr;
}

/******************************************************************************
Description: Reads codebook entries from file finfo and stores them in the
             given map structure.

Return value: The number of errors that occured while reading the codebooks.
******************************************************************************/
int ReadCodes(struct FileInfo *finfo, struct Map *map)
{
  UNSIGNED x, y;
  FLOAT *fptr;
  char *label;
  size_t(*ReadVector)(FLOAT *, size_t, struct FileInfo *);
  char*(*ReadLabel)(struct FileInfo *);
  int(*CheckForTrailingData)(struct FileInfo *);

  map->codes = (struct Codebook*)MyCalloc(map->xdim*map->ydim, sizeof(struct Codebook));

  if (finfo->byteorder != 0){
    ReadVector = ReadBinaryVector;
    ReadLabel = ReadBinaryLabel;
    CheckForTrailingData = SetErrorIfAnyDataAvailable;
  }
  else{
    ReadVector = ReadAscIIVector;
    ReadLabel = ReadWord;
    CheckForTrailingData = SetErrorIfDataAvailable;
  }

  for (y = 0; y < map->ydim; y++){
    for (x = 0; x < map->xdim; x++){
      fptr = (FLOAT*)MyMalloc(map->dim * sizeof(FLOAT)); 
      ReadVector(fptr, map->dim, finfo);
      map->codes[y*map->xdim+x].points = fptr;
      map->codes[y*map->xdim+x].x = x;
      map->codes[y*map->xdim+x].y = y;
      label = ReadLabel(finfo);
      if (label != NULL){
	map->codes[y*map->xdim+x].label = AddLabel(label);
	free(label);
      }
      if (finfo->byteorder == 0)      /* In ASC-II mode...       */
	CheckForTrailingData(finfo);  /* check for trailing data */
      if (CheckErrors() > 0)
	return CheckErrors();
    }
  }
  CheckForTrailingData(finfo);  /* Check for unexpected trailing data */

  return 0;
}


/******************************************************************************
Description: Read a datafile header from file finfo. A header can occur at the
             start of the file or between any two graphs. Reading starts from
             the line pointed to by cptr, header information is stored in a
             prototype called prime, file format information is stored in
             dformat.

Return value: Pointer to the first line of text which follows the header, or
              NULL if there is no more data in the file.
******************************************************************************/
char *ReadDataHeader(char *cptr, UNSIGNED *dformat, struct Graph *prime, struct FileInfo *finfo)
{
  UNSIGNED numrecognized;

  while(cptr != NULL){
    numrecognized = 0;
    cptr = strnspc(cptr);
    numrecognized+=satou(GetFileOption(cptr,"dim_target"),(uint*)&prime->tdim);
    numrecognized+=satou(GetFileOption(cptr,"indegree"), (uint*)&prime->FanIn);
    numrecognized+=satou(GetFileOption(cptr,"outdegree"),(uint*)&prime->FanOut);
    numrecognized+=satou(GetFileOption(cptr,"dim_label"), (uint*)&prime->ldim);
    numrecognized+=satou(GetFileOption(cptr, "byteorder"),&finfo->byteorder);
    numrecognized += GetDataFormat(GetFileOption(cptr, "format"), dformat);
    if (!strncmp(cptr, "graph", 5)) /* Graph data follows */
      break;

    if (numrecognized == 0 && *cptr != '\0' && *cptr != '#'){
      AddError("Unrecognized keyword found in header.");
      AddFileInfoOnError(finfo);  /* Add file status info to error message */
      break;
    }
    cptr = ReadLine(finfo);
  }
  /* Verify that header contained useful information */
  if (finfo && finfo->byteorder > 0 && finfo->byteorder != LITTLE_ENDIAN && finfo->byteorder != BIG_ENDIAN && finfo->byteorder != PDP_ENDIAN)
    AddError("Invalid byteorder specified in file!");

  if (prime->FanIn + prime->FanOut + prime->tdim + prime->ldim == 0)
    AddError("Overall dimension of data is zero!");

  if (prime->ldim > 0 && !FindInt(dformat, MAXFIELDS, NODELABEL))
    AddError("Dimension of node label is non-zero but no labels are given");
  if (prime->tdim > 0 && !FindInt(dformat, MAXFIELDS, TARGET))
    AddError("Dimension of target value is non-zero but no targets are given");
  if (prime->FanIn > 0 && !FindInt(dformat, MAXFIELDS, LINKS))
    AddError("FanIn must be zero when undirected links are specified");
  if (FindInt(dformat, MAXFIELDS, STATE) && (FindInt(dformat, MAXFIELDS, PARENTSTATE) || FindInt(dformat, MAXFIELDS, CHILDSTATE)))
    AddError("Both directed and undirected links are specified. This is currently not supported.");
  return cptr;
}

/******************************************************************************
Description: Connect the nodes stored in gptr according to information provided
             by links.

Return value: This function does not return a value;
******************************************************************************/
void LinkNodes(struct Graph *gptr)
{
  UNSIGNED i, j;
  struct Node *node, *child;
  static int nerror = 0;
  int *links;            /* Array holding outlink information */

  if (gptr->FanOut == 0) /* Graph has no offsprings (e.g. single node only) */
    return;

  for (i = 0; i < gptr->numnodes; i++){
    node = gptr->nodes[i];
    links = (int*)node->children; /* Rectify a dirty hack in ReadNodes() */
    node->children=(struct Node**)MyCalloc(gptr->FanOut, sizeof(struct Node*));
    for (j = 0; j < gptr->FanOut; j++){
      if (links[j] >= 0 && links[j] < gptr->numnodes){
	node->children[j] = gptr->nodes[links[j]]; /* Set link to children */

	child = gptr->nodes[links[j]];  /* Current node is parent of its     */
	child->numparents += 1;/* children, thus add node to list of parents */
	child->parents = MyRealloc(child->parents, child->numparents * sizeof(struct Node*));
	child->parents[child->numparents-1] = node;
      }
      else if (links[j] > 0){ /* Ignore links to a non-existing nodes */
	char msg[256];

	if (nerror < 10){
	  snprintf(msg, 256, "Warning: Ignoring link from node %d of graph '%s' to non-existing node %d.", node->nnum, gptr->gname, links[j]);
	  AddMessage(msg);
	}
	else if (nerror == 10)
	  AddMessage("These warnings occur more than 10 times...truncating.");

	nerror++;
      }
    }
    free(links);
  }
}

/******************************************************************************
Description: Read the nodes of a graph from file finfo. Store the nodes which
             are expected to be available in the format given by dformat in
             the graph structure provided by gptr.

Return value: 0 if no error, otherwise the number of errors occured while
              reading the nodes is returned.
******************************************************************************/
int ReadNodes(struct Graph *gptr, UNSIGNED *dformat, struct FileInfo *finfo)
{
  int maxid, cval;          /* cval is a read-ahead character */
  UNSIGNED i;
  UNSIGNED nodeno = 0;      /* Number of nodes read from file */
  UNSIGNED numnodes = 0;    /* Number of nodes for which we allocated memory */
  UNSIGNED maxnodes = MAX_UNSIGNED;/* Max number of nodes for current graph  */
  UNSIGNED dimension;          /* Total data dimension of the nodes     */
  UNSIGNED coff = 0, poff = 0, toff = 0;/*Offset values for vector components*/
  struct Node **nodes = NULL;  /* The array of nodes                    */
  struct Node *node;           /* Pointer to current node               */
  int  *links;                 /* Array holding outlink information     */
  char *label;
  size_t(*ReadVector)(FLOAT *, size_t, struct FileInfo *);
  int(*ReadInt)(struct FileInfo *);
  int(*ReadLinks)(int *links, UNSIGNED size, struct FileInfo *);
  char*(*ReadLabel)(struct FileInfo *);

  /* Set pointers to the right functions depending on whether the data is
     in binary or in ASC-II format */
  if (finfo->byteorder != 0){          /* If in binary format */
    ReadVector = ReadBinaryVector;
    ReadInt    = ReadBinaryInt;
    ReadLinks  = ReadLinksBinary;
    ReadLabel  = ReadBinaryLabel;
    if (bo_fread(&numnodes, sizeof(UNSIGNED), 1, finfo) != 1){
      AddError("Unexpected end of file.");
      return 1;
    }
    nodes = (struct Node **)MyCalloc(numnodes, sizeof(struct Node *));
    maxnodes = numnodes;/* read no more nodes than numnodes in binary mode */
  }
  else{  /* otherwise data are assumed to be in ASC-II format */
    ReadVector = ReadAscIIVector;
    ReadInt    = ReadAscIIInt;
    ReadLinks  = ReadLinksAscII;
    ReadLabel  = ReadWord;
    nodes = (struct Node **)MyCalloc(127, sizeof(struct Node *));
    numnodes = 127;
  }

  /* Compute total dimension of data label, and the offset values of its
     elements. */
  dimension = 0;
  dimension += gptr->ldim;
  coff = dimension;    /* offset value of child state vector */
  dimension += 2 * gptr->FanOut;
  poff = dimension;   /* offset value of parent state vector */
  dimension += 2 * gptr->FanIn;
  toff = dimension;         /* offset value of target vector */
  dimension += gptr->tdim;

  /* Loop to read one node at a time */
  while(maxnodes > nodeno && CheckErrors() == 0){

    if (finfo->byteorder == 0){ /* In ASC-II mode only, */
      cval = zgetc(finfo);      /* Find start of data */
      while (cval != EOF && (isspace(cval) || cval == '#')){
	if (cval == '\n')
	  finfo->lineno++;      /* Keep track of line numbers */
	else if (cval == '#')
	  GotoEndOfLine(finfo);
	cval = zgetc(finfo);
      }

      if (cval == EOF)          /* There is no more data */
	break;

      zungetc(cval, finfo);   /* Put read-ahead character back into stream  */
      if (isalpha(cval))      /* check if the read-ahead was alphabetic, as */
	break;                /* this indicates start of a new header       */
    }                         /* ...end ASC-II mode only */

    /* Allocate memory for new node and its data vector */
    node = (struct Node *)MyCalloc(1, sizeof(struct Node));
    node->points = (FLOAT*)MyCalloc(dimension, sizeof(FLOAT));

    /* Read node in given file format */
    for (i = 0; dformat[i] != 0 && CheckErrors() == 0; i++){
      if (dformat[i] == NODELABEL)
	ReadVector(node->points, gptr->ldim, finfo);      /* Read node label */
      else if (dformat[i] == CHILDSTATE)
	ReadVector(&node->points[coff], 2 * gptr->FanOut, finfo); /* c-state */
      else if (dformat[i] == PARENTSTATE)
	ReadVector(&node->points[poff], 2 * gptr->FanIn, finfo);  /* p-state */
      else if (dformat[i] == TARGET)
	ReadVector(&node->points[toff], gptr->tdim, finfo); /* target vector */
      else if (dformat[i] == NODENO)
	node->nnum = ReadInt(finfo);  /* Read node number  */
      else if (dformat[i] == DEPTH)
	node->depth = ReadInt(finfo); /* Read node depth   */
      else if (dformat[i] == LINKS){  /* To convert outlinks to pointers use */
	links = MyCalloc(gptr->FanOut, sizeof(int));/* this dirty hack which */
	node->children = (struct Node**)links; /* is set right in LinkNodes()*/
	ReadLinks(links, gptr->FanOut, finfo);
      }
      else if (dformat[i] == LABEL){
	label = ReadLabel(finfo);
	node->label = AddLabel(label);  /* Read the symbolic data label */
	free(label);
      }
    }
    if (!FindInt(dformat, MAXFIELDS, NODENO)) /* If node number not available*/
      node->nnum = nodeno;            /* set a node's default logical number */

    if (numnodes <= nodeno){
      numnodes += 128;
      nodes = (struct Node**)MyRealloc(nodes, numnodes*sizeof(struct Node*));
      memset(&nodes[numnodes-128], 0, 128*sizeof(struct Node*));
    }

    nodes[nodeno] = node;             /* Add node to array */
    nodeno++;                         /* Increase node counter */

    if (CheckErrors() == 0 && finfo->byteorder == 0) /* In ASC-II mode       */
      SetErrorIfDataAvailable(finfo); /*No further data expected in this line*/

    if (CheckErrors() > 0){
      finfo->lineno++;           /* Adjust line number as it is off by one */
      AddFileInfoOnError(finfo); /* Add file status info on error          */
    }
  }

  maxid = 0;
  for (i = 0; i < nodeno; i++){
    if (nodes[i]->nnum > maxid)
      maxid = nodes[i]->nnum;                /* Find greatest node ID */
  }
  gptr->nodes = (struct Node **)MyCalloc(maxid+1, sizeof(struct Node *));
  gptr->numnodes = maxid+1;
  for (i = 0; i < nodeno; i++)
    gptr->nodes[nodes[i]->nnum] = nodes[i];  /* Assign nodes to graph */
  free(nodes);
  nodes = gptr->nodes;
  gptr->dimension = dimension;

  /* */
  /* What follows here are post-analysis and post-initializations */
  /* */
  if (CheckErrors() > 0)
    return CheckErrors();
  else if (maxid+1 != nodeno){/*Highest node-id must equal number of nodes*/
    AddError("Inconsistency with node numbers.");
    AddFileInfoOnError(finfo); /* Add file status info on error           */
  }
  else{ /* No errors */

    if (FindInt(dformat, MAXFIELDS, LINKS))
      LinkNodes(gptr);    /* Set internal links to parents and offsprings */

    /* Initialize child states if they were not specified in the file */
    if (gptr->FanOut > 0 && !FindInt(dformat, MAXFIELDS, CHILDSTATE)){
      for (i = 0; i < maxid+1; i++)
	InitFloatVector(&nodes[i]->points[coff], 2*gptr->FanOut, -1.0);
    }

    /* Initialize parent states if they were not specified in the file */
    if (gptr->FanIn > 0 && !FindInt(dformat, MAXFIELDS, PARENTSTATE)){
      for (i = 0; i < maxid+1; i++)
	InitFloatVector(&nodes[i]->points[poff], 2*gptr->FanIn, -1.0);
    }
  }
  if (CheckMessages()){
    fputc('\n', stderr);
    PrintMessages();
  }
  return CheckErrors();
}

/******************************************************************************
Description: Read a single graph structure from file pointed to by finfo. The
             graph is stored in the structure provided by gptr, it's nodes are
             expected to be available on file in the format defined by dformat.
             The pointer cptr points to the line which indicates the start of
             the graph structure (it starts with the keyword 'graph') and may
             contain a name of the graph.

Return value: Pointer to the first line of text in the file after the graph
              structure which either points to the start of a new graph, the
              start of a new header, or NULL if there are no further graphs.
******************************************************************************/
char *ReadGraph(char *cptr, UNSIGNED *dformat, struct Graph *gptr, struct FileInfo *finfo)
{
  if (!((cptr = strchr(cptr, ':')) == NULL || (cptr = strnspc(cptr+1))== '\0'))
    gptr->gname = strdup(cptr);     /* Get the name of the graph */

  ReadNodes(gptr, dformat, finfo);  /* Now read the graph's nodes */

  cptr = ReadLine(finfo);           /* read next line of data  */
  return cptr;
}

/******************************************************************************
Description: Read graph definitions from a file named fname, and return a
             pointer to a linked list of graphs (read from the given file).

Return value: Pointer to a linked list of graphs, or NULL if there were no
              graphs read.
******************************************************************************/
struct Graph* LoadData(char *fname)
{
  UNSIGNED dformat[MAXFIELDS+1] = {NODELABEL,CHILDSTATE,LINKS,LABEL,0};
  UNSIGNED gnum = 0;              /* Provides a unique number to each graph */
  UNSIGNED numnodes = 0;          /* Counts the total number of nodes       */
  char *cptr;
  struct FileInfo *finfo;         /* Structure to hold file status info     */
  struct Graph prime;             /* Used as a prototype for all graphs     */
  struct Graph *head = NULL, *prev = NULL, *gptr; /* Handle the graph-list  */

  fprint(stderr, "Reading data.......");       /* Print what is being done */
  if ((finfo = OpenFile(fname, "rb")) == NULL) /* Try to open data stream  */
    AddError("No file name given.");

  if (CheckErrors()){                          /* Any errors so far?       */
    fprintf(stderr, "%55s\n", "[FAILED]");
    return NULL;
  }

  memset(&prime, 0, sizeof(struct Graph)); /* Initialize primal graph */
  InitProgressMeter(-1);             /* Initialize the progress meter */

  /* Read the header */
  cptr = ReadLine(finfo);                 /* Read first line of data  */
  cptr = ReadDataHeader(cptr, dformat, &prime, finfo);
  if (cptr == NULL)         /* Obligatory keyword "graph" not found   */
    AddError("This doesn't seem to be a valid data file.");

  while (cptr != NULL && CheckErrors() == 0){
    gptr = MyMalloc(sizeof(struct Graph));
    memcpy(gptr, &prime, sizeof(struct Graph));
    gptr->gnum = gnum++;            /* Give this graph a logical number */
    cptr = ReadGraph(cptr, dformat, gptr, finfo); /* Read graph */
    if (prev != NULL)               /* Attach to list of graph  */
      prev->next = gptr;
    else
      head = gptr;
    prev = gptr;

    numnodes += gptr->numnodes;
    PrintProgress(gnum);  /* Print progress */
    if (CheckErrors() == 0)
      cptr = ReadDataHeader(cptr, dformat, &prime, finfo);
  }
  CloseFile(finfo);       /* Close data stream */
  if (CheckErrors() == 0)
    SetNodeDepth(head);   /* Ensure that depth value of nodes is initialized */

  StopProgressMeter();    /* Stop the progress meter */
  if (!CheckErrors())     /* If no errors...   */
    fprintf(stderr, "%d nodes%*s\n", (int)numnodes, 48-(int)(log10(numnodes)), "[OK]");
  else
    fprintf(stderr, "%55s\n", "[FAILED]");

  return head;
}

/******************************************************************************
Description: 

Return value: 
******************************************************************************/
void SaveData(FILE *ofile, struct Graph *gptr)
{
  UNSIGNED i, n;
  UNSIGNED ldim, tdim, FanIn, FanOut;
  UNSIGNED soff, coff, poff;
  struct Node *node;

  if (gptr == NULL)
    return;

  ldim = INT_MAX;  /* Initialize graph properties with illegal values to  */
  tdim = INT_MAX;  /* enforce the writing of a data header for the first  */
  FanIn = INT_MAX; /* graph. Any successife graph will have an own header */
  FanOut = INT_MAX;/* if a property differs from an earlier graph.        */
  fprintf(ofile, "format=nodenumber,nodelabel,target,links,label\n");
  for(; gptr != NULL; gptr = gptr->next){

    /* Compute max indegree of graph */
    for (n = 0; n < gptr->numnodes; n++){
      if (gptr->FanIn < gptr->nodes[n]->numparents)
	gptr->FanIn = gptr->nodes[n]->numparents;
    }

    if (ldim != gptr->ldim){
      ldim = gptr->ldim;
      fprintf(ofile, "dim_label=%d\n", ldim);
    }
    if (tdim != gptr->tdim){
      tdim = gptr->tdim;
      fprintf(ofile, "dim_target=%d\n", tdim);
    }
    if (FanOut != gptr->FanOut){
      FanOut = gptr->FanOut;
      fprintf(ofile, "outdegree=%d\n", FanOut);
    }
    if (gptr->gname != NULL)
      fprintf(ofile, "graph:%s\n", gptr->gname);
    else
      fprintf(ofile, "graph\n");

    for (n = 0; n < gptr->numnodes; n++){
      node = gptr->nodes[n];
      soff = gptr->ldim + 2*gptr->FanOut;
      poff = soff + 2*node->numparents;
      coff = poff + gptr->tdim;

      fprintf(ofile, "%u ", node->nnum);  /* Print node number */
      for (i = 0; i < gptr->ldim; i++){
	if (node->points[i] == (int)node->points[i])
	  fprintf(ofile, "%3d ", (int)node->points[i]);/* Print node label   */
	else
	  fprintf(ofile, "%f ", node->points[i]); /* Print node label   */
      }
      for (i = poff; i < coff; i++)
	fprintf(ofile, "%f ", node->points[i]); /* Print target vector */

      for (i = 0; i < gptr->FanOut; i++){  /* Print links */
	if (node->children[i] == NULL)
	  fprintf(ofile, "- ");
	else
	  fprintf(ofile, "%u ", node->children[i]->nnum);
      }

      if (GetLabel(node->label) != NULL) /* Print node label */
	fprintf(ofile, "%s\n", GetLabel(node->label));
      else
	fprintf(ofile, "\n");
    }
  }

  /*
  UNSIGNED n, i;
  struct Graph *gptr;
  UNSIGNED ldim, tdim, FanIn, FanOut;

  printf("format=nodenumber,nodelabel,childstate,parentstate,target,depth,links,label\n");
  ldim = INT_MAX;  
  tdim = INT_MAX; 
  FanIn = INT_MAX;
  FanOut = INT_MAX;
  for(gptr = graph; gptr != NULL; gptr = gptr->next){
    if (ldim != gptr->ldim){
      ldim = gptr->ldim;
      printf("dim_label=%d\n", ldim);
    }
    if (tdim != gptr->tdim){
      tdim = gptr->tdim;
      printf("dim_target=%d\n", tdim);
    }
    if (FanIn != gptr->FanIn){
      FanIn = gptr->FanIn;
      printf("indegree=%d\n", FanIn);
    }
    if (FanOut != gptr->FanOut){
      FanOut = gptr->FanOut;
      printf("outdegree=%d\n", FanOut);
    }
    if (gptr->gname != NULL)
      fprintf(ofile, "graph:%s\n", gptr->gname);
    else
      fprintf(ofile, "graph:\n");
    for (n = 0; n < gptr->numnodes; n++){
      fprintf(ofile, "%d ", gptr->nodes[n]->nnum);
      for (i = 0; i < gptr->dimension; i++){
	PrintFloat(ofile, gptr->nodes[n]->points[i]);
	fprintf(ofile, " ");
	  //	fprintf(ofile, "%f ", gptr->nodes[n]->points[i]);
      }
      fprintf(ofile, "%d", (int)gptr->nodes[n]->depth);
      
      for (i = 0; i < gptr->FanOut; i++){
	if (gptr->nodes[n]->children[i] == NULL)
	  fprintf(ofile, "- ");
	else
	  fprintf(ofile, "%d ", (int)((graph->nodes[n]->children[i] - graph->nodes[0])/sizeof(struct Node*)));
      }
      for (i = 0; i < gptr->FanIn; i++){
	if (i < gptr->nodes[n]->numparents)     
	  printf("%3d %3d ", gptr->nodes[n]->parents[i]->x, gptr->nodes[n]->parents[i]->y);
	else
	  fprintf(ofile, " -1  -1 ");
      }

      fprintf(ofile, "\n");
    }
  }
*/
  sync();  /* Ensure data is actually written to disk */
}

/******************************************************************************
Description: Read the Self-Organizing Map data from a given file and store
             the data in the structure pointed to by map.

Return value: 0 if no errors, or a value greater than zero if there were errors
******************************************************************************/
int LoadMap(struct Parameters *params)
{
  char *cptr;  /* Pointer to text we read from the file */
  char *carg;  /* Pointer used to point at the argument of an parameter */
  UNSIGNED num;
  struct Map *map;
  struct FileInfo *finfo = NULL; /* File structure */

  if (params == NULL)
    return 0;

  map = &params->map;
  fprintf(stderr, "Reading codebook entries....");/* Print What is being done*/
  memset(map, 0, sizeof(struct Map));    /* Initialize Map structure         */
  if (params->inetfile == NULL)          /* Ensure that there is a file name */
    AddError("No file name for map-file given.");
  else if ((finfo = OpenFile(params->inetfile, "rb")) == NULL) /* open stream*/
    AddError("Unable to open file for reading.");

  if (CheckErrors()){                    /* Any errors so far?       */
    fprintf(stderr, "%46s\n", "[FAILED]");
    return CheckErrors();
  }

  /* Read the header */
  cptr = ReadLine(finfo);              /* Read first line of data  */
  while(cptr != NULL){
    cptr = strnspc(cptr);
    num = 0;
    num += satou(GetFileOption(cptr, "iteration"),(uint*)&map->iter);
    num += satou(GetFileOption(cptr, "dim"), (uint*)&map->dim);
    num += satou(GetFileOption(cptr, "xdim"), (uint*)&map->xdim);
    num += satou(GetFileOption(cptr, "ydim"), (uint*)&map->ydim);
    num += satou(GetFileOption(cptr, "byteorder"),&finfo->byteorder);
    num += GetNeighborhoodID(GetFileOption(cptr, "neighborhood"), &map->neighborhood);
    num += GetTopologyID(GetFileOption(cptr, "topology"), &map->topology);
    if (!strncasecmp(cptr, "Train", 5)){/* If info on train params available */
      cptr += 5;  /* Jump over key "Train" */
      num += satof(GetFileOption(cptr, "mu1"), &params->mu1);
      num += satof(GetFileOption(cptr, "mu2"), &params->mu2);
      num += satof(GetFileOption(cptr, "mu3"), &params->mu3);
      num += satof(GetFileOption(cptr, "mu4"), &params->mu4);
      num += satof(GetFileOption(cptr, "alpha"), &params->alpha);
      num += satof(GetFileOption(cptr, "beta"), &params->beta);
      num += satou(GetFileOption(cptr, "iter"), (uint*)&params->rlen);
      num += satou(GetFileOption(cptr, "radius"), (uint*)&params->radius);
      num += satos(GetFileOption(cptr, "data"), &params->datafile);
      num += satos(GetFileOption(cptr, "valid"), &params->validfile);
      /* Now we look at bit-field values */
      if ((carg = GetFileOption(cptr, "supervised")) != NULL &&isdigit((int)*carg)){
	params->super = atoi(carg);
	num++;
      }
      if ((carg = GetFileOption(cptr, "alphatype")) != NULL && isdigit((int)*carg)){
	params->alphatype = atoi(carg);
	num++;
      }
      if ((carg = GetFileOption(cptr, "kernel")) != NULL && isdigit((int)*carg)){
	params->kernel = atoi(carg);
	num++;
      }
      if ((carg = GetFileOption(cptr, "batchmode")) != NULL && *carg != '0' && strncasecmp("no", carg, 2)){
	params->batch = 1;
	num++;
      }
      if ((carg = GetFileOption(cptr, "momentum")) != NULL && *carg != '0' && strncasecmp("no", carg, 2)){
	params->momentum = 1;
	num++;
      }
    }
    else if (!strncmp(cptr, "map", 3)) /* Codebook vectors follow */
      break;

    if (num == 0 && *cptr != '\0' && *cptr != '#'){
      AddError("Unrecognized keyword found in header.");
      AddFileInfoOnError(finfo);  /* Add file status info to error message */
      break;
    }
    cptr = ReadLine(finfo);
  }

  if (CheckErrors())          /* Abort if there were errors in the header  */
    return CheckErrors();

  /* Verify that header contained useful information */
  if (finfo && finfo->byteorder > 0 && finfo->byteorder != LITTLE_ENDIAN && finfo->byteorder != BIG_ENDIAN && finfo->byteorder != PDP_ENDIAN){
    fprintf(stderr, ">>%d %d %d\n", finfo->byteorder, LITTLE_ENDIAN, BIG_ENDIAN);
    AddError("Invalid byteorder specified in file!");
  }
  if (map->xdim == 0 || map->ydim == 0)        /* No map dimension specified */
    AddError("Map dimension is zero!");

  if (map->dim == 0)
    AddError("Codebook dimension is zero!"); /* Codebook dimension is zero */

  if (cptr == NULL)            /* Obligatory keywork "map" not found   */
    AddError("This doesn't seem to be a codebook file.");

  if (!CheckErrors())          /* If no errors...                */
    ReadCodes(finfo, map);     /* then read the codebook entries */

  CloseFile(finfo);            /* clean up */

  if (!CheckErrors())          /* If no errors...                */
    fprintf(stderr, "%d codes%*s\n", map->xdim * map->ydim, 39-(int)(log10(map->xdim * map->ydim)), "[OK]");
  else
    fprintf(stderr, "%46s\n", "[FAILED]");

  return CheckErrors();
}

/******************************************************************************
Description: Save the Self-Organizing Map data in a given file in a given
             format. If format is 0, then write data in binary format,
             otherwise save the map in AscII format.

Return value: 0 if no errors, or a value greater than zero if there were errors
******************************************************************************/
int SaveMapInFormat(struct Parameters *params, char *fname, int format)
{
  UNSIGNED i;
  UNSIGNED clen;          /* Length of a label */
  char *label;            /* Pointer to label  */
  struct Map *map;
  struct FileInfo *finfo = NULL; /* File structure    */

  if (params == NULL)
    return 0;

  map = &params->map;
  fprintf(stderr, "Saving codebook entries....");/* Print what is being done */
  if (fname == NULL)                     /* Ensure that there is a file name */
    AddError("No file name given to save map.");
  else if ((finfo = OpenFile(fname, "wb")) == NULL) /* open output stream    */
    AddError("Unable to open file for writing.");

  if (CheckErrors()){                            /* Any errors so far?       */
    fprintf(stderr, "%47s\n", "[FAILED]");
    return CheckErrors();
  }

  if (format == 0 && FindEndian() == UNKNOWN)/* If binary mode was requested */
    format = 1;/* but Endian of this hardware is unknown switch to AscII mode*/

  /* Write header */
#ifdef PROG_VERSION
  fprintf(finfo->fptr, "#Written by: %s\n", PROG_VERSION);
#endif
  fprintf(finfo->fptr, "\n#Network properties:\n");
  fprintf(finfo->fptr, "Iteration=%u\n", map->iter);
  fprintf(finfo->fptr, "Dim=%u\n", map->dim);
  fprintf(finfo->fptr, "Xdim=%u\n", map->xdim);
  fprintf(finfo->fptr, "Ydim=%u\n", map->ydim);
  if (format == 0)  /* Write byteorder in binary mode */
    fprintf(finfo->fptr, "Byteorder=%d\n", FindEndian());
  fprintf(finfo->fptr, "Neighborhood=%s\n", GetNeighborhoodName(map->neighborhood));
  fprintf(finfo->fptr, "Topology=%s\n", GetTopologyName(map->topology));
  if (params){
    fprintf(finfo->fptr, "\n#Training parameters used:\n");
    if (params->rlen > 0)
      fprintf(finfo->fptr, "TrainIter=%u\n", params->rlen);
    if (params->alpha > 0.0)
      fprintf(finfo->fptr, "TrainAlpha=%.9f\n", params->alpha);
    if (params->beta > 0.0)
      fprintf(finfo->fptr, "TrainBeta=%.9f\n", params->beta);
    if (params->radius > 0)
      fprintf(finfo->fptr, "TrainRadius=%u\n", params->radius);
    if (params->datafile != NULL)
      fprintf(finfo->fptr, "TrainData=%s\n", params->datafile);
    if (params->validfile != NULL)
      fprintf(finfo->fptr, "TrainValid=%s\n", params->validfile);
    if (params->mu1 > 0.0)
      fprintf(finfo->fptr, "Trainmu1=%.9f\n", params->mu1);
    if (params->mu2 > 0.0)
      fprintf(finfo->fptr, "Trainmu2=%.9f\n", params->mu2);
    if (params->mu3 > 0.0)
      fprintf(finfo->fptr, "Trainmu3=%.9f\n", params->mu3);
    if (params->mu4 > 0.0)
      fprintf(finfo->fptr, "Trainmu4=%.9f\n", params->mu4);
    if (params->alphatype != 0)
      fprintf(finfo->fptr, "TrainAlphatype=%u\n", params->alphatype);
    if (params->batch != 0)
      fprintf(finfo->fptr, "TrainBatchmode=1\n");
    if (params->momentum != 0)
      fprintf(finfo->fptr, "TrainMomentum=1\n");
    if (params->super != 0)
      fprintf(finfo->fptr, "TrainSuper=%u\n", params->super);
    if (params->kernel != 0)
      fprintf(finfo->fptr, "TrainKernel=1\n");
  }
  fprintf(finfo->fptr, "\nmap\n");

  /* Write the codebooks */
  for (i = 0; i < map->xdim * map->ydim; i++){
    if (format)
      WriteAscIIVector(map->codes[i].points, map->dim, finfo);
    else
      WriteBinaryVector(map->codes[i].points, map->dim, finfo);

    if (CheckErrors() != 0)
      break;

    /* Write a codebook's label if available */
    if ((label = GetLabel(map->codes[i].label)) != NULL){
      clen = strlen(GetLabel(map->codes[i].label));
      if (format == 0){  /* In binary mode */
	if (fwrite(&clen, sizeof(unsigned), 1, finfo->fptr) != 1){
	  AddError("Unable to write data. File system full?");
	  break;
	}
	if (fwrite(label, sizeof(char), clen, finfo->fptr) != clen){
	  AddError("Unable to write data. File system full?");
	  break;
	}
      }
      else{ /* In AscII mode */
	if (fprintf(finfo->fptr, "%s\n", label) != clen){
	  AddError("Unable to write data. File system full?");
	  break;
	}
      }
    }
    else{ /* No label available */
      if (format == 0){  /* In binary mode */
	clen = 0;
	if (fwrite(&clen, sizeof(unsigned), 1, finfo->fptr) != 1){
	  AddError("Unable to write data. File system full?");
	  break;
	}
      }
      else{  /* In AscII mode */
	fputc('\n', finfo->fptr);
      }
    }
  }
  CloseFile(finfo);            /* clean up and return */

  if (!CheckErrors())          /* If no errors...                */
    fprintf(stderr, "%47s\n", "[OK]");
  else
    fprintf(stderr, "%47s\n", "[FAILED]");

  sync();  /* Ensure data is actually written to disk */
  return CheckErrors();
}

/******************************************************************************
Description: Save the Self-Organizing Map data to a given file in AscII format.

Return value: 0 if no errors, or a value greater than zero if there were errors
******************************************************************************/
int SaveMapAscII(struct Parameters *params)
{
  if (params->onetfile == NULL){         /* Ensure that there is a file name */
    AddError("No file name given to save map.");
    return 1;
  }
  return SaveMapInFormat(params, params->onetfile, 1);
}

/******************************************************************************
Description: Save the Self-Organizing Map data to a given file in the
             default binary format.

Return value: 0 if no errors, or a value greater than zero if there were errors
******************************************************************************/
int SaveMap(struct Parameters *params)
{
  if (params->onetfile == NULL){         /* Ensure that there is a file name */
    AddError("No file name given to save map.");
    return 1;
  }
  return SaveMapInFormat(params, params->onetfile, 0);
}

/******************************************************************************
Description: Save the snapshot of the Self-Organizing Map data to file in the
             default binary format.

Return value: 0 if no errors, or a value greater than zero if there were errors
******************************************************************************/
int SaveSnapShot(struct Parameters *params)
{
  if (params->snap.file == NULL){    /* Ensure that there is a file name */
    AddError("No file name given to save snapshot.");
    return 1;
  }
  fputc('\r', stderr);
  return SaveMapInFormat(params, params->snap.file, 0);
}

/* End of file */
