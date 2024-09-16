/*
  Contents: The main module of the network initializer for somsd

  Author: Markus Hagenbuchner

  Comments and questions concerning this program package may be sent
  to 'markus@artificial-neural.net'
 */


/************/
/* Includes */
/************/
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "common.h"
#include "data.h"
#include "fileio.h"
#include "system.h"
#include "train.h"
#include "utils.h"


/* Begin functions... */

/******************************************************************************
Description: Print usage information to the screen

Return value: The function does not return.
******************************************************************************/
void Usage()
{
  fprintf(stderr, "\n\
Usage: initsom [options]\n\n\
Options are:\n\
    -din <fname>       The file which holds the training data\n\
    -cout <fname>      The newly initialized map will be saved in filename.\n\
    -topol <type>      Specify the topology type of the map. Type can be\n\
                       rectagonal   Neurons are 4-connected\n\
                       hexagonal    Neurons are 6-connected. (the default)\n\
                       octagonal    Neurons are 8-connected.\n\
                       vq           VQ mode (no topology).\n\
    -neigh <type>      The neighborhood type which can be\n\
                       bubble       Limit neighborhood relationship\n\
                       gaussian     Gaussian bell relationship (default)\n\
    -seed <int>        Use int as the seed for the random number generator.\n\
                       Default seed is current system time.\n\
    -linear            Use linear initialization. Codebook vectors with values\n\
                       linearily increasing with the distance from the origin.\n\
    -xdim <xdim>       Horizontal extension of the map.\n\
    -ydim <ydim>       Vertical extension of the map.\n\
    -help              Print this help.\n\
 \n");
  exit(0);
}

/******************************************************************************
Description: main

Return value: zero
******************************************************************************/
int main(int argc, char **argv)
{
  UNSIGNED i, mode;
  struct Graph *data = NULL;
  struct Map *map;
  struct Parameters parameter;

  memset(&parameter, 0, sizeof(struct Parameters));/* Init with 0-values     */
  parameter.seed = (UNSIGNED)time(NULL);           /* Random seed by default */
  mode = INIT_DEFAULT;
  map = &parameter.map;
  for (i = 1; i < argc; i++){
    if (!strncmp(argv[i], "-cout", 2))
      GetArg(TYPE_STRING, argc, argv, i++, &parameter.onetfile);
    else if (!strncmp(argv[i], "-din", 2))
      GetArg(TYPE_STRING, argc, argv, i++, &parameter.datafile);
    else if (!strncmp(argv[i], "-neigh", 2))
      map->neighborhood = GetNeighborhoodID(argv[++i], NULL);
    else if (!strncmp(argv[i], "-seed", 2))
      GetArg(TYPE_INT, argc, argv, i++, &parameter.seed);
    else if (!strncmp(argv[i], "-topol", 2))
      map->topology = GetTopologyID(argv[++i], NULL);
    else if (!strcmp(argv[i], "-linear"))
      mode = INIT_LINEAR;
    else if (!strncmp(argv[i], "-verbose", 5))
      parameter.verbose = 1;
    else if (!strncmp(argv[i], "-xdim", 2))
      GetArg(TYPE_UNSIGNED, argc, argv, i++, &map->xdim);
    else if (!strncmp(argv[i], "-ydim", 2))
      GetArg(TYPE_UNSIGNED, argc, argv, i++, &map->ydim);
    else if (!strcmp(argv[i], "-help") || !strcmp(argv[i], "-h") || !strcmp(argv[i], "-?")){
      Usage();
    }
    else
      fprintf(stderr, "Warning: Ignoring unrecognized command line option '%s'\n", argv[i]);

    if (CheckErrors() != 0)
      break;
  }

  if (parameter.verbose != 0){
    PrintSoftwareInfo(stderr); /* Print a nice header with software and */
    PrintSystemInfo(stderr);   /* hardware information */
  }


  if (CheckErrors() == 0){                 /* No errors so far ... */
    /* Check that we have useful initial data */
    if (map->topology == TOPOL_VQ)
      map->neighborhood = NEIGH_NONE;
    else if (map->neighborhood == UNKNOWN){
      fprintf(stderr, "Note: Will use default neighborhood 'gaussian'\n");
      map->neighborhood = NEIGH_GAUSSIAN;
    }
    if (map->topology == UNKNOWN){
      fprintf(stderr, "Note: Will use default topology 'hexagonal'\n");
      map->topology = TOPOL_HEXA;
    }
    if (map->xdim * map->ydim == 0)
      AddError("Network dimension not specified, or is zero.");
  }

  if (CheckErrors() == 0)                /* No errors so far ... */
    data = LoadData(parameter.datafile); /* Load the dataset     */

  if (CheckErrors() == 0){     /* If there were no errors so far then      */
    fprintf(stderr, "Initializing network...."); /* Print what's happening */
    srand48(parameter.seed);   /* Initialize random number generator       */
    InitCodes(map, data, mode);/* Allocate and initialize the map          */
    if (CheckErrors() == 0)
      fprintf(stderr, "%50s\n", "[OK]");    /* print confirmation          */
    else
      fprintf(stderr, "%50s\n", "[FAILED]");/*Network initialization failed*/
  }
  if (CheckErrors() == 0)       /* If there are still no errors then   */
    SaveMap(&parameter);        /* save the map to a file    */

  if (CheckErrors()){           /* If there were errors then */
    PrintErrors();              /* print them.               */
    return 1;
  }

  if (parameter.verbose != 0)
    fprintf(stderr, "all done.\n");

  Cleanup(&parameter);         /* Free allocated memory and flush errors */
  return 0;
}

/* End of file */
