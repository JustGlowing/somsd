#ifndef FILEIO_H_DEFINED
#define FILEIO_H_DEFINED

struct FileInfo{
  char *fname;        /* Name of the file      */
  UNSIGNED lineno;    /* Line number we are on */
  unsigned byteorder; /* Byte order (nonzero value indicates a binary file) */
  int   ctype;        /* Type of compression (if file is compressed)        */
  FILE *fptr;         /* File pointer          */
};

struct Graph* LoadData(char *fname);
void SaveData(FILE *ofile, struct Graph *graph);
int LoadMap(struct Parameters *params);
int SaveMap(struct Parameters *);
int SaveMapAscII(struct Parameters *);
int SaveSnapShot(struct Parameters *);

#endif
