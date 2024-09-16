#ifndef UTILS_H_DEFINED
#define UTILS_H_DEFINED

#include <time.h>

#define UNKNOWN       0x00
#define NO            0x00
#define YES           0x01

#define TYPE_STRING    1
#define TYPE_INT       2
#define TYPE_UNSIGNED  3
#define TYPE_FLOAT     4

#define RAW  0
#define GZIP 1
#define BZIP 2

/* Memory management utilities */
void *MyMalloc(size_t size);               /* Fail safe malloc */
void *MyCalloc(size_t nmemb, size_t size); /* Fail safe calloc */
void *MyRealloc(void *ptr, size_t size);   /* Fail safe realloc */
void *memdup(void *ptr, size_t size);      /* Duplicate a memory area */

/* String functions */
char *stradd(char *str1, char *str2);      /* Concatenate two strings   */
char *strend(char *str);                   /* Return the pointer pointing to end of str1 */
char *strnspc(char *str);                  /* Find a non-white space in str */
char *strnstr(char *haystack, char *needle);/* Find characters not in needle */
int oatoi(char *str, int idefault);        /* Convert string to integer */
unsigned oatou(char *str, int udefault);   /* Convert a string to unsigned */
float oatof(char *cptr, float fdefault);   /* Convert string to float   */
int satoi(char *cptr, int *ival);          /* Convert string to int and store*/
int satou(char *cptr, unsigned *uval);     /* Convert str to unsigned & store*/
int satof(char *cptr, float *fval);        /* Convert string to float & store*/
int satos(char *cptr, char **sptr);        /* Convert string to string& store*/
int strstart(char *sub, char *str);        /* Check is str starts with 'sub' */

/* Print functions */
int fprint(FILE *ostream, char *text);     /* Flushed printing of text      */
void SlideIn(FILE *ostream, int speed, char *msg);/*Slide in a msg from left*/
void PrintFloat(FILE *ofile, float f);     /* High precision print of float */
char* PrintTime(time_t time);              /* Convert seconds into minutes, hours, etc. */

/* Functions that help to deal with command line options */
void GetArg(int type, int argc, char **argv, int idx, void *dest);

/* Error handling and error reporting functions */
void AddError(char *msg);  /* Add an error message              */
void PrintErrors();        /* Print and clear error messages    */
void ClearErrors();        /* Clear error messages              */
unsigned CheckErrors();    /* Check if there are error messages */

/* Message buffering and reporting functions */
void AddMessage(char *msg);   /* Add a message               */
void PrintMessages();         /* Print and clear messages    */
void ClearMessages();         /* Clear messages              */
unsigned CheckMessages();     /* Check if there are messages */

/* Progress meter */
void InitProgressMeter(int max);   /* Initialize the progress meter */
void PrintProgress(int state);     /* Print progress                */
void StopProgressMeter();          /* Stop the progress meter       */

/* Math and logic functions */
int approx(float v1, float v2, float deviation);  /*Are two values about same*/
int similar(float val1, float val2, float threshold);/*Are two values similar*/
int BitCount(int *array, int size); /* Count number of bits in array */

/* File handling functions */
FILE *MyFopen(const char *path, const char *mode);
int MyFclose(FILE *stream);
int GetCompressStatus(const char *path, char *mode);

/* gcc v4 seems to have fmaxf undefined. Hence, we define it here */
#ifndef fmaxf
extern float fmaxf(float, float);
#endif

#endif
