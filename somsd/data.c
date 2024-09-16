/*
  Contents: Functions used for data handling and data verification.

  Author: Markus Hagenbuchner

  Comments and questions concerning this program package may be send
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
#include "common.h"
#include "train.h"
#include "utils.h"


/********************/
/* Global variables */
/********************/
UNSIGNED Number_Labels  = 0;
char **Data_Labels = NULL;
struct LabelArray {
  char *label;
  int pos;
};


/* Begin functions... */

/*****************************************************************************
Description: Add label to an array of labels if the label is not already in
             the array.

Return value: The index (when starting to count from 1) of where the label
              is stored in the array.
*****************************************************************************/
UNSIGNED AddLabel(char *label)
{
  UNSIGNED i;

  if (label == NULL)
    return MAX_UNSIGNED;

  for (i = 0; i < Number_Labels; i++)
    if (!strcmp(Data_Labels[i], label))
      return i+1;

  Number_Labels++;
  Data_Labels = MyRealloc(Data_Labels, Number_Labels * sizeof(char*));
  Data_Labels[Number_Labels-1] = strdup(label);
  return Number_Labels;
}

/*****************************************************************************
Description: Return a pointer to a data label which corresponds to a given
             index value.

Return value: A pointer to a data label, or NULL if the index is undefined.
*****************************************************************************/
char* GetLabel(UNSIGNED index)
{
  if (index == 0 || index > Number_Labels)
    return NULL;
  return Data_Labels[index-1];
}

/*****************************************************************************
Description: Return the number of labels currently stored in the array
             Data_Labels.

Return value: The number of labels in Data_Labels.
*****************************************************************************/
UNSIGNED GetNumLabels()
{
  return Number_Labels;
}

int LabelSorter(const void *arg1, const void *arg2)
{
  return strcmp(((struct LabelArray*)arg1)->label, ((struct LabelArray*)arg2)->label);
}

/*****************************************************************************
Description: Return a list of label indexes which alphabetically sorts the
             labels.

Return value: An integer array of the same length as the number of labels.
*****************************************************************************/
int *GetSortedLabelIndex()
{
  static int *sindex = NULL;
  static int len = 0;
  int i;
  struct LabelArray *tmparray;

  if (sindex)
    free(sindex);
  sindex = calloc(Number_Labels, sizeof(int));
  tmparray = (struct LabelArray*)calloc(Number_Labels, sizeof(struct LabelArray));
  for (i = 0; i < Number_Labels; i++){
    tmparray[i].label = Data_Labels[i];
    tmparray[i].pos = i;
  }

  qsort(tmparray, Number_Labels, sizeof(struct LabelArray), LabelSorter);
  for (i = 0; i < Number_Labels; i++)
    sindex[i] = tmparray[i].pos+1;
  free(tmparray);
  len = Number_Labels;

  return sindex;
}


/*****************************************************************************
Description: Clear all labels from the list of data labels.

Return value: The function does not return a value.
*****************************************************************************/
void ClearLabels()
{
  UNSIGNED i;

  for (i = 0; i < Number_Labels; i++)
    free(Data_Labels[i]);
  Number_Labels = 0;
}

/*****************************************************************************
Description: Find the children of the current node, and assign the position of
             the childrens' winner neuron to the current node.

Return value: The function does not return a value.
*****************************************************************************/
void UpdateChildrensLocation(struct Graph *gptr, struct Node *node)
{
  UNSIGNED i, offset;

  offset = gptr->ldim;
  for (i = 0; i < gptr->FanOut; i++){
    if (node->children[i] != NULL){
      node->points[offset+i*2] =  (FLOAT)node->children[i]->x;
      node->points[offset+i*2+1]= (FLOAT)node->children[i]->y;
    }
  }
}

/*****************************************************************************
Description: In VQ mode, find the children of the current node, and assign the
             ID of the childrens' winner neuron to the current node.

Return value: The function does not return a value.
*****************************************************************************/
void UpdateChildrensLocationVQ(struct Graph *gptr, struct Node *node)
{
  UNSIGNED i, offset;

  offset = gptr->ldim;
  for (i = 0u; i < gptr->FanOut; i++){
    if (node->children[i] != NULL){
      node->points[offset+i*2] =  node->children[i]->winner;
    }
  }
}

/*****************************************************************************
Description: Find the children and parents of the current node, and assign the
             position of the childrens' winner neuron to the current node.

Return value: The function does not return a value.
*****************************************************************************/
void UpdateChildrenAndParentLocation(struct Graph *gptr, struct Node *node)
{
  UNSIGNED i, offset;

  offset = gptr->ldim;
  for (i = 0u; i < gptr->FanOut; i++){
    if (node->children[i] != NULL){
      node->points[offset+i*2] =  node->children[i]->x;
      node->points[offset+i*2+1]= node->children[i]->y;
    }
  }
  offset = gptr->ldim + 2 * gptr->FanOut;
  for (i = 0u; i < node->numparents; i++){
    if (node->parents[i] != NULL){
      node->points[offset+i*2] =  node->parents[i]->x;
      node->points[offset+i*2+1]= node->parents[i]->y;
    }
  }
}

/*****************************************************************************
Description: Use with caution!! May be very slow

Return value: The quantization error.
*****************************************************************************/
/*
FLOAT ComputeStablePointOBSOLETE(struct Map *map, struct Graph *graph)
{
  UNSIGNED nnum, n, statechanges, oldstatechanges;
  FLOAT qerror;
  struct Node *node;
  struct Graph *gptr;
  struct Winner winner;
  void (*FindWinner)(struct Map *map, struct Node *node,struct Graph *gptr,struct Winner *winner);
  void (*UpdateStates)(struct Graph *gptr, struct Node *node);

  if (map == NULL && graph == NULL)
    return 0.0;

  if (map->topology == TOPOL_VQ){      
    fprintf(stderr, "Error: VQ in contextual mode not fully implemented");
    exit(0);
    FindWinner = VQFindWinnerEucledian; 
    UpdateStates = UpdateChildrensLocationVQ; 
  }
  else{
    FindWinner = FindWinnerEucledian;
    UpdateStates = UpdateChildrenAndParentLocation;
  }

  oldstatechanges = MAX_UNSIGNED;
  while (1){
    qerror = 0.0;
    statechanges = 0;
    n = 0;
    for (gptr = graph; gptr != NULL; gptr = gptr->next){
      for (nnum = 0; nnum < gptr->numnodes; nnum++){
	node = gptr->nodes[nnum];
	UpdateStates(gptr, node);
	FindWinner(map, node, gptr, &winner);
	
	if (map->topology == TOPOL_VQ)
	  node->winner = winner.codeno;
	else{
	  if (node->x != map->codes[winner.codeno].x){
	    node->x = map->codes[winner.codeno].x;
	    statechanges++;
	  }
	  if (node->y != map->codes[winner.codeno].y){
	    node->y = map->codes[winner.codeno].y;
	    statechanges++;
	  }
	}
	qerror += winner.diff;
	n++;
      }
    }
    if (statechanges == 0 || oldstatechanges <= statechanges)
      break;
    oldstatechanges = statechanges;
  }
  return qerror/n;
}
*/
/*****************************************************************************
Description: Compute the states of every node in the dataset using K-step
             approximation. 
             Use with caution!! May be very slow

Return value: The quantization error normalized with respect to the total
              number of nodes in the dataset.
*****************************************************************************/
FLOAT K_Step_Approximation(struct Map *map, struct Graph *graph, int mode)
{
  UNSIGNED nnum, n, nmax, iter, statechanges;
  FLOAT gpr_qerror, qerror;
  struct Node *node;
  struct Graph *gptr;
  struct Winner winner;
  void (*FindWinner)(struct Map *map, struct Node *node,struct Graph *gptr,struct Winner *winner);
  void (*UpdateStates)(struct Graph *gptr, struct Node *node);

  if (map == NULL && graph == NULL)
    return 0.0;

  if (map->topology == TOPOL_VQ){            /* In VQ mode...   */
    fprintf(stderr, "Error: VQ in contextual mode not fully implemented");
    exit(0);
    FindWinner = VQFindWinnerEucledian; /* Use Eucledian distance VQ mode  */
    UpdateStates = UpdateChildrensLocationVQ; /* use ID value    */
  }
  else if (mode == 1){
    FindWinner = FindWinnerEucledian;   /* Use standard Eucledian distance */
    UpdateStates = UpdateChildrenAndParentLocation;    /* Use coordinates */
  }
  else{
    FindWinner = FindWinnerEucledian;   /* Use standard Eucledian distance */
    UpdateStates = UpdateChildrensLocation;  /* Update neighbors only */
  }

  /* Compute the total number of nodes in given dataset, and the maximum
     number of nodes in a single graph */
  n = 0;
  nmax = 0;
  for (gptr = graph; gptr != NULL; gptr = gptr->next){
    n += gptr->numnodes;
    if (nmax < gptr->numnodes)
      nmax = gptr->numnodes;
  }

  qerror = 0.0;
  gpr_qerror = 0.0;
  int *xstates, *ystates;
  xstates = (int*)MyMalloc(nmax * sizeof(int));
  ystates = (int*)MyMalloc(nmax * sizeof(int));
  for (gptr = graph; gptr != NULL; gptr = gptr->next){

    iter = 0;
    for(iter = 0; iter <= gptr->depth; iter++){
      gpr_qerror = 0.0;
      statechanges = 0;

      /* Compute the state of every node in the graph */
      for (nnum = 0; nnum < gptr->numnodes; nnum++){
	node = gptr->nodes[nnum];
	UpdateStates(gptr, node);
	FindWinner(map, node, gptr, &winner);
	gpr_qerror += winner.diff;

	if (map->topology == TOPOL_VQ){
	  /* Inclomplete implementation for VQ mode: Need to update
	     node->winner in batch mode. Perhaps we can abuse xstates
	     for this task. */
	  node->winner = winner.codeno;
	}
	else{
	  xstates[nnum] = map->codes[winner.codeno].x;
	  ystates[nnum] = map->codes[winner.codeno].y;
	}
      }

      /* Update state of every node in the graph */
      for (nnum = 0; nnum < gptr->numnodes; nnum++){
	node = gptr->nodes[nnum];
	node->x = xstates[nnum];
	node->y = ystates[nnum];
      }
    }

    /* Update point-vector of every node */
    for (nnum = 0; nnum < gptr->numnodes; nnum++){
      node = gptr->nodes[nnum];
      UpdateStates(gptr, node);
    }
    qerror += gpr_qerror;
  }
  free(xstates);
  free(ystates);

  return qerror/n;
}


/*****************************************************************************
Description: 

Return value: The quantization error.
*****************************************************************************/
FLOAT GetNodeCoordinates(struct Map *map, struct Graph *gptr)
{
  UNSIGNED nnum, n;
  FLOAT qerror;
  struct Node *node;
  struct Winner winner;
  void (*FindWinner)(struct Map *map, struct Node *node,struct Graph *gptr,struct Winner *winner);
  void (*UpdateOffspringStates)(struct Graph *gptr, struct Node *node);

  if (map == NULL && gptr == NULL)
    return 0.0;

  n = 0;
  qerror = 0.0;
  if (map->topology == TOPOL_VQ){            /* In VQ mode...   */
    FindWinner = VQFindWinnerEucledian; /* Use Eucledian distance VQ mode  */
    UpdateOffspringStates = UpdateChildrensLocationVQ; /* use ID value    */
  }
  else{
    FindWinner = FindWinnerEucledian;   /* Use standart Eucledian distance */
    UpdateOffspringStates = UpdateChildrensLocation;    /* Use coordinates */
  }

  for (;gptr != NULL; gptr = gptr->next){
    for (nnum = 0; nnum < gptr->numnodes; nnum++){
      node = gptr->nodes[nnum];
      UpdateOffspringStates(gptr, node);
      FindWinner(map, node, gptr, &winner);

      if (map->topology == TOPOL_VQ)
	node->winner = winner.codeno;
      else{
	node->x = map->codes[winner.codeno].x;
	node->y = map->codes[winner.codeno].y;
      }
      qerror += winner.diff;
      n++;
    }
  }
  return qerror/n;
}




/*****************************************************************************
Description: Returns the number of offsprings a given node has.

Return value: Number of offsprings of given node which can be any value from
              0 to FanOut.
*****************************************************************************/
UNSIGNED GetNumChildren(struct Node *node, UNSIGNED FanOut)
{
  UNSIGNED i, num = 0;

  for (i = 0; i < FanOut; i++)
    if (node->children[i] != NULL)
      num++;

  return num;
}

/*****************************************************************************
Description: Check if a given node is a root node.

Return value: 1 if the node is a root node, 0 otherwise.
*****************************************************************************/
UNSIGNED IsRoot(struct Node *node)
{
  if (node->numparents == 0) /* Root nodes have no parent nodes */
    return 1;
  else
    return 0;                /* It is not a root node */
}

/*****************************************************************************
Description: Check if a given node is a leaf node.

Return value: 1 if the node is a leaf node, 0 otherwise.
*****************************************************************************/
UNSIGNED IsLeaf(struct Node *node)
{
  if (node->depth == 0)      /* Leaf nodes are located at depth 0 */
    return 1;
  else
    return 0;                /* It is not a leaf node */
}

/*****************************************************************************
Description: Check if a given node is a intermediate node (not leaf nor root).

Return value: 1 if the node is a intermediate node, 0 otherwise.
*****************************************************************************/
UNSIGNED IsIntermediate(struct Node *node)
{
  if (node->depth > 0 && node->numparents > 0) /* Intermediate node */
    return 1;
  else
    return 0;                      /* It is not a intermediate node */
}

/*****************************************************************************
Description: Returns the type of a given node which can be either ROOT, LEAF,
             or INTERMEDIATE.

Return value: Returns the constant value ROOT, LEAF, or INTERMEDIATE depending
              on whether the node is a root node, a leaf node, or neither leaf
              nor root.
*****************************************************************************/
UNSIGNED GetNodeType(struct Node *node)
{
  if (node->numparents == 0) /* Root nodes have no parent nodes */
    return ROOT;
  else if (node->depth == 0) /* Leaf nodes are of depth 0       */
    return LEAF;
  else
    return INTERMEDIATE;   /* Not root or leaf so it must be an intermediate */
}

/*****************************************************************************
Description: Computes the longest path from a given node to a leaf node.

Return value: The length of the longest path from the given node to the
              furthest leaf node.
*****************************************************************************/
UNSIGNED GetMaxPathLength(struct Node *node, UNSIGNED FanOut, int maxiter)
{
  UNSIGNED i, maxlen, len;

  /* If depth is already known, or we are at a leaf node */
  if (node->depth != 0 || GetNumChildren(node, FanOut) == 0)
    return node->depth;

  /* Graph appears to have a endless loop */
  if (--maxiter <= 0)
    return 0;

  maxlen = 0;
  for (i = 0; i < FanOut; i++){
    if (node->children[i] != NULL){
      len = GetMaxPathLength(node->children[i], FanOut, maxiter)+1;
      if (len > maxlen)
	maxlen = len;
    }
  }
  return maxlen;
}

/*****************************************************************************
Description: Set the depth value of all nodes in all graphs. This works well
             on small graphs. NOTE: This function will fail for very deep
             graph structures (~100000 nodes of more) due to a stack overflow.

Return value: The function does not return a value.
*****************************************************************************/
void SetNodeDepthRecursively(struct Graph *gptr)
{
  UNSIGNED n, max;
  struct Node *node;

  for (; gptr != NULL; gptr = gptr->next){
    if (gptr->numnodes <= 1) /* No nodes or only one node */
      continue;
    max = 0;
    for (n = 0u; n < gptr->numnodes; n++){
      node = gptr->nodes[n];
      node->depth = GetMaxPathLength(node, gptr->FanOut, gptr->numnodes);
      if (max < node->depth)
	max = node->depth;
    }
    gptr->depth = max;
  }
}

/*****************************************************************************
Description: Set the depth value of all nodes in all graphs using an iterative
             approach. This function works best on shallow graphs.

Return value: The function does not return a value.
*****************************************************************************/
void SetNodeDepthIteratively(struct Graph *gptr)
{
  UNSIGNED i, n, depth, max;
  UNSIGNED changes;
  struct Node *node;
  UNSIGNED *flags;

  for (; gptr != NULL; gptr = gptr->next){
    if (gptr->numnodes <= 1) /* No nodes or only one node */
      continue;

    flags = (UNSIGNED*)MyMalloc(gptr->numnodes * sizeof(UNSIGNED));
    memset(flags, 1, gptr->numnodes * sizeof(UNSIGNED));
    max = 0;
    do{
      changes = 0;
      for (n = 0u; n < gptr->numnodes; n++){
	if (flags[n] == 0)
	  continue;

	flags[n] = 0;
	node = gptr->nodes[n];
	if (GetNumChildren(node, gptr->FanOut)==0){ //Leaf nodes are of depth 0
	  node->depth = 0;
	  continue;
	}

	depth = 0;
	for (i = 0; i < gptr->FanOut; i++){
	  if (node->children[i] != NULL){
	    if (node->children[i]->depth > depth)
	      depth = node->children[i]->depth;
	  }
	}
	if (node->depth != depth + 1){
	  node->depth = depth + 1;
	  max = depth + 1;
	  flags[n] = 1;
	  changes++;
	}
      }
    }while(changes > 0);
    free(flags);
    gptr->depth = max;
  }
}

/*****************************************************************************
Description: Set the depth value of all nodes in all graphs by using a reverse
             breadth first approach. This works best on large narrow graphs.

Return value: The function does not return a value.
*****************************************************************************/
void SetNodeDepth_UsingReverseTrace(struct Graph *gptr)
{
  UNSIGNED n, p, nlevel, num, newnum;
  UNSIGNED depth, newlevelsize;
  struct Node **onlevel, **newlevel;

  for (; gptr != NULL; gptr = gptr->next){
    gptr->depth = 0;  /* Initialize graph's depth with a known value */
    num = 0;
    for (n = 0u; n < gptr->numnodes; n++){
      if (GetNumChildren(gptr->nodes[n], gptr->FanOut)==0){
	num++;        /* Count number of leafs in a graph */
      }
    }
    onlevel = (struct Node **)MyMalloc(num * sizeof(struct Node *));
    nlevel = num;
    num = 0;
    for (n = 0u; n < gptr->numnodes; n++){
      if (GetNumChildren(gptr->nodes[n], gptr->FanOut)==0){
	onlevel[num++] = gptr->nodes[n];
      }
    }

    /* Starting from leaf nodes, traverse graph layer by layer towards root */
    depth = 1;
    newlevel = NULL;
    while(num > 0){
      newnum = 0;
      newlevelsize = 16;
      newlevel = (struct Node **)MyMalloc(16 * sizeof(struct Node *));
      for (n = 0; n < num; n++){
	for (p = 0; p < onlevel[n]->numparents; p++){
	  onlevel[n]->parents[p]->depth = depth;
	  if (newlevelsize <= newnum){
	    newlevelsize += 16;
	    newlevel = (struct Node **)MyRealloc(newlevel, newlevelsize * sizeof(struct Node *));
	  }
	  newlevel[newnum] = onlevel[n]->parents[p];
	  newnum++;
	}
      }
      num = newnum;
      free(onlevel);
      onlevel = newlevel;
      depth++;
    }
    free(onlevel);
    gptr->depth = depth - 1;
  }
}

/*****************************************************************************
Description: Set the depth value of all nodes in all graphs.

Return value: The function does not return a value.
*****************************************************************************/
void SetNodeDepth(struct Graph *graph)
{
  UNSIGNED maxFanOut, maxNumNodes;
  struct Graph *gptr;

  if (graph == NULL) /* Sanity check */
    return;

  /* Determine some dataset properties... */
  maxFanOut = 0;
  maxNumNodes = 0;
  for (gptr = graph; gptr != NULL; gptr = gptr->next){
    if (gptr->FanOut > maxFanOut)
      maxFanOut = gptr->FanOut;
    if (gptr->numnodes > maxNumNodes)
      maxNumNodes = gptr->numnodes;
  }

  /* ...chose fastest routine for the task */
  if (maxNumNodes < 1000)
    SetNodeDepthRecursively(graph); /* Fastest but requires lots of memory */
  else if (maxFanOut < 5)
    SetNodeDepth_UsingReverseTrace(graph); /* Fast if FanOut is small   */
  else
    SetNodeDepthIteratively(graph); /* Slowest but needs little memory  */
}

/*****************************************************************************
Description: Increase the size of a given vector component for every node in
             a given graph.

Return value: The function does not return a value.
*****************************************************************************/
void IncreaseDimension(struct Graph *gptr, int newdim, int component)
{
  UNSIGNED n, dimension, dimincrease;

  /* Compute the actual increase of the vector dimension */
  dimincrease = 0;
  switch (component){
  case DATALABEL:
    dimincrease = newdim - gptr->ldim;
    break;
  case CHILDSTATES:
    dimincrease = 2 * (newdim - gptr->FanOut);
    break;
  case PARENTSTATES:
    dimincrease = 2 * (newdim - gptr->FanIn);
    break;
  case TARGETS:
    dimincrease = newdim - gptr->tdim;
    break;
  }
  if (dimincrease <= 0)  /* Leave unchanged if dimension is not increasing */
    return;

  fprintf(stderr, "WARNING: Increasing vector dimension without increasing dimension of codebook vectors. Implementation is incomplete!\n");

  /* Compute old vector dimension */
  dimension = gptr->ldim + 2*(gptr->FanOut + gptr->FanIn) + gptr->tdim;

  for (n = 0; n < gptr->numnodes; n++){
    gptr->nodes[n]->points = (FLOAT*)MyRealloc(gptr->nodes[n]->points, (dimension + dimincrease) * sizeof(FLOAT));

    /* Move old vector components back into the right place, and initialize
       newly allocated space with zero values. */
    switch (component){
    case DATALABEL:
      memmove(&gptr->nodes[n]->points[gptr->ldim+dimincrease], &gptr->nodes[n]->points[gptr->ldim], (2*(gptr->FanOut+gptr->FanIn)+gptr->tdim) * sizeof(FLOAT));
      memset(&gptr->nodes[n]->points[gptr->ldim], 0, dimincrease * sizeof(FLOAT));
      break;
    case CHILDSTATES:
      memmove(&gptr->nodes[n]->points[gptr->ldim+2*gptr->FanOut+dimincrease], &gptr->nodes[n]->points[gptr->ldim+2*gptr->FanOut], (2*gptr->FanIn+gptr->tdim) * sizeof(FLOAT));
      memset(&gptr->nodes[n]->points[gptr->ldim+2*gptr->FanOut], 0, dimincrease * sizeof(FLOAT));
      break;
    case PARENTSTATES:
      memmove(&gptr->nodes[n]->points[gptr->ldim+2*(gptr->FanOut+gptr->FanIn)+dimincrease], &gptr->nodes[n]->points[gptr->ldim+2*(gptr->FanOut+gptr->FanIn)], gptr->tdim * sizeof(FLOAT));
      memset(&gptr->nodes[n]->points[gptr->ldim+2*(gptr->FanOut+gptr->FanIn)], 0, dimincrease * sizeof(FLOAT));
      break;
    case TARGETS:
      /* nothing to be moved here. */
      memset(&gptr->nodes[n]->points[gptr->ldim+2*(gptr->FanOut+gptr->FanIn)+gptr->tdim], 0, dimincrease * sizeof(FLOAT));
      break;
    }
  }
}

/*****************************************************************************
Description: Perform padding on all nodes to ensure consistent size.

Return value: The function does not return a value.
*****************************************************************************/
void Padding(struct Parameters param)
{
  UNSIGNED mindim, maxdim;
  struct Graph *gptr;
  struct Graph *graphs;

  graphs = param.train;
  mindim = MAX_UNSIGNED;
  maxdim = 0;
  for (gptr = graphs; gptr != NULL; gptr = gptr->next){
    if (mindim > gptr->dimension)
      mindim = gptr->dimension;
    if (maxdim < gptr->dimension)
      maxdim = gptr->dimension;
  }
  if (mindim == maxdim)
    return; /* No padding required */

  /* Check if data label component requires padding */
  mindim = MAX_UNSIGNED;
  maxdim = 0;
  for (gptr = graphs; gptr != NULL; gptr = gptr->next){
    if (mindim > gptr->ldim)
      mindim = gptr->ldim;
    if (maxdim < gptr->ldim)
      maxdim = gptr->ldim;
  }
  if (mindim != maxdim){
    fprintf(stderr, "\nData label component requires padding.\n");
  }

  /* Check if child state vector requires padding */
  mindim = MAX_UNSIGNED;
  maxdim = 0;
  for (gptr = graphs; gptr != NULL; gptr = gptr->next){
    if (mindim > gptr->FanOut)
      mindim = gptr->FanOut;
    if (maxdim < gptr->FanOut)
      maxdim = gptr->FanOut;
  }
  if (mindim != maxdim){
    fprintf(stderr, "\nData child state vector requires padding.\n");
  }

  /* Check if parent state vector requires padding */
  mindim = MAX_UNSIGNED;
  maxdim = 0;
  for (gptr = graphs; gptr != NULL; gptr = gptr->next){
    if (mindim > gptr->FanIn)
      mindim = gptr->FanIn;
    if (maxdim < gptr->FanIn)
      maxdim = gptr->FanIn;
  }
  if (mindim != maxdim){
    fprintf(stderr, "\nData parent state vector requires padding.\n");
  }

  /* Check if target label component requires padding */
  mindim = MAX_UNSIGNED;
  maxdim = 0;
  for (gptr = graphs; gptr != NULL; gptr = gptr->next){
    if (mindim > gptr->tdim)
      mindim = gptr->tdim;
    if (maxdim < gptr->tdim)
      maxdim = gptr->tdim;
  }
  if (mindim != maxdim){
    fprintf(stderr, "\nData target vector requires padding.\n");
  }

  fprintf(stderr, "Padding of training/test/validation data is required but not implemented in module data.c, function Padding(). This may cause a segmentation fault, or may produce wrong or unexpected results.\n");
}

/******************************************************************************
Description: Temporary function until undirected graph file format is supported

Return value: This function does not return any value.
******************************************************************************/
void ConvertToUndirectedLinks(struct Graph *train)
{
  struct Graph *gptr;
  struct Node *node, *child;
  int nnum, childno, cchildno;
  int maxFanIn;

  fprintf(stderr, "Converting all links in dataset to undirected links.");
  for (gptr = train; gptr != NULL; gptr = gptr->next){
    maxFanIn = 0;
    for (nnum = 0; nnum < gptr->numnodes; nnum++){
      node = gptr->nodes[nnum];
      for (childno = 0; childno < gptr->FanOut; childno++){
	child = node->neighbors[childno];
	if (child == NULL)
	  continue;

	/* Add node to list of child's children */
	for (cchildno = 0; cchildno < gptr->FanOut; cchildno++){
	  if (child->neighbors[cchildno] == node)
	    break;
	  else if (child->neighbors[cchildno] == NULL){
	    child->neighbors[cchildno] = node;
	    break;
	  }
	}
	if (cchildno >= gptr->FanOut){
	  fprintf(stderr, "\nFanOut too small. Cannot convert to undirected links.\n");
	  exit(0);
	}

	/* Add node's child to list of parents */
	//	node->numparents += 1;  /* add node to list of parents */
	//	node->parents = MyRealloc(node->parents, node->numparents * sizeof(struct Node*));
	//	node->parents[node->numparents-1] = child;
	//	if (node->numparents > maxFanIn)
	//	  maxFanIn = node->numparents;
      }
    }
    /* This is only good if we were to do contextual stuff
    if (maxFanIn > gptr->FanIn){
      IncreaseDimension(gptr, maxFanIn, PARENTSTATES);
      gptr->FanIn = maxFanIn;
    }
    */
  }
  fprintf(stderr, "%22s\n", "[OK]");
}

/*****************************************************************************
Description: Initializes nodes with vector weight values.

Return value: The function does not return a value.
*****************************************************************************/
void SetWeightValues(FLOAT mu1, FLOAT mu2, FLOAT mu3, FLOAT mu4, struct Graph *gptr)
{
  UNSIGNED i, j;

  for (; gptr != NULL; gptr = gptr->next){
    for (i = 0u; i < gptr->numnodes; i++){
      if (gptr->nodes[i]->mu == NULL)
	gptr->nodes[i]->mu = MyMalloc(gptr->dimension * sizeof(FLOAT));

      for (j = 0; j < gptr->ldim; j++)
	gptr->nodes[i]->mu[j] = mu1;

      for (; j < gptr->ldim + 2*gptr->FanOut; j++)
	gptr->nodes[i]->mu[j] = mu2;

      for (; j < gptr->ldim + 2*gptr->FanOut + 2*gptr->FanIn; j++)
	gptr->nodes[i]->mu[j] = mu3;

      for (; j < gptr->dimension; j++)
	gptr->nodes[i]->mu[j] = mu4;
    }
  }
}

/*****************************************************************************
Description: Auxilary function used by qsort to sort nodes according to the
             node's depth. 

Return value: The function return -1,+1, or 0 if the depth of the dirt node is
              greater than, smaller than, or equal to the depth of a given 
              second node.
*****************************************************************************/
int CompareDepth(const void *node1, const void *node2)
{
  if ((*(struct Node**)node1)->depth > (*(struct Node**)node2)->depth)
    return 1;
  else if ((*(struct Node**)node1)->depth < (*(struct Node**)node2)->depth)
    return -1;
  else
    return 0;
}

/*****************************************************************************
Description: Auxilary function used by qsort to obtain randomization of items
             in an array. Abusing qsort for this task works well for 
             relatively small arrays (i.e. arrays with less than 100000
             elements).

Return value: The function returns a uniformly distributed signed integers.
*****************************************************************************/
int Random(const void *node1, const void *node2)
{
  return (int)mrand48();
}

/*****************************************************************************
Description: Sort the nodes in each graph accoring to the depth values in
             ascending order.

Return value: The function does not return a value.
*****************************************************************************/
void SortNodesByDepth(struct Graph *gptr)
{
  for(; gptr != NULL; gptr = gptr->next)
    qsort(gptr->nodes, gptr->numnodes, sizeof(struct Node*), CompareDepth);
}

/*****************************************************************************
Description: Randomizes the occurence of nodes in each graph. The connectivity
             remains unaffected. Hence, this randomization merely affects how
             nodes are processed during training (i.e. in a random order rather
             than in a bottom-up order as achieved by calling the function
             SortNodesByDepth().

Return value: The function does not return a value.
*****************************************************************************/
void RandomizeNodeOrder(struct Graph *gptr)
{
  for(; gptr != NULL; gptr = gptr->next)
    qsort(gptr->nodes, gptr->numnodes, sizeof(struct Node*), Random);
}

/*****************************************************************************
Description: Randomizes the sequence of a given list of graphs.

Return value: Pointer to the new first element of the list of graphs.
*****************************************************************************/
struct Graph *RandomizeGraphOrder(struct Graph *graph)
{
  unsigned rval;
  int i, num;
  struct Graph **garray, *gptr;

  /* Sanity check */
  if (graph == NULL)
    return graph;

  /* Initialize an auxilliary array for list elements */
  for(gptr = graph, num = 0; gptr != NULL; gptr = gptr->next, num++);
  garray = (struct Graph **)MyMalloc(num * sizeof(struct Graph *));
  for(gptr = graph, num = 0; gptr != NULL; gptr = gptr->next, num++)
    garray[num] = gptr;

  /* Randomize order of elements in the array */
  for (i = 0; i < num; i++){
    rval = (unsigned)(drand48() * num / 1.0);
    gptr = garray[i];
    garray[i] = garray[rval];
    garray[rval] = gptr;
  }
  graph = garray[0];

  /* (Re-)link list elements */
  for (i = 0; i < num-1; i++)
    garray[i]->next = garray[i+1];
  garray[i]->next = NULL;

  free(garray); /* Free memory used by auxillary array */
  return graph;
}

/*****************************************************************************
Description: This function is called in VQ mode, it allocates memory to hold
             the index number of winner neurons.

Return value: The function does not return a value.
*****************************************************************************/
void VQInitWinner(struct Graph *graph)
{
  struct Graph *gptr;
  UNSIGNED n;

  for (gptr = graph; gptr != NULL; gptr = gptr->next){
    for (n = 0; n < gptr->numnodes; n++){
      gptr->nodes[n]->winner = -1; /* Initialize with illegal ID value */
    }
  }
}

/*****************************************************************************
Description: This function ensures that all nodes in the dataset are fully
             initialized. It also performs padding if nodes are of different
             dimension, or if the requested training mode requires padding.

Return value: The function does not return a value.
*****************************************************************************/
void PrepareData(struct Parameters *param)
{
  int i;
  struct Graph *GList[3];

  /* Datasets will be prepared in the following order */
  GList[0] = param->train;
  GList[1] = param->valid;
  GList[2] = param->test;

  Padding(*param); /* Ensure that all nodes are of the same dimension */
  for (i = 0; i < 3; i++){
    if (GList[i] == NULL)
      continue;

    SetWeightValues(param->mu1, param->mu2, param->mu3, param->mu4, GList[i]);

    if (param->nodeorder == 1)
      RandomizeNodeOrder(GList[i]); /* Randomize the order of nodes */
    else
      SortNodesByDepth(GList[i]);   /* Ensure bottom up processing of nodes */

    if (param->graphorder == 1)
      param->train = RandomizeGraphOrder(GList[i]);

    if (param->map.topology == TOPOL_VQ)              /* In VQ mode only... */
      VQInitWinner(GList[i]);
  }
}

/*****************************************************************************
Description: Free all memory allocated to the list of graphs starting from
             the pointer graph.

Return value: The function does not return a value.
*****************************************************************************/
void FreeGraphs(struct Graph *graph)
{
  struct Graph *gptr, *prev;
  struct Node *node;
  UNSIGNED n;

  for (gptr = graph; gptr != NULL; ){
    if (gptr->gname != NULL)
      free(gptr->gname);
    if (gptr->nodes != NULL){
      for (n = 0; n < gptr->numnodes; n++){
	node = gptr->nodes[n];
	if (node == NULL)
	  continue;
	if (node->points != NULL)
	  free(node->points);
	if (node->parents != NULL)
	  free(node->parents);
	if (node->children != NULL)
	  free(node->children);
	free(node);
      }
      free(gptr->nodes);
    }
    prev = gptr;
    gptr = gptr->next;
    memset(prev, 0, sizeof(struct Graph));  /* Reset the graph */
    free(prev);
  }
}

/*****************************************************************************
Description: Free all memory allocated by the map.

Return value: The function does not return a value.
*****************************************************************************/
void FreeMap(struct Map *map)
{
  UNSIGNED i, noc;

  if (map->codes != NULL){
    noc = map->xdim * map->ydim;
    for (i = 0; i < noc; i++)
      if (map->codes[i].points != NULL)
	free(map->codes[i].points);
  }

  memset(map, 0, sizeof(struct Map));  /* Reset the map */
}

/******************************************************************************
Description: Free the memory allocated for the Parameters structure

Return value: This function does not return a value.
******************************************************************************/
void ClearParameters(struct Parameters* parameters)
{
  if (parameters == NULL)
    return;

  if (parameters->inetfile)
    free(parameters->inetfile);
  if (parameters->datafile)
    free(parameters->datafile);
  if (parameters->validfile)
    free(parameters->validfile);
  if (parameters->onetfile)
    free(parameters->onetfile);
  if (parameters->snap.command)
    free(parameters->snap.command);
  if (parameters->snap.file)
    free(parameters->snap.file);

  FreeGraphs(parameters->train);
  FreeGraphs(parameters->valid);
  FreeGraphs(parameters->test);
  FreeMap(&parameters->map);

  memset(parameters, 0, sizeof(struct Parameters));
}

/******************************************************************************
Description: Free all dynamically allocated memory, and flush error messages.

Return value: This function does not return a value.
******************************************************************************/
void Cleanup(struct Parameters* parameters)
{
  ClearParameters(parameters); /* Free memory used by the parameter struct */
  ClearLabels();               /* Clear all data labels */
  PrintErrors();               /* Flush all error messages */
}

/* End of file */
