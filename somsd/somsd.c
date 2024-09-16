/*
  Contents: The main module of the som-sd package.

  Author: Markus Hagenbuchner

  Comments and questions concerning this program package may be sent
  to 'markus@artificial-neural.net'

  ChangeLog:
    03/10/2006
      - Port to CYGWIN complete.
      - Be more verbose about bad command line parameters, and attempts to
        access features that are not yet implemented.
      - All command line parameters (other than -cin) now have default values:
      - Compute default neighborhood radius if not specified at command line.
      - Use 64 as default number of training iterations if -iter was not
        specified at the command line.
      - Use 1.0 as default value for the learning rate if -alpha was not
        specified at the command line.
    14/09/2006
      - BugFix: Specification of seed at command line option had no effect.
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
#include <time.h>
#include "common.h"
#include "data.h"
#include "fileio.h"
#include "system.h"
#include "train.h"
#include "utils.h"

extern int TrainMapThread(struct Parameters *parameters);


/* Begin functions... */

/******************************************************************************
Description: Print usage information to the screen

Return value: The function does not return.
******************************************************************************/
void Usage()
{
  fprintf(stderr, "\n\
Usage: somsd [options]\n\n\
Options are:\n\
    -alpha <float>        initial learning rate alpha value\n\
    -cin <filename>       initial codebook file\n\
    -cout <filename>      the trained map will be saved in filename.\n\
    -din <filename>       The file which holds the training data\n\
    -iter <int>           The number of training iterations.\n\
    -radius <float>       initial radius of neighborhood\n\
    -seed <int>           seed for random number generator. 0 is current time\n\
    -batch                use batch mode training\n\
    -contextual           Contextual mode (single map).\n\
    -log <filename>       Print loging information to <filename>. If <filename>\n\
                          is '-' then print to stdout. At current, only the\n\
                          running quantization error is logged.\n\
    -simple_kernel        Use a simple kernel SOM.\n\
    -kernel               Use a full kernel SOM.\n\
    -momentum <float>     use momentum term (implies -batch)\n\
    -nice                 Be nice, sleep while system load is high.\n\
    -alpha_type <type>    Type of alpha decrease. Type can be either:\n\
                          sigmoidal    (default) sigmoidal decrease.\n\
                          linear       linear decrease.\n\
                          exponential  exponential decrease.\n\
                          constant     no decrease. Alpha remains constant.\n\
    -randomize <entity>   Randomize the order of an entity. Valid entities are:\n\
                          nodes, graphs. By default, the order of graphs is\n\
                          maintained as read from a datafile while nodes are\n\
                          sorted in an inverse topological order. This option\n\
                          allows to change this behaviour.\n\
    -snapfile <filename>  snapshot filename\n\
    -snapinterval <int>   interval between snapshots\n\
    -exec <command>       Execute the <command> every <snapinterval>.\n\
    -super <mode>         Enable supervised training in a given mode which can\n\
                          be either of the following:\n\
                          kohonen : supervised training Kohonen like. Kohonen\n\
                              attaches numeric target vectors to the network\n\
                              input to achive supervison.\n\
                          inheritance: same as kohonen mode but descendant\n\
                              nodes inherit class label from parents.\n\
                          rejection : supervised training using a global\n\
                              rejection term. Requires symbolic targets.\n\
                          localreject: supervised training using a local\n\
                              rejection term. Requires symbolic targets.\n\
    -beta <float>         rejection rate in conjunction with -super only.\n\
    -vin <filename>       validation data set.\n\
    -mu1 float            Weight for the label component.\n\
    -mu2 float            Weight for the position component.\n\
    -mu3 float            Weight for the parents position component.\n\
    -mu4 float            Weight for the class label component.\n\
    -undirected           Treat all links as undirected links.\n\
    -v                    Be verbose.\n\
    -help                 Print this help.\n\
 \n");

#ifdef _BE_MULTITHREADED
  fprintf(stderr, "Advanced Options:\n\
    -cpu <n>              Assume that the system has exactly <n> CPUs. This\n\
                          controls the level of parallelism of this software.\n\
 \n");
#endif

  exit(0);
}

/******************************************************************************
Description: Convert the value string associated with command line option
             -super into a corresponding type value.

Return value: The value associated with the constant KOHONEN, INHERITANCE, 
              REJECT, LOCALREJECT, or UNKNOWN depending on whether the value
              string was "kohonen", "inheritance", "rejection", "localreject",
              or neither of these.
******************************************************************************/
UNSIGNED GetSuperMode(char *arg)
{
  if (arg == NULL)
    return UNKNOWN;
  else if (strncasecmp(arg, "kohonen", 3))
    return KOHONEN;
  else if (strncasecmp(arg, "inheritance", 3))
    return INHERITANCE;
  else if (strncasecmp(arg, "rejection", 3))
    return REJECT;
  else if (strncasecmp(arg, "localreject", 3))
    return LOCALREJECT;
  else
    return UNKNOWN;
}

/******************************************************************************
Description: Retrieves recognized command line options and stores given
             options in the Parameters structure.

Return value: A pointer to a dynamically allocated Parameters structure which
              is initialized with values specified in the command line, or null
              for elements not touched by the given command line parameters.
******************************************************************************/
struct Parameters* GetParameters(struct Parameters *parameters, int argc, char **argv)
{
  int i;

  for (i = 1; i < argc; i++){
    if (!strcmp(argv[i], "-cin"))
      GetArg(TYPE_STRING, argc, argv, i++, &parameters->inetfile);
    else if (!strcmp(argv[i], "-din"))
      GetArg(TYPE_STRING, argc, argv, i++, &parameters->datafile);
    else if (!strcmp(argv[i], "-vin"))
      GetArg(TYPE_STRING, argc, argv, i++, &parameters->validfile);
    else if (!strcmp(argv[i], "-cout"))
      GetArg(TYPE_STRING, argc, argv, i++, &parameters->onetfile);
    else if (!strcmp(argv[i], "-log"))
      GetArg(TYPE_STRING, argc, argv, i++, &parameters->logfile);
    else if (!strcmp(argv[i], "-iter"))
      GetArg(TYPE_UNSIGNED, argc, argv, i++, &parameters->rlen);
    else if (!strcmp(argv[i], "-alpha"))
      GetArg(TYPE_FLOAT, argc, argv, i++, &parameters->alpha);
    else if (!strcmp(argv[i], "-alpha_type"))
      parameters->alphatype = GetAlphaType(argv[++i]);
    else if (!strcmp(argv[i], "-beta"))
      GetArg(TYPE_FLOAT, argc, argv, i++, &parameters->beta);
    else if (!strcmp(argv[i], "-radius"))
      GetArg(TYPE_UNSIGNED, argc, argv, i++, &parameters->radius);
    else if (!strcmp(argv[i], "-seed"))
      GetArg(TYPE_UNSIGNED, argc, argv, i++, &parameters->seed);
    else if (!strcmp(argv[i], "-exec"))
      GetArg(TYPE_STRING, argc, argv, i++, &parameters->snap.command);
    else if (!strcmp(argv[i], "-batch"))
      parameters->batch = 1;
    else if (!strcmp(argv[i], "-cpu"))
      GetArg(TYPE_UNSIGNED, argc, argv, i++, &parameters->ncpu);
    else if (!strncmp(argv[i], "-context", 8))
      parameters->contextual = 1;
    else if (!strcmp(argv[i], "-simple_kernel"))
      parameters->kernel = KERNEL_SIMPLE;
    else if (!strcmp(argv[i], "-kernel"))
      parameters->kernel = KERNEL_FULL;
    else if (!strcmp(argv[i], "-momentum"))
      parameters->momentum = 1;
    else if (!strcmp(argv[i], "-mu1"))
      GetArg(TYPE_FLOAT, argc, argv, i++, &parameters->mu1);
    else if (!strcmp(argv[i], "-mu2"))
      GetArg(TYPE_FLOAT, argc, argv, i++, &parameters->mu2);
    else if (!strcmp(argv[i], "-mu3"))
      GetArg(TYPE_FLOAT, argc, argv, i++, &parameters->mu3);
    else if (!strcmp(argv[i], "-mu4"))
      GetArg(TYPE_FLOAT, argc, argv, i++, &parameters->mu4);
    else if (!strcmp(argv[i], "-nice"))
      parameters->nice = 1;
    else if (!strncmp(argv[i], "-randomize", 7)){
      if (!(parameters->nodeorder = (strncmp(argv[++i], "node", 4) ? 0 : 1)) &&
	  !(parameters->graphorder = (strncmp(argv[i], "graph", 5) ? 0 : 1))){
	fprintf(stderr, "Warning: Ignoring unrecognized value '%s' for option -randomize.\n", argv[i]);
      }
    }
    else if (!strcmp(argv[i], "-snapfile"))
      GetArg(TYPE_STRING, argc, argv, i++, &parameters->snap.file);
    else if (!strcmp(argv[i], "-snapinterval"))
      GetArg(TYPE_UNSIGNED, argc, argv, i++, &parameters->snap.interval);
    else if (!strcmp(argv[i], "-super"))
      parameters->super = GetSuperMode(argv[++i]);
    else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "-verbose"))
      parameters->verbose = 1;
    else if (!strcmp(argv[i], "-undirected")){
      parameters->undirected = 1;
      parameters->contextual = 1;
    }
    else if (!strcmp(argv[i], "-help") || !strcmp(argv[i], "-h") || !strcmp(argv[i], "-?")){
      Usage();
    }
    else{
      fprintf(stderr, "Warning: Ignoring unrecognized command line option '%s'\n", argv[i]);
    }

    if (CheckErrors() != 0)
      break;
  }

  return parameters;
}

/******************************************************************************
Description: Check given command line parameters for consistency, set default
          values if necessary, and print warnings if there unusual parameters
          are used.

Return value: The function does not return a value. 
******************************************************************************/
void CheckParameters(struct Parameters* parameters)
{
  char msg[256];

  fprintf(stderr, "Checking parameters");
  if (parameters->map.codes == NULL)
    AddError("No Map, or Map is empty.");

  if (parameters->datafile == NULL)
    AddError("No training data.");

  if (parameters->onetfile == NULL){
    parameters->onetfile = strdup("trained.net");  // Set default output file
    AddMessage("WARNING: Will save trained network to file 'trained.net'.");
    AddMessage("         Use option -cout to alter this behaviour.");
  }

  if (parameters->rlen == 0){
    AddMessage("WARNING: Number of training iterations not specified or zero.");
    AddMessage("         Training iterations defaults to: 64");
    AddMessage("         Use option -iter to alter this behaviour.");
    parameters->rlen  = 64;
  }

  if (parameters->alpha == 0.0)
    AddMessage("WARNING: Learning rate is zero.");

  if (parameters->alpha < 0.0)
    AddMessage("WARNING: Learning rate is negative!");

  if (parameters->alpha > 2.0){
    AddMessage("WARNING: Learning rate is likely to be too large.");
    AddMessage("         Suggested values for learning rate are within [0;1]");
  }

  if (parameters->alphatype == UNKNOWN)
    parameters->alphatype = ALPHA_SIGMOIDAL; /* Set default alpha type */

  if ((parameters->super == REJECT ||  parameters->super == LOCALREJECT)){
    if (parameters->beta == 0.0)
      AddMessage("WARNING: Rejection rate is zero in supervised mode.");
  }
  else if (parameters->beta != 0.0){
    parameters->beta = 0.0;
    AddMessage("Note: Not in corresponding supervised mode. Will ignore rejection rate.");
  }

  /* Check if a useful neigborhood radius is specified. In some cases a
     radius can be zero or negative. But in most cases it cannot. */
  if (parameters->radius == 0 && parameters->map.topology != TOPOL_VQ){
    AddMessage("WARNING: Neigborhood radius not specified or zero!");
    parameters->radius  = 1.0+sqrt(parameters->map.xdim * parameters->map.xdim + parameters->map.ydim * parameters->map.ydim)/9.0;
    sprintf(msg, "         Neigborhood radius defaults to: %d", parameters->radius);
    AddMessage(msg);
    AddMessage("         Use option -radius to alter this behaviour.");
  }

  /* Check seed values and initialize random number generator if needed */
  if (parameters->seed > 0)
    srand48(parameters->seed);

  if (parameters->batch == 0 && parameters->momentum != 0){
    parameters->batch = 1;   /* momentum term requires batch mode processing */
  }

  if (parameters->batch != 0){
    AddMessage("WARNING: Batch mode processing not yet implemented!");
    AddMessage("         Will proceed in default online mode.");
  }

  if (parameters->super != 0){
    AddMessage("WARNING: Supervised processing not yet implemented!");
    AddMessage("         Will proceed in default unsupervised mode.");
  }

  if (parameters->kernel != 0){
    AddMessage("WARNING: Kernel mode processing not yet implemented!");
    AddMessage("         Will proceed in default SOM-SD mode.");
  }

  if (parameters->ncpu == 0)
    parameters->ncpu = GetNumCPU();

  if (parameters->logfile == NULL || strlen(parameters->logfile) == 0)
    parameters->logfile = strdup("somsd.log");

  if (parameters->snap.interval > 0 && parameters->snap.file == NULL && parameters->snap.command == NULL){
    parameters->snap.file = strdup("snapshot.net");
    AddMessage("Note: Will save a snapshots to file 'snapshot.net'");
  }
  else if (parameters->snap.interval == 0 && parameters->snap.file != NULL){
    parameters->snap.interval = 1;
    AddMessage("Note: Will save a snapshots at every iteration");
  }

  SuggestMu(parameters);/* Compute optimal weight parameters */

  /* Print error and warning messages if there are any */
  if (CheckErrors()){
    fprintf(stderr, "%55s\n", "[FAILED]");
    return;
  }
  else{
    if (CheckMessages()){
      fprintf(stderr, "%55s\n", "[CAUTION]");
      PrintMessages();
      fprintf(stderr, "\n");
    }
    else
      fprintf(stderr, "%55s\n", "[OK]");
  }
}

/******************************************************************************
Description: main

Return value: zero
******************************************************************************/
int main(int argc, char **argv)
{
  struct Parameters parameters;
  time_t starttime;

  starttime = time(NULL);
  memset(&parameters, 0, sizeof(struct Parameters));
  parameters.alpha = 1.0; /* Default learning rate */
  GetParameters(&parameters, argc, argv);

  if (parameters.verbose){
    PrintSoftwareInfo(stderr); /* Print a nice header with software and */
    PrintSystemInfo(stderr);   /* hardware information */
  }

  if (CheckErrors() == 0)
    LoadMap(&parameters);  /* Load the map */

  if (CheckErrors() == 0)
    GetParameters(&parameters, argc, argv);  /* Overwrite network parameters*/

  if (CheckErrors() == 0)
    parameters.train = LoadData(parameters.datafile); /* Load training data */

  if (CheckErrors() == 0 && parameters.validfile != NULL)
    parameters.valid = LoadData(parameters.validfile);/*Load validation data*/

  if (CheckErrors() == 0)
    CheckParameters(&parameters);       /* Check for parameter consistancy  */

  if (CheckErrors() == 0 && parameters.undirected)      /* treat undirected */
    ConvertToUndirectedLinks(parameters.train);

  if (CheckErrors() == 0)
    PrepareData(&parameters);/* Prepare data for training phase */

  if (CheckErrors() == 0){
    TrainMap(&parameters); /* Train the network                 */ 
    SaveMap(&parameters);  /* Save the trained map              */
  }

  if (CheckErrors())       /* If there were errors then         */
    PrintErrors();         /* print them.                       */
  else{
    fprintf(stderr, "Total time: %s\n", PrintTime(time(NULL) - starttime));
    fprintf(stderr, "all done.\n");
  }

  /* Debugging stuff only */
  //  parameters.onetfile = strdup("test2");
  //SaveMapAscII(&parameters);    /* save the map to an asc-ii file.        */
  Cleanup(&parameters);         /* Free allocated memory and flush errors */

  return 0;
}

/* End of file */
