/*
  Contents: Routines used to train the som-sd.

  Author: Markus Hagenbuchner

  Comments and questions concerning this program package may be sent
  to 'markus@artificial-neural.net'


  ChangeLog:
    03/10/2006:
    - BugFix: File name for interrupted training runs was not unique if several
              processes terminated within one second.
    30/03/2006:
    - Bug removed: ComputeHexaDistance(.) now computes the correct distance.
    - Speed improvement: The Distance(.) functions now return a squared value
      which results in a speed improvement of about 4-8% depending on which
      Distance(.) function is used.
    - Speed improvement: Forced inlining of function AdaptVector(.) resulted in
      a speed gain of approx. 8%.
    20/05/2005:
    - Bug removed: Confusion of topology and neighborhood in TrainMap(.)

 */


/************/
/* Includes */
/************/
#include <ctype.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "common.h"
#include "data.h"
#include "fileio.h"
#include "system.h"
#include "train.h"
#include "utils.h"


/********************/
/* Global variables */
/********************/
int _save_then_exit_ = 0; /* Indicate if an interrupt was caught */



/* Begin functions... */

/******************************************************************************
Description: Function is used to compute the current alpha value (the
             learning rate) for the current training iteration, given the
             maximum number of training iterations, and given initial value
             of alpha at iter=0, and assuming a linearly decrease of alpha.

Return value: The alpha value for the given itaration assuming a linearly
              decreasing learning rate.
******************************************************************************/
FLOAT LinearDecrease(UNSIGNED iter, UNSIGNED length, FLOAT alphastart)
{
  return (alphastart * (FLOAT) (length - iter) / (FLOAT) length);
}

/******************************************************************************
Description: Function the current alpha value. Other parameters are ignored.
             It is a dummy function which is required to maintain compatibility
             with other alpha functions.

Return value: The alpha value for the given itaration which is assumed to
              remain unchanged.
******************************************************************************/
FLOAT ConstantAlpha(UNSIGNED iter, UNSIGNED length, FLOAT alphastart)
{
  return alphastart;
}

/******************************************************************************
Description: Function is used to compute the current alpha value (the
             learning rate) for the current training iteration, given the
             maximum number of training iterations, and given initial value
             of alpha at iter=0, and assuming an exponential decrease of alpha.
             The Contant INV_ALPHA_CONSTANT specifies the "steepness" of the
             decrease, and the lower limit of alpha.

Return value: The alpha value for the given itaration assuming a exponential
              decreasing learning rate.
******************************************************************************/
#define INV_ALPHA_CONSTANT 100.0
FLOAT ExponentialAlpha(UNSIGNED iter, UNSIGNED length, FLOAT alphastart)
{
  FLOAT c;
  
  c = length / INV_ALPHA_CONSTANT;
  
  return (alphastart * c / (c + iter));
}

/******************************************************************************
Description: Function is used to compute the current alpha value (the
             learning rate) for the current training iteration, given the
             maximum number of training iterations, and given initial value
             of alpha at iter=0, and assuming an sigmoidal decrease of alpha.
             The constant GAUSS_ALPHA_CONSTANT specifies the "steepness" of the
             decrease, and influences the lower limit of alpha.

Return value: The alpha value for the given itaration assuming a sigmoidal
              decreasing learning rate.
******************************************************************************/
#define GAUSS_ALPHA_CONSTANT 4.0
float SigmoidalAlpha(UNSIGNED iter, UNSIGNED length, FLOAT alphastart)
{
  return alphastart*exp(-((FLOAT)iter*iter*GAUSS_ALPHA_CONSTANT)/((FLOAT)length*length));
}

/******************************************************************************
Description: Compute the geographical distance between two codebook coordinates
             given a rectagonal neighborhood relationship.

Return value: The squared!! Eucledian distance between two points.
******************************************************************************/
FLOAT ComputeRectDistance(int bx, int by, int tx, int ty)
{
  FLOAT ret, diff;

  diff = bx - tx;
  ret = diff * diff;
  diff = by - ty;
  ret += diff * diff;

  return ret;
  //  return (FLOAT)sqrtf(ret);
}

/******************************************************************************
Description: Compute the geographical distance between two codebook coordinates
             given a hexagonal neighborhood relationship.

Return value: The squared!!  eucledian distance between two points.
******************************************************************************/
FLOAT ComputeHexaDistance(int bx, int by, int tx, int ty)
{
  FLOAT ret, dy;
  int dx;

  dx = bx - tx;
  dy = by - ty;

  if (dx & 1){
    if (tx & 1)
      dy += 0.5;
    else
      dy -= 0.5;
  }

  ret = dy * dy;
  ret += 0.75 * dx * dx;

  return ret;
  //  return (FLOAT)sqrtf(ret);
}

/******************************************************************************
Description: Compute the geographical distance between two codebook coordinates
             given a octagonal neighborhood relationship.

Return value: The squared distance between two points.
******************************************************************************/
FLOAT ComputeOctDistance(int bx, int by, int tx, int ty)
{
  FLOAT ret;

  ret = (FLOAT)max(abs(bx-tx), abs(by-ty));

  return ret*ret;
}

/******************************************************************************
Description: move a codebook vector towards another vector

Return value: 
******************************************************************************/
inline void AdaptVector(FLOAT *codebook, FLOAT *sample, UNSIGNED dim, FLOAT alpha)
{
  UNSIGNED i;

  for (i = 0; i < dim; i++)
    codebook[i] += alpha * (sample[i] - codebook[i]);
}

/******************************************************************************
Description: Find best matching codebook using the Eucledian distance meassure.

Return value: The best matching codebook is returned to parameter "winner".
******************************************************************************/
void FindWinnerEucledian(struct Map *map, struct Node *node, struct Graph *gptr, struct Winner *winner)
{
  FLOAT *mu;
  UNSIGNED vdim;
  UNSIGNED noc;  /* Number of codebooks in the map */
  FLOAT *codebook, *sample;
  UNSIGNED n, i;
  FLOAT diffsf, diff, difference;

  vdim = gptr->dimension;
  mu = node->mu;
  noc = map->xdim * map->ydim;
  diffsf = FLT_MAX;
  sample = node->points;
  for (n = 0; n < noc; n++){  /* For every codebook of the map */
    codebook = map->codes[n].points;
    difference = 0.0;

    /* Compute the difference between codebook and input entry */
    for (i = 0; i < vdim; i++){
      diff = codebook[i] - sample[i];
      difference += diff * diff * mu[i];
      if (difference > diffsf)
	break;
    }
    /* If distance is smaller than previous distances */
    if (difference < diffsf){
      winner->codeno = n;
      diffsf         = difference;
    }
  }
  winner->diff   = diffsf;

  return;
}

/******************************************************************************
Description: 

Return value: 
******************************************************************************/
void VQFindWinnerEucledian(struct Map *map, struct Node *node, struct Graph *gptr, struct Winner *winner)
{
  FLOAT *mu;
  UNSIGNED ldim, fanout, fanin, tend;
  UNSIGNED noc;  /* Number of codebooks in the map */
  FLOAT *codebook, *sample;
  UNSIGNED n, i;
  int id;

  FLOAT diffsf, diff, difference;

  ldim = gptr->ldim;            /* Offset for label component         */
  fanout = gptr->FanOut;        /* Offset for child state component   */
  fanin = gptr->FanIn;          /* Offset for parent state component  */
  tend = ldim+2*(fanin+fanout)+gptr->tdim; /* End of target vector component */

  mu = node->mu;

  noc = map->xdim * map->ydim;
  diffsf = FLT_MAX;
  sample = node->points;
  for (n = 0; n < noc; n++){  /* For every codebook of the map */
    codebook = map->codes[n].points;
    difference = 0.0;

    /* Compute the difference between codebook and input entry label */
    for (i = 0; i < ldim; i++){
      diff = codebook[i] - sample[i];
      difference += diff * diff * mu[i];
      if (difference >= diffsf)
      	goto big_difference;
    }

    /* Consider children coordinate vector */
    diff = map->codes[n].a;
    for (i = 0; i < fanout; i++){
      id = (int)sample[ldim + i*2];
      if (id >= 0)
	diff += (1.0 - 2 * codebook[ldim+noc*i+id]);
    }
    difference += diff * mu[i-1];
    if (difference >= diffsf)
      goto big_difference;

    /* Difference to parent coordinate vector */
    diff = map->codes[n].b;
    for (i = 0; i < fanin; i++){  
      id = (int)sample[ldim + 2+ fanout + i*2];
      if (id >= 0)
	diff += (1.0 - 2 * codebook[ldim+noc*fanout+noc*i+id]);
    }
    difference += diff * mu[i-1];
    if (difference >= diffsf)
      goto big_difference;

    /* Difference to target vector component */
    for (i = ldim + 2*fanin + 2*fanout; i < tend; i++){
      diff = codebook[i] - sample[i];
      difference += diff * diff * mu[i];
      if (difference >= diffsf)
      	goto big_difference;
    }

    /* Distance is smaller than previous distances */
    winner->codeno = n;
    diffsf         = difference;
  big_difference:
    continue;
  }
  winner->diff   = diffsf;

  return;
}

/******************************************************************************
Description: Adapt all codebook vectors which are located within a fixed
             radius around the winning codebook.

Return value: none
******************************************************************************/
void BubbleAdapt(struct Graph *gptr,struct Map *map, struct Node *node, struct Winner *winner, FLOAT radius, FLOAT alpha)
{
  UNSIGNED n, noc;
  FLOAT dist;
  FLOAT (*ComputeDistance)(int bx, int by, int tx, int ty);

  ComputeDistance = ComputeHexaDistance;

  noc = map->xdim * map->ydim;
  node->x = map->codes[winner->codeno].x;
  node->y = map->codes[winner->codeno].y;
  radius *= radius;  /* Distance computation is squared, thus square radius */
  for (n = 0; n < noc; n++){  /* For every codebook of the map */

    /* Compute distance to winner */
    dist = ComputeDistance(node->x, node->y, map->codes[n].x, map->codes[n].y);
    if (dist <= radius)
      AdaptVector(map->codes[n].points, node->points, map->dim,alpha);/*Update step*/
  }
}

/******************************************************************************
Description: Adapt all codebook vectors assuming a gaussian neighborhood
             relationship between the codebooks.

Return value: none
******************************************************************************/
void GaussianAdapt(struct Graph *gptr,struct Map *map, struct Node *node, struct Winner *winner, FLOAT radius, FLOAT alpha)
{
  UNSIGNED n, noc;
  //UNSIGNED off;
  FLOAT dist;
  FLOAT (*ComputeDistance)(int bx, int by, int tx, int ty);

  ComputeDistance = ComputeHexaDistance;

  noc = map->xdim * map->ydim;
  node->x = map->codes[winner->codeno].x;
  node->y = map->codes[winner->codeno].y;
  for (n = 0; n < noc; n++){  /* For every codebook of the map */

    /* Compute distance to winner */
    dist = ComputeDistance(node->x, node->y, map->codes[n].x, map->codes[n].y);
    //    if (dist > radius*4)
    //      continue;
	/*
    adapt(map->codes[n].points, node->points, gptr->ldim, node->mu1*alpha * expf((dist/(-2.0 * radius))));
    off = gptr->ldim;
    adapt(&map->codes[n].points[off], &node->points[off], 2*gptr->FanOut, node->mu2*alpha * expf((dist/(-2.0 * radius))));
    off += 2*gptr->FanOut;
    adapt(&map->codes[n].points[off], &node->points[off], 2*gptr->FanIn, node->mu3*alpha * expf((dist/(-2.0 * radius))));
    off += 2*gptr->FanIn;
    adapt(&map->codes[n].points[off], &node->points[off], gptr->tdim, node->mu4*alpha * expf((dist/(-2.0 * radius))));
    */
    /* Update the codebook */
    AdaptVector(map->codes[n].points, node->points, map->dim, alpha * expf((dist/(-2.0 * radius * radius))));
  }
}

/******************************************************************************
Description: 

Return value: 
******************************************************************************/
void VQAdapt(struct Graph *gptr,struct Map *map, struct Node *node, struct Winner *winner, FLOAT radius, FLOAT alpha)
{
  int i, n, id;
  FLOAT *codebook, a, b;
  UNSIGNED noc, ldim, offset;

  node->winner = winner->codeno;
  ldim = gptr->ldim;
  noc = map->xdim * map->ydim;

  /* Update the codebook */
  codebook = map->codes[winner->codeno].points;

  for (i = 0; i < ldim; i++)  /* update label component */
    codebook[i] += alpha * (node->points[i] - codebook[i]);

  /* update child coord component */
  a = 0.0;
  for (i = 0; i < gptr->FanOut; i++){
    id = node->points[ldim + 2*i];
    for (n = 0; n < noc; n++){
      if (n == id)
	codebook[ldim+i*noc+n] += alpha * (1 - codebook[ldim+i*noc+n]);
      else
	codebook[ldim+i*noc+n] -= alpha * codebook[ldim+i*noc+n];
      a += SQR(codebook[ldim+i*noc+n]);
    }
  }
  map->codes[winner->codeno].a = a;

  /* update parent coord component */
  offset = ldim+gptr->FanOut*noc;
  b = 0.0;
  for (i = 0; i < gptr->FanIn; i++){
    id = (int)node->points[gptr->ldim + 2*noc + 2*i];
    for (n = 0; n < noc; n++){
      if (n == id)
	codebook[offset+i*noc+n] += alpha * (1 - codebook[offset+i*noc+n]);
      else
	codebook[offset+i*noc+n] -= alpha * codebook[offset+i*noc+n];
      b += SQR(codebook[offset+i*noc+n]);
    }
  }
  map->codes[winner->codeno].b = b;

  /* update target component */
  offset = ldim + noc * (gptr->FanOut + gptr->FanIn);
  for (i = 0; i < gptr->tdim; i++)
    codebook[offset+i] += alpha * (node->points[ldim+2*(gptr->FanOut+gptr->FanIn)+i] - codebook[offset+i]);
}



/****************************************************************************
Description: Signal handler. This function specifies what to do when a ctrl-c
             is caught.

Return value: This function does not return a value.
****************************************************************************/
void SigHandler(int arg)
{
  static int flag = 0;
  time_t mytime;

  _save_then_exit_ = 1;      /* Indicate that we wish to save before exit */
  mytime = time(NULL);
  if (flag == 0){            /* If ctrl-c cought the first time... */
    fprintf(stderr, "\nFirst interrupt signal detected on %s", ctime(&mytime));
    SlideIn(stderr, 1, "Interrupting training...");
    fputc('\n', stderr);
    SlideIn(stderr, 0, "Wait for current iteration to stop for a safe exit or press ctrl-c again to");
    fputc('\n', stderr);
    SlideIn(stderr, 0, "force an immediate stop but then all trained network data will be lost!");
    fputc('\n', stderr);
  }

  else{                      /* If ctrl-c is cought more than once */
    fprintf(stderr, "\nSecond interrupt signal detected on %s", ctime(&mytime));
    fprintf(stderr, "Forced exit. Stopping now!\n");
    exit(0);                 /* Stop here and now */
  }
  flag++;
}

/****************************************************************************
Description: Installs a signal handler which will catch interrupt signals such
             as those initiated by ctrl-c of a kill command.

Return value: This function does not return a value.
****************************************************************************/
void InstallHandlers()
{
  struct sigaction act;

  memset(&act, 0, sizeof(struct sigaction));
  act.sa_handler =  SigHandler;
  sigaction(SIGINT, &act, NULL);
}

/******************************************************************************
Description: Set the appropriate function for computing the alpha value

Return value: A function pointer to the appropriate function for computing
              the alpha value.
******************************************************************************/
FLOAT (*SetAlpha(int alphatype))(UNSIGNED, UNSIGNED, FLOAT)
{
  if (alphatype == ALPHA_LINEAR)
    return LinearDecrease;  /* Linearly decreasing alpha      */
  else if (alphatype == ALPHA_EXPONENTIAL)
    return ExponentialAlpha;/* Exponentially decreasing alpha */ 
  else if (alphatype == ALPHA_CONSTANT)
    return ConstantAlpha;   /* Constant alpha value           */
  else
    return SigmoidalAlpha;  /* Default alpha type is sigmoidal*/
}

/******************************************************************************
Description: Set the appropriate function for adapting the network parameters

Return value: A function pointer to the appropriate function for adapting the
              network parameters.
******************************************************************************/
void (*SetAdapt(UNSIGNED neighborhood))(struct Graph*, struct Map*, struct Node*, struct Winner*, FLOAT, FLOAT)
{
  if (neighborhood == NEIGH_GAUSSIAN)
    return GaussianAdapt;    /* Gaussian neighborhood */
  else if (neighborhood == NEIGH_BUBBLE)
    return BubbleAdapt;      /* Strict neighborhood   */
  else
    return GaussianAdapt;    /* Default neighborhood  */
}

/******************************************************************************
Description: 

Return value: 
******************************************************************************/
int TrainMap(struct Parameters *parameters)
{
  UNSIGNED i, nnum;
  FLOAT alpha_t, radius_t;
  struct Map *map;
  struct Node *node;
  struct Graph *gptr;
  FILE *logfile;
  FLOAT (*GetAlpha)(UNSIGNED, UNSIGNED, FLOAT);
  void (*FindWinner)(struct Map *map, struct Node *node,struct Graph *gptr,struct Winner *winner);
  void (*Adapt)(struct Graph *gptr,struct Map *map, struct Node *node, struct Winner *winner, FLOAT radius, FLOAT alpha_t);
  void (*UpdateOffspringStates)(struct Graph *gptr, struct Node *node);
  int kstepmode = 1;
  struct Winner winner;
  UNSIGNED tlen, t;
  int counter;
  FLOAT terror;

  /* Sanity check */
  if (parameters->train == NULL){
    fprintf(stderr, "Warning: Training aborted. No training set provided.\n");
    return 0;
  }

  InstallHandlers();                           /* Install interrupt handler */
  logfile = MyFopen(parameters->logfile, "w"); /* Open log-file   */

  /* Set the appropriate function for computing the learning rate */
  GetAlpha = SetAlpha(parameters->alphatype);

  /* Set the appropriate function for computing the winner codebook   */
  FindWinner = FindWinnerEucledian;  /* Use Eucledian distance        */

  /* Set the appropriate function for adapting the network parameters */
  Adapt = SetAdapt(parameters->map.neighborhood);


  /* Set the appropriate function for updating childens location in nodes */
  if (parameters->map.topology == TOPOL_VQ){           /* In VQ mode...   */
    UpdateOffspringStates = UpdateChildrensLocationVQ; /* use ID value    */
    FindWinner = VQFindWinnerEucledian; /* Use Eucledian distance VQ mode */
    Adapt = VQAdapt;   /* No topology = no neighborhood = VQ adapt mode   */
    VQSet_ab(parameters);  /* Initialize auxillary variables a and b      */
  }
  if (parameters->contextual){
    if (parameters->undirected)
      kstepmode = 0;
    else if (parameters->train->FanIn == 0){
      fprintf(stderr, "Warning: No inlink available for contextual mode. Will fall back to normal mode.\n");
      UpdateOffspringStates = UpdateChildrensLocation;   /* Use coordinates */
      parameters->contextual = NO;
    }
    if(parameters->contextual){
      fprintf(stderr, "Contextual mode: Training on single map is assumed\n");
      fprintf(stderr, "Will recompute states at every iteration!!\n");
      UpdateOffspringStates = UpdateChildrenAndParentLocation;/*p & c coords*/
      K_Step_Approximation(&parameters->map, parameters->train, kstepmode);
    }
  }
  else
    UpdateOffspringStates = UpdateChildrensLocation;   /* Use coordinates */

  InitProgressMeter(parameters->rlen);   /* Initialize the progress meter */
  fprint(stderr, "Training map......");  /* Print what is being done      */
  map = &parameters->map;

  tlen = 0;                   /* Compute the total number of update steps */
  for (gptr = parameters->train; gptr != NULL; gptr = gptr->next)
    tlen += gptr->numnodes;
  tlen = tlen * (parameters->rlen - map->iter);

  t = 0;
  for (i = map->iter; i < parameters->rlen; i++){
    if (parameters->graphorder == 1)
      parameters->train = RandomizeGraphOrder(parameters->train);

    counter = 0;
    terror = 0.0;
    for (gptr = parameters->train; gptr != NULL; gptr = gptr->next){
      for (nnum = 0; nnum < gptr->numnodes; nnum++){
	node = gptr->nodes[nnum];
	alpha_t = GetAlpha(t, tlen, parameters->alpha);
	radius_t = 1.0 + (parameters->radius - 1.0) * (float)(tlen - t)/(float)tlen;
	t++;
	if (!parameters->contextual)
	  UpdateOffspringStates(gptr, node);  /* Update child-state-vector   */
	FindWinner(map, node, gptr, &winner); /* Find best matching codebook */
	Adapt(gptr, map, node, &winner, radius_t, alpha_t);/* update codebook*/
	terror += winner.diff;
	counter++;
      }
    }

    if (parameters->contextual)
      K_Step_Approximation(&parameters->map, parameters->train, kstepmode);

    map->iter++;
    fprintf(logfile, "%f\n", terror/counter);  /* Print normalized q-error */
    fflush(logfile);

    if (_save_then_exit_){ /* Save and exit if a interrupt signal was caught */
      char fname[32];
      sprintf(fname, "interrupted%d.net", (int)getpid());
      free(parameters->onetfile);
      parameters->onetfile = strdup(fname);     /* Assign alternate filename */
      fprintf(stderr, "\nSaving net to '%s'\n", parameters->onetfile);
      break;                                    /* Break the training cycle  */
    }

    /* Create a snapshot if required */
    if (parameters->snap.interval>0 &&!(map->iter %parameters->snap.interval)){
      if (parameters->snap.file != NULL)
	SaveSnapShot(parameters);
      if (parameters->snap.command != NULL)
	system(parameters->snap.command);
    }

    if (parameters->nice)
      SleepOnHiLoad();  /* Sleep when system load is high */

    PrintProgress(map->iter);  /* Print Progress */
  }
  StopProgressMeter();
  if (!_save_then_exit_)
    fprintf(stderr, "%56s\n", "[OK]");

  if (logfile != stdout)
    MyFclose(logfile);

  return 0;
}
