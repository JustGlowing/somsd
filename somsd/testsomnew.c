/*
  THIS IS AN UNTESTED DEVELOPMENT VERSION!
  NOT FOR DISTRIBUTION!

  Contents: The main module used to evaluate a somsd

  Author: Markus Hagenbuchner

  Comments and questions concerning this program package may be sent
  to 'markus@artificial-neural.net'
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

/*************/
/* Constants */
/*************/
#define CONTEXT        0x00000001
#define CONTEXTUAL     0x00000002
#define MAPNODETYPE    0x00000004
#define MAPNODELABEL   0x00000008
#define MAPGRAPH       0x00000010
#define SHOWGRAPH      0x00000020
#define SHOWCLUSTERING 0x00000040
#define SHOWSUBGRAPHS  0x00000080
#define PRECISION      0x00000100
#define RETRIEVALPERF  0x00000200
#define CLASSIFY       0x00000400
#define ANALYSE        0x00000800
#define BALANCE        0x00001000
#define DISTANCES      0x00002000
#define WEBSOM         0x00004000
#define TRUNCATE       0x00008000

struct VMap{
  UNSIGNED max;
  UNSIGNED **activation;
  UNSIGNED numclasses;
  UNSIGNED ***classes;
  UNSIGNED **winnerclass;
};

struct AllHits{
  struct Graph *graph;
  struct Node *node;
  char *structID;
  char *substructID;
  struct AllHits *next;
};

int KstepEnabled = 0;

/* Begin functions... */

/******************************************************************************
Description: Print usage information to the screen

Return value: The function does not return.
******************************************************************************/
void Usage()
{
  fprintf(stderr, "\n\
Usage: testsom [options]\n\n\
Options are:\n\
    -cin <fname>        Codebook file\n\
    -din <fname>        The file which holds the training data set.\n\
    -tin <fname>        The file which holds the test data set.\n\
    -mode <mode>        Test mode, which can be:\n\
                        context   Writes a data set suitable for further\n\
                                  contextual processing (multi-layer contexttual mode).\n\
                        contextual Computes stable point of dataset, then writes\n\
                                  the result to stdout (single layer contextual mode).\n\
                        mapgraph  For each codebook, plot (at most) one graph\n\
                                  structure that was mapped at that location.\n\
                        mapcluster Same as mapgraph, but plot class membership as well.\n\
                        showgraph Visualize a graph at a given location\n\
                                  location info is read from stdin.\n\
                        showsubgraph Visualize a given graph and all its\n\
                                  subgraphs. info is read from stdin.\n\
                        maplabel  For each codebook, list the node labels of\n\
                                  nodes that activated the codebook.\n\
                        precision Compute the mapping precision of the map.\n\
                        retrievalperformance Compute the retrieval\n\
                                  performance.\n\
                        classify  Classify a given set of data. A labelled\n\
                                  training set needs to be available.\n\
                        analyse   Statistically analyse a given dataset.\n\
                        balance   Produce a balanced dataset\n\
                        truncate <n>  Truncate outdegree of graphs in a given\n\
                                  dataset to at most n.\n\
                        distances List for every node in the dataset distances\n\
                                  to every codebook in the map.\n\
    -mu1 float[:float]  Weight(range) for the label component.\n\
    -mu2 float[:float]  Weight(range) for the child state component.\n\
    -mu3 float[:float]  Weight(range) for the parents position component.\n\
    -mu4 float[:float]  Weight(range) for the class label component.\n\
    -quiet              Restrict amount of text printed to screen.\n\
    -help               Print this help.\n\
 \n");
  exit(0);
}

/*****************************************************************************
Description: Print an xfig header

Return value:
*****************************************************************************/
void PrintXfigHeader(FILE *ofile)
{
  time_t t;

  t = clock();
  fprintf(ofile, "#FIG 3.2\n");
  fprintf(ofile, "Landscape\n");
  fprintf(ofile, "Center\n");
  fprintf(ofile, "Inches\n");
  fprintf(ofile, "Letter\n");
  fprintf(ofile, "100.00\n");
  fprintf(ofile, "Single\n");
  fprintf(ofile, "-2\n");
  fprintf(ofile, "1200 2\n");
  fprintf(ofile, "0 32 #bebabe\n");

  fprintf(ofile, "0 33 #f6f6f6\n");
  fprintf(ofile, "0 34 #f0f0f0\n");
  fprintf(ofile, "0 35 #e6e6e6\n");
  fprintf(ofile, "0 36 #e0e0e0\n");
  fprintf(ofile, "0 37 #d6d6d6\n");
  fprintf(ofile, "0 38 #d0d0d0\n");
  fprintf(ofile, "0 39 #c6c6c6\n");
  fprintf(ofile, "0 40 #c0c0c0\n");
  fprintf(ofile, "0 41 #b6b6b6\n");
  fprintf(ofile, "0 42 #b0b0b0\n");
  fprintf(ofile, "0 43 #a6a6a6\n");
  fprintf(ofile, "0 44 #a0a0a0\n");
  fprintf(ofile, "0 45 #969696\n");
  fprintf(ofile, "0 46 #909090\n");
}

void DrawPattern(int pattern, int hoff, int voff, int w, int h)
{
  pattern = 15;

  switch (pattern){
  case 0:
    printf("2 1 0 1 0 7 50 -1 -1 0.000 0 0 -1 0 0 2\n");
    printf("\t%d %d %d %d\n", hoff+w/3, voff-h/2, hoff+2*w/3, voff+h/2);
    break;
  case 1:
    printf("2 1 0 1 0 7 50 -1 -1 0.000 0 0 -1 0 0 2\n");
    printf("\t%d %d %d %d\n", hoff+2*w/3, voff-h/2, hoff+w/3, voff+h/2);
    break;
  case 2:
    printf("2 1 0 1 0 7 50 -1 -1 0.000 0 0 -1 0 0 2\n");
    printf("\t%d %d %d %d\n", hoff+w/3, voff-h/2, hoff+2*w/3, voff+h/2);
    printf("2 1 0 1 0 7 50 -1 -1 0.000 0 0 -1 0 0 2\n");
    printf("\t%d %d %d %d\n", hoff+2*w/3, voff-h/2, hoff+w/3, voff+h/2);
    break;
  case 3:
    printf("2 1 0 1 0 7 50 -1 -1 0.000 0 0 -1 0 0 2\n");
    printf("\t%d %d %d %d\n", hoff+w/2, voff-h/2, hoff+w/2, voff+h/2);
    break;
  case 4:
    printf("2 1 0 1 0 7 50 -1 -1 0.000 0 0 -1 0 0 2\n");
    printf("\t%d %d %d %d\n", hoff, voff, hoff+w, voff);
    break;
  case 5:
    printf("2 1 0 1 0 7 50 -1 -1 0.000 0 0 -1 0 0 2\n");
    printf("\t%d %d %d %d\n", hoff+w/2, voff-h/2, hoff+w/2, voff+h/2);
    printf("2 1 0 1 0 7 50 -1 -1 0.000 0 0 -1 0 0 2\n");
    printf("\t%d %d %d %d\n", hoff, voff, hoff+w, voff);
    break;
  case 6:
    printf("2 1 0 1 0 7 50 -1 -1 0.000 0 0 -1 0 0 2\n");
    printf("\t%d %d %d %d\n", hoff+w/3, voff-h/2, hoff+2*w/3, voff+h/2);
    printf("2 1 0 1 0 7 50 -1 -1 0.000 0 0 -1 0 0 2\n");
    printf("\t%d %d %d %d\n", hoff+2*w/3, voff-h/2, hoff+w/3, voff+h/2);
    printf("2 1 0 1 0 7 50 -1 -1 0.000 0 0 -1 0 0 2\n");
    printf("\t%d %d %d %d\n", hoff+w/2, voff-h/2, hoff+w/2, voff+h/2);
    printf("2 1 0 1 0 7 50 -1 -1 0.000 0 0 -1 0 0 2\n");
    printf("\t%d %d %d %d\n", hoff, voff, hoff+w, voff);
    break;
  case 7:
    printf("2 1 0 1 0 7 50 -1 -1 0.000 0 0 -1 0 0 2\n");
    printf("\t%d %d %d %d\n", hoff+w/3, voff-h/2, hoff+w/3, voff+h/2);
    printf("2 1 0 1 0 7 50 -1 -1 0.000 0 0 -1 0 0 2\n");
    printf("\t%d %d %d %d\n", hoff+w/2, voff-h/2, hoff+w/2, voff+h/2);
    printf("2 1 0 1 0 7 50 -1 -1 0.000 0 0 -1 0 0 2\n");
    printf("\t%d %d %d %d\n", hoff+2*w/3, voff-h/2, hoff+2*w/3, voff+h/2);
    break;
  case 8:
    printf("2 1 0 1 0 7 50 -1 -1 0.000 0 0 -1 0 0 4\n");
    printf("\t%d %d %d %d %d %d %d %d\n", hoff, voff, hoff+2*w/3, voff-h/2, hoff+2*w/3, voff+h/2, hoff, voff);
    break;
  case 9:
    printf("2 1 0 1 0 7 50 -1 -1 0.000 0 0 -1 0 0 4\n");
    printf("\t%d %d %d %d %d %d %d %d\n", hoff+w/3, voff-h/2, hoff+w, voff, hoff+w/3, voff+h/2, hoff+w/3, voff-h/2);
    break;
  case 10:
    printf("2 1 0 1 0 7 50 -1 -1 0.000 0 0 -1 0 0 2\n");
    printf("\t%d %d %d %d\n", hoff+w/3, voff-h/2, hoff+w/2, voff);
    printf("2 1 0 1 0 7 50 -1 -1 0.000 0 0 -1 0 0 2\n");
    printf("\t%d %d %d %d\n", hoff+w/3, voff+h/2, hoff+w/2, voff);
    printf("2 1 0 1 0 7 50 -1 -1 0.000 0 0 -1 0 0 2\n");
    printf("\t%d %d %d %d\n", hoff+w/2, voff, hoff+w, voff);
    break;
  case 11:
    printf("2 1 0 1 0 7 50 -1 -1 0.000 0 0 -1 0 0 2\n");
    printf("\t%d %d %d %d\n", hoff, voff, hoff+w/2, voff);
    printf("2 1 0 1 0 7 50 -1 -1 0.000 0 0 -1 0 0 2\n");
    printf("\t%d %d %d %d\n", hoff+w/2, voff, hoff+2*w/3, voff-h/2);
    printf("2 1 0 1 0 7 50 -1 -1 0.000 0 0 -1 0 0 2\n");
    printf("\t%d %d %d %d\n", hoff+w/2, voff, hoff+2*w/3, voff+h/2);   
    break;
  case 12:
    printf("2 1 0 1 0 7 50 -1 -1 0.000 0 0 -1 0 0 2\n");
    printf("\t%d %d %d %d\n", hoff+w/3, voff-h/2, hoff+w/2, voff);
    printf("2 1 0 1 0 7 50 -1 -1 0.000 0 0 -1 0 0 2\n");
    printf("\t%d %d %d %d\n", hoff+2*w/3, voff-h/2, hoff+w/2, voff);
    printf("2 1 0 1 0 7 50 -1 -1 0.000 0 0 -1 0 0 2\n");
    printf("\t%d %d %d %d\n", hoff+w/2, voff, hoff+w/2, voff+h/2);   
    break;
  case 13:
    printf("2 1 0 1 0 7 50 -1 -1 0.000 0 0 -1 0 0 2\n");
    printf("\t%d %d %d %d\n", hoff+w/3, voff+h/2, hoff+w/2, voff);
    printf("2 1 0 1 0 7 50 -1 -1 0.000 0 0 -1 0 0 2\n");
    printf("\t%d %d %d %d\n", hoff+2*w/3, voff+h/2, hoff+w/2, voff);
    printf("2 1 0 1 0 7 50 -1 -1 0.000 0 0 -1 0 0 2\n");
    printf("\t%d %d %d %d\n", hoff+w/2, voff, hoff+w/2, voff-h/2);   
    break;
  case 14:
    printf("2 1 0 1 0 7 50 -1 -1 0.000 0 0 -1 0 0 2\n");
    printf("\t%d %d %d %d\n", hoff, voff, hoff+w/2, voff);
    printf("2 1 0 1 0 7 50 -1 -1 0.000 0 0 -1 0 0 2\n");
    printf("\t%d %d %d %d\n", hoff, voff, hoff+2*w/3, voff-h/2);
    printf("2 1 0 1 0 7 50 -1 -1 0.000 0 0 -1 0 0 2\n");
    printf("\t%d %d %d %d\n", hoff, voff, hoff+2*w/3, voff+h/2);   
    break;
  case 15:
    printf("2 1 0 1 0 7 50 -1 -1 0.000 0 0 -1 0 0 2\n");
    printf("\t%d %d %d %d\n", hoff+w/3, voff-h/2, hoff+w, voff);
    printf("2 1 0 1 0 7 50 -1 -1 0.000 0 0 -1 0 0 2\n");
    printf("\t%d %d %d %d\n", hoff+w/3, voff+h/2, hoff+w, voff);
    printf("2 1 0 1 0 7 50 -1 -1 0.000 0 0 -1 0 0 2\n");
    printf("\t%d %d %d %d\n", hoff+w/2, voff, hoff+w, voff);
    break;
  }
}

/*****************************************************************************
Description: Draw the neurons of a map as .fig format

Return value:
*****************************************************************************/
void DrawMap(struct Map map, FLOAT scale, struct VMap *vmap)
{
  int w, h;       /* Width and height of a neuron */
  int hoff, voff; /* horizontal and vertical offsets */
  int x, y;       /* count through width and height of map */
  FLOAT cval;

  w = scale*1.3;
  h = scale*1.3;
  for (y = 0; y < map.ydim; y++){
    for (x = 0; x < map.xdim; x++){
      hoff = x*2*w/3 ;
      if (x % 2)
	voff = y*h-h/2;
      else
	voff = y*h;
      if (vmap != NULL){
	if (vmap->activation[y][x] == 0)
	  printf("2 1 0 2 32 7 50 -1 -1 0.000 0 0 -1 0 0 7\n");
	else{
	  cval = 46 - (vmap->max - vmap->activation[y][x])*13/vmap->max;
	  printf("2 1 0 2 32 %d 51 -1 20 0.000 0 0 -1 0 0 7\n", (int)cval);
	}
      }
      else
	printf("2 1 0 2 32 7 50 -1 -1 0.000 0 0 -1 0 0 7\n");

      printf("\t%d %d ", hoff, voff);
      printf("%d %d ", hoff+w/3, voff-h/2);
      printf("%d %d ", hoff+2*w/3, voff-h/2);
      printf("%d %d ", hoff+w, voff);
      printf("%d %d ", hoff+2*w/3, voff+h/2);
      printf("%d %d ", hoff+w/3, voff+h/2);
      printf("%d %d\n", hoff, voff);

      if (vmap->winnerclass && vmap->winnerclass[y][x] > 0)
	DrawPattern(vmap->winnerclass[y][x]-1, hoff, voff, w, h);
    }
  }
}

int GetGraphWidth(struct Graph* gptr)
{
  int n, max, maxdepth;
  int *stats;

  if (gptr == NULL)
    return 0;

  maxdepth = 0;
  for (n = 0; n < gptr->numnodes; n++){
    if (maxdepth < gptr->nodes[n]->depth)
      maxdepth = gptr->nodes[n]->depth;
  }
  stats = (int*)MyCalloc((maxdepth+1), sizeof(int));

  for (n = 0; n < gptr->numnodes; n++)
    stats[gptr->nodes[n]->depth]++;

  max = 0;
  for (n = 0; n < maxdepth+1; n++){
    if (max < stats[n])
      max = stats[n];
  }
  free(stats);
  return max;
}

void DrawXfigGraphAt(FILE *ofile, struct Graph* gptr, struct Node *root, int x, int y, float scale)
{
  struct Helper{
    int x, y;
    struct Node *node;
  };
//  static int offset = 11;
  int width, height;
  int d, cd, flag;
  int n, i, c, cn, j, wo, num;
  //  char buffer[1024];
  struct Helper **gmatrix, *xynode, *child;
  struct Node *node;
  //  int fontsize = 8;

  if (ofile == NULL || gptr == NULL)
    return;

  //Initializations
  width = GetGraphWidth(gptr);
  height = root->depth+1; //GetGraphDepth(gptr)+1;

  gmatrix = (struct Helper**)MyMalloc(height * sizeof(struct Helper*));
  for (i = 0; i < height; i++)
    gmatrix[i] = (struct Helper*)MyCalloc((width+1), sizeof(struct Helper));

  gmatrix[root->depth][0].node = root;
  for (d = root->depth; d > 0; d--){
    c = 0;
    for (i = 0; i < width; i++){
      if (gmatrix[d][i].node == NULL)
	continue;

      for (j = 0; j < gptr->FanOut; j++){
	if (gmatrix[d][i].node->children[j] != NULL){
	  gmatrix[d-1][c].node = gmatrix[d][i].node->children[j];
	  c++;
	}
      }
    }
  }
  for (d = 0; d < height; d++){
    num = 0;
    for (c = 0; c < width; c++){
      if (gmatrix[d][c].node != NULL)
	num++;
      gmatrix[d][c].x = scale+x+c*scale*3;
      gmatrix[d][c].y = y+(height-d)*scale*3;
    }
    wo = (3*(width-1-num)*scale)/2;
    for (c = 0; c < width; c++){
      gmatrix[d][c].x += wo;
    }
  }

  /*  for(n = 0; n < gptr->numnodes; n++){
    d = gptr->nodes[n]->depth;
    for (i = 0;gmatrix[d][i].node != NULL && gmatrix[d][i].node != gptr->nodes[n];i++);
    if (gmatrix[d][i].node == gptr->nodes[n])
      continue;
    gmatrix[d][i].node = gptr->nodes[n];
    gmatrix[d][i].x = i * spacing;
    gmatrix[d][i].y = 30 + d * spacing;
  }
  for (d = 0; d < height; d++){
    for(n = 0; gmatrix[d][n].node != NULL; n++);
    for(i = 0; gmatrix[d][i].node != NULL; i++){
      gmatrix[d][i].x = (spacing*(width-n+2*i))/2+offset;
    }
  }
  */

  flag = 0;
  for (d = 0; d < height; d++){
    for(n = 0; gmatrix[d][n].node != NULL; n++){
      xynode = &gmatrix[d][n];
      node = gmatrix[d][n].node;
      
      /* Print graph name */
      /*
      if (!flag && node->gname != NULL){
	fprintf(ofile, "4 0 0 50 0 0 %d 0.0000 4 105 645 ", (fontsize+4));
	fprintf(ofile, "%d %d %s\\001\n", offset*20, (fontsize+4)*10, node->gname);
	flag = 1;
      }
      */
      /* Print node target */
      /*
      if (node->lab.label > 1){
	sprintf(buffer, "c=\"%s\"", find_conv_to_lab(node->lab.label));
	fprintf(ofile, "4 0 0 50 0 0 %d 0.0000 4 105 645 ", fontsize);
	fprintf(ofile, "%d %d %s\\001\n", xynode->x*20 + 200+30, (int)(xynode->y*20-200+spacing+(fontsize*1.5)*10), buffer);
      }
      */
      /* Print node label */
      /*
      if (data->dimension > 0){
	if (node->points[0] == (int)node->points[0])
	  sprintf(buffer, "l=(%d", (int)node->points[0]);
	else
	  sprintf(buffer, "l=(%f", node->points[0]);
      }
      else
	strcpy(buffer, "(");
      */
      /*
      for (i = 1; i < data->dimension; i++){
	if (node->points[i] == (int)node->points[i])
	  sprintf(buffer, "%s, %d", buffer, (int)node->points[i]);
	else
	  sprintf(buffer, "%s, %f", buffer, node->points[i]);
      }
      strcat(buffer, ")");
      fprintf(ofile, "4 0 0 50 0 0 %d 0.0000 4 105 645 ", fontsize);
      fprintf(ofile, "%d %d %s\\001\n", xynode->x*20 + 200+30, xynode->y*20-200+spacing, buffer);
      */
      /* Print node and its links */
      fprintf(ofile, "1 4 0 2 0 7 50 0 -1 0.000 1 0.0000 ");
      fprintf(ofile, "%d %d %d %d %d %d %d %d\n", xynode->x, xynode->y, (int)scale, (int)scale, xynode->x-201, xynode->y, xynode->x+201, xynode->y);

      for (c = 0; c < gptr->FanOut && node->children[c] != NULL; c++){
	cd = d-1;
	for(cn = 0; gmatrix[cd][cn].node != NULL && gmatrix[cd][cn].node != node->children[c] && cn < width; cn++);
	if (gmatrix[cd][cn].node == NULL){
	  fprintf(stderr, "Child not found %d,%d\n", node->nnum, node->depth);
	  continue;
	}
	child = &gmatrix[cd][cn];
	fprintf(ofile, "2 1 0 2 0 7 50 0 -1 0.000 0 0 -1 1 0 2\n");
	fprintf(ofile, "0 0 1.00 60.00 120.00\n"); //if arrow mode is on
      	fprintf(ofile, "%d %d %d %d\n", xynode->x, xynode->y+(int)scale, child->x, child->y-(int)scale);
      }
    }
  }
  for (i = 0; i < height; i++)
    free(gmatrix[i]);
  free(gmatrix);
}

//Compute activation of every node of the map, and the maximum activation of any node on the map. Result is stored in vmap.activation[y][x], and vmap.max
struct VMap GetHits(int xdim, int ydim, struct Graph *graph, int mode)
{
  int n, N, nN, hits, x, y;
  struct VMap vmap = {0};
  struct Graph *gptr;
  struct Node *node;

  fprintf(stderr, "t3\n");
  vmap.activation = (UNSIGNED**)MyMalloc(ydim * sizeof(UNSIGNED*));
  for (n = 0; n < ydim; n++)
    vmap.activation[n] = (UNSIGNED*)MyCalloc(xdim, sizeof(FLOAT));

  hits = 0;
  vmap.max = 0;
  N = 0;
  nN = 0;
  for(gptr = graph; gptr != NULL; gptr = gptr->next){
    nN += gptr->numnodes;
    for (n = 0; n < gptr->numnodes; n++){
      node = gptr->nodes[n];

      if (mode & ROOT && IsRoot(node)){
	N++;
	vmap.activation[node->y][node->x] += 1;
	if (vmap.activation[node->y][node->x] > vmap.max)
	  vmap.max = vmap.activation[node->y][node->x];
	continue;
      }
      else if (mode & LEAF && IsLeaf(node)){
	vmap.activation[node->y][node->x] += 1;
	if (vmap.activation[node->y][node->x] > vmap.max)
	  vmap.max = vmap.activation[node->y][node->x];
	continue;
      }
      else if (mode & INTERMEDIATE && IsIntermediate(node)){
	vmap.activation[node->y][node->x] += 1;
	if (vmap.activation[node->y][node->x] > vmap.max)
	  vmap.max = vmap.activation[node->y][node->x];
	continue;
      }
    }
  }
  for (y = 0; y < ydim; y++){
    for (x = 0; x < xdim; x++){
      if (vmap.activation[y][x] != 0)
	hits++;
    }
  }
  fprintf(stdout, "Neurons activated: %d\n", hits);
  fprintf(stdout, "Compression ratio: %f (root nodes only)\n", (float)N/hits);
  fflush(stdout);

  return vmap;
}
//plot "x" u 1:2:3:4 w e, "x" u 1:2 w l lt 2, "x" u 1:3 w l lt 2, "x" u 1:4 w l lt 2
//plot "x" u 1:5:6:7 w e, "x" u 1:5 w l lt 2, "x" u 1:6 w l lt 2, "x" u 1:7 w l lt 2
//plot "x" u 1:8:9:10 w e, "x" u 1:8 w l lt 2, "x" u 1:9 w l lt 2, "x" u 1:10 w l lt 2
//plot "x" u 1:11:12:13 w e, "x" u 1:11 w l lt 2, "x" u 1:12 w l lt 2, "x" u 1:13 w l lt 2

void GetClusterID(struct Map map, struct Graph *graph, struct VMap *vmap)
{
  UNSIGNED *frequency;
  int numlabels;
  int n, max, id, x, y;
  struct Node *node;
  struct Graph *gptr;
  int xdim, ydim;
  int noactive = 0;

  xdim = map.xdim;
  ydim = map.ydim;
  vmap->winnerclass = (UNSIGNED**)MyMalloc(ydim * sizeof(UNSIGNED*));
  vmap->classes = (UNSIGNED***)MyMalloc(ydim * sizeof(UNSIGNED**));
  for (n = 0; n < ydim; n++){
    vmap->winnerclass[n] = (UNSIGNED*)MyCalloc(xdim, sizeof(UNSIGNED));
    vmap->classes[n] = (UNSIGNED**)MyCalloc(xdim, sizeof(UNSIGNED*));
  }

  if (graph == NULL)
    return;

  numlabels = GetNumLabels();
  vmap->numclasses = numlabels;

  for (y = 0; y < ydim; y++){
    for (x = 0; x < xdim; x++){
      if (vmap->activation[y][x] == 0)
	continue;

      frequency = MyCalloc(numlabels, sizeof(UNSIGNED));
      for(gptr = graph; gptr != NULL; gptr = gptr->next){
	for (n = 0; n < gptr->numnodes; n++){
	  node = gptr->nodes[n];
	  if (node->x == x && node->y == y)
	    if (GetLabel(node->label) != NULL)
	      	      if (strcmp(GetLabel(node->label), "*"))
		frequency[node->label-1]++;
	  
	}
      }
      id = -1;
      max = 0;
      for (n = 0; n < numlabels; n++){
	if (frequency[n] > max){
	  max = frequency[n];
	  id = n;
	}
      }
      if (id >= 0){
	vmap->winnerclass[y][x] = id+1;
	vmap->classes[y][x] = frequency;
      }
      else{
	noactive++;
	if (noactive < 10){
	  for (n = 1; n <= numlabels; n++)
	    fprintf(stderr, "%s->%d ", GetLabel(n), frequency[n-1]);
	  fprintf(stderr, "\n");
	}
	free(frequency);
      }
    }
  }
  if (noactive)
    fprintf(stderr, "There were %d activated neurons without label\n", noactive);
}

/*****************************************************************************
Description:

Return value:
*****************************************************************************/
void MapNodeLabel(struct Map map, struct Graph *graph)
{
  UNSIGNED n, cnt;
  UNSIGNED x, y;
  UNSIGNED *flags;
  struct Node *node;
  struct Graph *gptr;

  if (graph == NULL)
    return;

  if (GetNumLabels() < 1)
    return;

  flags = (UNSIGNED*)MyMalloc(GetNumLabels() * sizeof(UNSIGNED));

  /* initialize node location */
  if (KstepEnabled)
    K_Step_Approximation(&map, graph, 1);
  else
    GetNodeCoordinates(&map, graph);

  cnt = 0;
  for(gptr = graph; gptr != NULL; gptr = gptr->next)
    for (n = 0; n < gptr->numnodes; n++)
      if (IsRoot(gptr->nodes[n]))
	cnt++;
  fprintf(stderr, "%d roots\n", cnt);

  for (y = 0; y < map.ydim; y++){
    for (x = 0; x < map.xdim; x++){
      cnt = 0;
      memset(flags, 0, GetNumLabels() * sizeof(UNSIGNED));
      for(gptr = graph; gptr != NULL; gptr = gptr->next){
	for (n = 0; n < gptr->numnodes; n++){
	  node = gptr->nodes[n];
	  if (node->x == x && node->y == y && IsRoot(node) && GetLabel(node->label) != NULL){
	    flags[node->label-1]++;
	    cnt = 1;
	  }
	}
      }

      if (cnt > 0){
	printf("%d %d", x, y);
	for (n = 0; n < GetNumLabels(); n++){
	  printf(" %d", flags[n]);
	}
	printf("\n");
      }
    }
  }
  free(flags);
}


/*****************************************************************************
Description: In multi-layer contextual mode

Return value:
*****************************************************************************/
void ClassifyAndWriteDatafileWithParents(struct Map map, struct Graph *gptr)
{
  UNSIGNED i, n;
  UNSIGNED ldim, tdim, FanIn, FanOut;
  UNSIGNED soff, coff, poff;
  struct Node *node;

  if (gptr == NULL)
    return;

  /* initialize node location */
  if (KstepEnabled)
    K_Step_Approximation(&map, gptr, 1);
  else
    GetNodeCoordinates(&map, gptr);

  ldim = INT_MAX;  /* Initialize graph properties with illegal values to  */
  tdim = INT_MAX;  /* enforce the writing of a data header for the first  */
  FanIn = INT_MAX; /* graph. Any successife graph will have an own header */
  FanOut = INT_MAX;/* if a property differs from an earlier graph.        */
  printf("format=nodenumber,nodelabel,childstate,parentstate,target,depth,links,label\n");
  for(; gptr != NULL; gptr = gptr->next){

    /* Compute max indegree of graph */
    for (n = 0; n < gptr->numnodes; n++){
      if (gptr->FanIn < gptr->nodes[n]->numparents)
	gptr->FanIn = gptr->nodes[n]->numparents;
    }

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
      printf("graph:%s\n", gptr->gname);
    else
      printf("graph\n");

    for (n = 0; n < gptr->numnodes; n++){
      node = gptr->nodes[n];
      soff = gptr->ldim + 2*gptr->FanOut;
      poff = soff + 2*node->numparents;
      coff = poff + gptr->tdim;

      printf("%u ", node->nnum);  /* Print node number */
      for (i = 0; i < gptr->ldim; i++){
	if (node->points[i] == (int)node->points[i])
	  printf("%3d ", (int)node->points[i]); /* Print node label   */
	else
	  printf("%f ", node->points[i]); /* Print node label   */
      }
      for (; i < soff; i++){
	if (node->points[i] == (int)node->points[i])
	  printf("%3d ", (int)node->points[i]); /* Print child state  */
	else
	  printf("%f ", node->points[i]); /* Print child state  */
      }
      for (i = 0; i < gptr->FanIn; i++){
	if (i < node->numparents)       /* Print parent state */
	  printf("%3d %3d ", node->parents[i]->x, node->parents[i]->y);
	else
	  printf(" -1  -1 ");
      }
      for (i = poff; i < coff; i++)
	printf("%f ", node->points[i]); /* Print target vector */

      printf("%u ", node->depth);       /* Print node depth */
      
      for (i = 0; i < gptr->FanOut; i++){  /* Print links */
	if (node->children[i] == NULL)
	  printf("- ");
	else
	  printf("%u ", node->children[i]->nnum);
      }
      if (GetLabel(node->label) != NULL) /* Print node label */
	printf("%s\n", GetLabel(node->label));
      else
	printf("\n");
    }
  }
}

/*****************************************************************************
Description: In single-layer contextual mode

Return value:
*****************************************************************************/
void ClassifyAndWriteDatafile(struct Map map, struct Graph *gptr)
{
  UNSIGNED i, n;
  UNSIGNED ldim, tdim, FanIn, FanOut;
  UNSIGNED soff, coff, poff;
  struct Node *node;

  if (gptr == NULL)
    return;

  /* initialize node location */
  if (KstepEnabled)
    K_Step_Approximation(&map, gptr, 1);
  else
    GetNodeCoordinates(&map, gptr);

  ldim = INT_MAX;  /* Initialize graph properties with illegal values to  */
  tdim = INT_MAX;  /* enforce the writing of a data header for the first  */
  FanIn = INT_MAX; /* graph. Any successife graph will have an own header */
  FanOut = INT_MAX;/* if a property differs from an earlier graph.        */
  printf("format=nodenumber,nodelabel,childstate,parentstate,target,depth,links,label\n");
  for(; gptr != NULL; gptr = gptr->next){

    /* Compute max indegree of graph */
    for (n = 0; n < gptr->numnodes; n++){
      if (gptr->FanIn < gptr->nodes[n]->numparents)
	gptr->FanIn = gptr->nodes[n]->numparents;
    }

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
      printf("graph:%s\n", gptr->gname);
    else
      printf("graph\n");

    for (n = 0; n < gptr->numnodes; n++){
      node = gptr->nodes[n];
      soff = gptr->ldim + 2*gptr->FanOut;
      poff = soff + 2*node->numparents;
      coff = poff + gptr->tdim;

      printf("%u ", node->nnum);  /* Print node number */
      for (i = 0; i < gptr->ldim; i++){
	if (node->points[i] == (int)node->points[i])
	  printf("%3d ", (int)node->points[i]); /* Print node label   */
	else
	  printf("%f ", node->points[i]); /* Print node label   */
      }
      for (; i < soff; i++){
	if (node->points[i] == (int)node->points[i])
	  printf("%3d ", (int)node->points[i]); /* Print child state  */
	else
	  printf("%f ", node->points[i]); /* Print child state  */
      }
      for (i = 0; i < gptr->FanIn; i++){
	if (i < node->numparents)       /* Print parent state */
	  printf("%3d %3d ", node->parents[i]->x, node->parents[i]->y);
	else
	  printf(" -1  -1 ");
      }
      for (i = poff; i < coff; i++)
	printf("%f ", node->points[i]); /* Print target vector */

      printf("%u ", node->depth);       /* Print node depth */
      
      for (i = 0; i < gptr->FanOut; i++){  /* Print links */
	if (node->children[i] == NULL)
	  printf("- ");
	else
	  printf("%u ", node->children[i]->nnum);
      }

      if (GetLabel(node->label) != NULL) /* Print node label */
	printf("%s\n", GetLabel(node->label));
      else
	printf("\n");
    }
  }
}

/******************************************************************************
Description: 

Return value: 
******************************************************************************/
void MapGraph(struct Parameters parameters, int x, int y)
{
  int n;
  FLOAT qerr;
  struct Graph *graph;
  struct Node *node;
  struct Map *map;
  struct Winner winner;
  struct Hits{
    int gnum;
    int nnum;
    FLOAT err;
  } **hits;

  if (parameters.train == NULL)
    return;

  map = &parameters.map;

  /* Find the winners for all nodes */
  if (KstepEnabled)
    qerr = K_Step_Approximation(map, parameters.train, 1);
  else
    qerr = GetNodeCoordinates(map, parameters.train);
  fprintf(stderr, "Qerror:%f\n", qerr);

  if (x >= 0 && y >= 0){
    for (graph = parameters.train; graph != NULL; graph = graph->next){
      for (n = 0; n < graph->numnodes; n++){
	node = graph->nodes[n];
	if (x == node->x && y == node->y){
	  printf("%d %d %d %d\n", x, y, graph->gnum, node->nnum);
	  fprintf(stderr, "%s\n", graph->gname);
	}
      }
    }
  }
  else{
    hits = (struct Hits**)MyMalloc(map->ydim * sizeof(struct Hits*));
    for (y = 0; y < map->ydim; y++)
      hits[y] = (struct Hits*)MyCalloc(map->xdim, sizeof(struct Hits));

    for (y = 0; y < map->ydim; y++)
      for (x = 0; x < map->xdim; x++){
	hits[y][x].gnum = -1;
	hits[y][x].err = MAX_FLOAT;
      }

    for (graph = parameters.train; graph != NULL; graph = graph->next){
      for (n = 0; n < graph->numnodes; n++){
	node = graph->nodes[n];
	FindWinnerEucledian(map, node, graph, &winner);  //Find best match at location
	if (hits[node->y][node->x].err > winner.diff){
	  if (node->y != map->codes[winner.codeno].y && node->x != map->codes[winner.codeno].y)
	    fprintf(stderr, "Internal error\n");
	  hits[node->y][node->x].gnum = graph->gnum;
	  hits[node->y][node->x].nnum = node->nnum;
	  hits[node->y][node->x].err = winner.diff;
	}
      }
    }

    for (y = 0; y < map->ydim; y++)
      for (x = 0; x < map->xdim; x++)
	if (hits[y][x].gnum >= 0)
	  printf("%d %d %d %d\n", x, y, hits[y][x].gnum, hits[y][x].nnum);

    for (y = 0; y < map->ydim; y++)
      free(hits[y]);
    free(hits);
  }
}

/******************************************************************************
Description: 

Return value: 
******************************************************************************/
void VisualizeGraph(struct Parameters parameters)
{
  int nnum, gnum;
  int x, y, n;
  float scale = 200;
  int xpos, ypos;
  struct Graph *graph;
  struct Node *node;
  struct VMap vmap;

  if (KstepEnabled)
    K_Step_Approximation(&parameters.map, parameters.train, 1);
  else
    GetNodeCoordinates(&parameters.map, parameters.train);

  vmap = GetHits(parameters.map.xdim, parameters.map.ydim, parameters.train, LEAF | INTERMEDIATE | ROOT);
  PrintXfigHeader(stdout);
  DrawMap(parameters.map, scale*10, &vmap);

  fscanf(stdin, "%d %d %d %d", &x, &y, &gnum, &nnum);
  while (!feof(stdin)){
    for (graph = parameters.train; graph != NULL && graph->gnum != gnum; graph = graph->next);
    if (graph != NULL){
      for (n = 0; n < graph->numnodes; n++){
	if (graph->nodes[n]->nnum == nnum)
	  break;
      }
      if (n == graph->numnodes){
	fprintf(stderr, "Node not found error\n");
	return;
      }
      node = graph->nodes[n];

      xpos = 2*x*scale*13/3;
      ypos = y*scale*13-scale*13/2;
      if (x % 2)
	ypos -= scale*13/2;

      DrawXfigGraphAt(stdout, graph, node, xpos, ypos, scale);
    }
    fscanf(stdin, "%d %d %d %d", &x, &y, &gnum, &nnum);
  }
}

/******************************************************************************
Description: 

Return value: 
******************************************************************************/
void VisualizeSubGraphs(struct Parameters parameters)
{
  int n, nnum, gnum;
  FLOAT qerr;
  int x, y;
  float scale = 200;
  int xpos, ypos;
  struct Graph *graph;
  struct Node *node;
  struct VMap vmap;

  /* Find the winners for all nodes */
  if (KstepEnabled)
    qerr = K_Step_Approximation(&parameters.map, parameters.train, 1);
  else
    qerr = GetNodeCoordinates(&parameters.map, parameters.train);
  fprintf(stderr, "Qerror:%f\n", qerr);

  vmap = GetHits(parameters.map.xdim, parameters.map.ydim, parameters.train, LEAF | INTERMEDIATE | ROOT);

  PrintXfigHeader(stdout);
  DrawMap(parameters.map, scale*10, &vmap);

  fscanf(stdin, "%d %d %d %d", &x, &y, &gnum, &nnum);
  while (!feof(stdin)){
    for (graph = parameters.train; graph != NULL && graph->gnum != gnum; graph = graph->next);
    if (graph != NULL){
      fprintf(stderr, "%s\n", graph->gname);
      for (n = 0; n < graph->numnodes; n++){
	node = graph->nodes[n];
	x = node->x;
	y = node->y;

	xpos = 2*x*scale*13/3;
	ypos = y*scale*13-scale*13/2;
	if (x % 2)
	  ypos -= scale*13/2;

	DrawXfigGraphAt(stdout, graph, node, xpos, ypos, scale);
      }
    }
    fscanf(stdin, "%d %d %d %d", &x, &y, &gnum, &nnum);
  }
}

/******************************************************************************
Description: 

Return value: 
******************************************************************************/
void VisualizeClustering(struct Parameters parameters)
{
  int nnum, gnum;
  int x, y, n;
  float scale = 200;
  int xpos, ypos;
  struct Graph *graph;
  struct Node *node;
  struct VMap vmap;

  if (KstepEnabled)
    K_Step_Approximation(&parameters.map, parameters.train, 1);
  else
    GetNodeCoordinates(&parameters.map, parameters.train);

  vmap = GetHits(parameters.map.xdim, parameters.map.ydim, parameters.train, LEAF | INTERMEDIATE | ROOT);

  GetClusterID(parameters.map, parameters.train, &vmap);

  PrintXfigHeader(stdout);
  DrawMap(parameters.map, scale*10, &vmap);

  fscanf(stdin, "%d %d %d %d", &x, &y, &gnum, &nnum);
  while (!feof(stdin)){
    for (graph = parameters.train; graph != NULL && graph->gnum != gnum; graph = graph->next);
    if (graph != NULL){
      for (n = 0; n < graph->numnodes; n++){
	if (graph->nodes[n]->nnum == nnum)
	  break;
      }
      if (n == graph->numnodes){
	fprintf(stderr, "Node not found error\n");
	return;
      }
      node = graph->nodes[n];

      xpos = 2*x*scale*13/3;
      ypos = y*scale*13-scale*13/2;
      if (x % 2)
	ypos -= scale*13/2;

      DrawXfigGraphAt(stdout, graph, node, xpos, ypos, scale);
    }
    fscanf(stdin, "%d %d %d %d", &x, &y, &gnum, &nnum);
  }
}


/******************************************************************************
Description: 

Return value: 
******************************************************************************/
char *GetStructID2(int FanOut, struct Node *node, char *StructID)
{
  int c;

  memmove(StructID+2, StructID, strlen(StructID)+1);
  StructID[0] = '(';
  StructID[1] = ')';

  for (c = 0; c < FanOut; c++){
    if (node->children[c] != NULL)
      GetStructID2(FanOut, node->children[c], StructID+1);
  }
  return NULL;
}
/******************************************************************************
Description: 

Return value: 
******************************************************************************/
char *GetStructID(int FanOut, struct Node *node, char *StructID)
{
  int n;

  if (FanOut == 0)
    return NULL;
  else if (FanOut == 1){  //sequences
    sprintf(StructID, "(%d)", node->depth+1);
    return NULL;
  }
  else
    return GetStructID2(FanOut, node, StructID);
}

int comparStructID(const void *v1, const void *v2)
{
  struct AllHits *a1, *a2;

  a1 = (struct AllHits*)v1;
  a2 = (struct AllHits*)v2;

  return strcmp(a1->structID, a2->structID);
}

int comparsubStructID(const void *v1, const void *v2)
{
  struct AllHits *a1, *a2;

  a1 = (struct AllHits*)v1;
  a2 = (struct AllHits*)v2;

  return strcmp(a2->substructID, a1->substructID);
}

int comparFloat(const void *v1, const void *v2)
{
  return ((*((FLOAT*)v1)) > (*((FLOAT*)v2)));
}


/******************************************************************************
Description: 

Return value: 
******************************************************************************/
void AnalyseDataset(struct Parameters parameters)
{
  int i, j, n, r;
  int ni, nG, N;
  int nsub, nsdsub;
  char *buffer, *rbuf, *bigbuf;
  struct Graph *graph;
  struct Node *node;
  struct AllHits *harray;
  int minnodes, maxnodes;
  int V = 0, Vn = 0;
  FLOAT *labelval;
  int l, nl = 0, no, o, tmpo;
  int maxO = 0, minO, totalO;
  int olen;
  char *ctmp;
  int nlinks, yme;

  if (parameters.train == NULL){
    fprintf(stderr, "Error: No training set given.");
    return;
  }

  N = 0;
  nG = 0;
  minnodes = INT_MAX;
  maxnodes = 0;
  for (graph = parameters.train; graph != NULL; graph = graph->next){
    if (maxnodes < graph->numnodes)
      maxnodes = graph->numnodes;
    if (minnodes > graph->numnodes)
      minnodes = graph->numnodes;
    nG++;
    for (n = 0; n < graph->numnodes; n++)
      N++;
  }

  fprintf(stderr, "Total number of graphs: %d\n", nG);
  fprintf(stderr, "Total number of nodes : %d\n", N);
  fprintf(stderr, "Size of graphs: min %d nodes, ", minnodes);
  fprintf(stderr, "max %d nodes, avg %.2f nodes\n", maxnodes, (float)N/nG);

  if (parameters.train->FanOut == 1){/* Sequences */
    buffer = MyCalloc((maxnodes*3+1), sizeof(char));
    bigbuf = MyCalloc(parameters.train->ldim * 12 + (maxnodes*3+1), sizeof(char));
  }
  else{
    buffer = MyCalloc((maxnodes*2+1), sizeof(char));
    bigbuf = MyCalloc(parameters.train->ldim * 12+ (maxnodes*2+1), sizeof(char));
  }
  harray = (struct AllHits*)MyCalloc(N, sizeof(struct AllHits));
  labelval = (FLOAT*)MyCalloc(N, sizeof(FLOAT));

  ni = 0;
  rbuf = NULL;
  olen = 0;
  nlinks = 0;
  totalO = 0;
  yme = 0;
  for (graph = parameters.train; graph != NULL; graph = graph->next){
    r = -1;
    for (n = 0; n < graph->numnodes; n++){
      if (IsRoot(graph->nodes[n])){
	r = n;
	break;
      }
    }
    if (r == -1){
      fprintf(stderr, "Error: There is a graph with no root node\n");
      exit(0);
    }

    memset(buffer, 0, olen);
    GetStructID(graph->FanOut, graph->nodes[r], buffer);
    olen = strlen(buffer);
    rbuf = memdup(buffer, olen+1);

    minO = INT_MAX;
    tmpo = 0;
    for (n = 0; n < graph->numnodes; n++){
      node = graph->nodes[n];
      harray[ni].graph = graph;
      harray[ni].node = node;
      harray[ni].structID = rbuf;
      memset(buffer, 0, olen);
      GetStructID(graph->FanOut, node, buffer);
      olen = strlen(buffer);
      harray[ni].substructID =(char*)memdup(buffer, olen+1);

      for (l = 0; l < graph->ldim; l++)
	labelval[ni] += node->points[l] * node->points[l];

      no = 0;
      for (o = 0; o < graph->FanOut; o++)
	if (node->children[o] != 0){
	  no++;
	  nlinks++;
	}
      if (tmpo < no)
	tmpo = no;

      ni++;
    }
    if (tmpo > 34)
      yme++;
    if (tmpo > maxO){
      maxO = tmpo;
      ctmp = graph->gname;
    }
    if (minO > tmpo){
      minO = tmpo;
    }
    totalO += tmpo;
  }
  nsub = 0;
  nsdsub = 0;
  if (ni > 0){
    qsort(harray, N, sizeof(struct AllHits), comparStructID);
    V=1;
    Vn = harray[0].graph->numnodes;
    for (i = 1; i < N; i++){
      if (strcmp(harray[i-1].structID, harray[i].structID)){
	Vn += harray[i-1].graph->numnodes;
	V++;
      }
      //      printf("%s\n", harray[i].structID);
    }
    qsort(harray, N, sizeof(struct AllHits), comparsubStructID);
    nsub = 1;
    for (i = 1; i < N; i++){
      if (strcmp(harray[i-1].substructID, harray[i].substructID))
	nsub++;
    }
    //    printf("#\n");

    for (i = 0; i < N; i++){
      *bigbuf = '\0';
      for (j = 0; j < parameters.train->ldim; j++)
	sprintf(&bigbuf[strlen(bigbuf)], " %f", harray[i].node->points[j]);
      sprintf(&bigbuf[strlen(bigbuf)], " %s", harray[i].substructID);
      free(harray[i].substructID);
      harray[i].substructID = strdup(bigbuf);
    }
    qsort(harray, N, sizeof(struct AllHits), comparsubStructID);
    nsdsub = 1;
    for (i = 1; i < N; i++){
      if (strcmp(harray[i-1].substructID, harray[i].substructID))
	nsdsub++;
    }

    nl = 0;
    qsort(labelval, N, sizeof(FLOAT), comparFloat);
    for (i = 1; i < N; i++){
      if (labelval[i-1] != labelval[i]){
	nl++;
      }
    }
  }
  if (parameters.train->FanOut != maxO)
    fprintf(stderr, "Max outdegree stated: %d, but actual ", parameters.train->FanOut);
  fprintf(stderr, "max. outdegree is: %d\n", maxO);
  fprintf(stderr, "Outdegree of graphs: min %d, max %d, avg %.2E\n", minO,maxO, (float)totalO/nG);
  fprintf(stderr, "Total number of links: %d\n", nlinks);

  fprintf(stderr, "Number of unique substructures: %d (struct only)\n", nsub);
  fprintf(stderr, "Number of unique substructures: %d (struct & label)\n", nsdsub);
  fprintf(stderr, "Number of unique graphs: %d\n", V);
  fprintf(stderr, "Number of unique nodes(structure only): %d\n", Vn);
  fprintf(stderr, "Number of unique labels: %d\n", nl);
  fprintf(stderr, "%s %d\n", ctmp, yme);

  /* Cleanup */
  for (i = 0; i < N; i++)
    free(harray[i].substructID);
  free(harray);
  free(buffer);
  free(labelval);
}

/******************************************************************************
Description: 

Return value: 
******************************************************************************/
void ComputePrecision(struct Parameters parameters)
{
  int i, n, x, y, r;
  int flag, ni, mi, N;
  FLOAT qerr, Si, sSi;
  char *buffer;
  int olen;
  struct Graph *graph;
  struct Node *node;
  struct Map *map;
  struct AllHits *hits = NULL, *hptr, *hprev, *harray;

  if (parameters.train == NULL)
    return;

  map = &parameters.map;

  /* Find the winners for all nodes */
  if (parameters.map.topology == TOPOL_VQ)
    VQSet_ab(&parameters);

  if (KstepEnabled)
    qerr = K_Step_Approximation(map, parameters.train, 1);
  else
    qerr = GetNodeCoordinates(map, parameters.train);
  
  fprintf(stdout, "Qerror:%E\n", qerr);
  //  return;

  N = 0;
  Si = sSi = 0.0;
  olen = 0;
  buffer = MyCalloc(1024*1024, sizeof(char));
  for (y = 0; y < parameters.map.ydim; y++){
    for (x = 0; x < parameters.map.xdim; x++){
      ni = 0;
      for (graph = parameters.train; graph != NULL; graph = graph->next){
	for (n = 0; n < graph->numnodes; n++){
	  node = graph->nodes[n];
	  if (map->topology == TOPOL_VQ)            /* In VQ mode...   */
	    flag = (y*parameters.map.xdim+x == node->winner);
	  else
	    flag = (x == node->x && y == node->y);
	  if (flag){
	    hptr = (struct AllHits*)MyCalloc(1, sizeof(struct AllHits));

	    for (r = 0; r < graph->numnodes; r++)
	      if (IsRoot(graph->nodes[r]))
		break;
	    if (!IsRoot(graph->nodes[r]))
	      fprintf(stderr, "No root found\n");
	    hptr->graph = graph;
	    hptr->node = node;
	    memset(buffer, 0, olen);
	    GetStructID(hptr->graph->FanOut, graph->nodes[r], buffer);
	    olen = strlen(buffer);
	    hptr->structID = (char*)memdup(buffer, olen+1);

	    memset(buffer, 0, olen);
	    GetStructID(hptr->graph->FanOut, hptr->node, buffer);
	    olen = strlen(buffer);
	    hptr->substructID =(char*)memdup(buffer, olen+1);

	    hptr->next = hits;
	    hits = hptr;
	    ni++;
	    //  printf("%d %d %d %d\n", x, y, graph->gnum, node->nnum);
	    //  fprintf(stderr, "%s\n", graph->gname);
	  }
	}
      }

      //Sort list by structID
      if (ni > 0){
	harray = malloc(ni * sizeof(struct AllHits));
	for (i = 0, hptr = hits; hptr != NULL; hptr = hptr->next, i++)
	  memcpy(&harray[i], hptr, sizeof(struct AllHits));

	qsort(harray, ni, sizeof(struct AllHits), comparStructID);

	mi = 1;
	n = 1;
	hprev = &harray[0];
	for (i = 1; i < ni; i++){
	  if (!strcmp(harray[i].structID, hprev->structID))
	    n++;
	  else{
	    if (mi < n)
	      mi = n;
	    n = 1;
	    hprev = &harray[i];
	  }
	}
	if (mi < n)
	  mi = n;
	Si += (FLOAT)mi/ni;
      }

      //Sort list by substructID
      if (ni > 0){
	qsort(harray, ni, sizeof(struct AllHits), comparsubStructID);

	mi = 1;
	n = 1;
	hprev = &harray[0];
	for (i = 1; i < ni; i++){
	  if (!strcmp(harray[i].substructID, hprev->substructID))
	    n++;
	  else{
	    if (mi < n)
	      mi = n;
	    n = 1;
	    hprev = &harray[i];
	  }
	}
	if (mi < n)
	  mi = n;
	sSi += (FLOAT)mi/ni;
	free(harray);
	harray = NULL;
      }


      //Delete list
      while(hits != NULL){
	hptr = hits->next;
	free(hits->structID);
	free(hits->substructID);
	free(hits);
	hits = hptr;
      }
      if (ni > 0)
	N++;
    } //End x-loop
  } //End y-loop
  fprintf(stdout, "Struct mapping performance (E): %f\n", Si/N);
  fprintf(stdout, "SubStruct mapping performance (e): %f\n", sSi/N);
}



/******************************************************************************
Description: Compute best matching codebook for which
             vmap->activation[y][x] != 0


Return value: 
******************************************************************************/
void FindWinnerEucledianOnActiveOnly(struct Map *map, struct Node *node, struct Graph *gptr, struct Winner *winner, struct VMap *vmap)
{
  FLOAT *mu;
  UNSIGNED tend;
  UNSIGNED noc;  /* Number of codebooks in the map */
  FLOAT *codebook, *sample;
  UNSIGNED n, i;
  FLOAT diffsf, diff, difference;

  tend = gptr->dimension;
  mu = node->mu;

  noc = map->xdim * map->ydim;
  diffsf = FLT_MAX;
  sample = node->points;
  for (n = 0; n < noc; n++){  /* For every codebook of the map */

    if (vmap->activation[map->codes[n].y][map->codes[n].x] == 0)
      continue;

    codebook = map->codes[n].points;
    difference = 0.0;

    /* Compute the difference between codebook and input entry */
    for (i = 0; i < tend; i++){
      diff = codebook[i] - sample[i];
      difference += diff * diff * mu[i];
      if (difference > diffsf)
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

float ComputeClassificationConfusion(int x, int y, struct VMap *vmap)
{
  int i, num, bestnum;

  if (vmap->classes[y][x] == NULL){
    //given y and x should point to a valid location of a activated neuron
    fprintf(stderr, "Unexpected internal error\n");
    fprintf(stderr, "Debug info: %d %d %d %d\n", x, y, vmap->numclasses, vmap->activation[y][x]);
  }
  num = 0;
  bestnum = 0;
  for (i = 0; i < vmap->numclasses; i++){
    num += vmap->classes[y][x][i];
    if (vmap->classes[y][x][i] > bestnum)
      bestnum = vmap->classes[y][x][i];
  }
  //Bestnum should actually be the same as: vmap->classes[y][x][vmap->winnerclass[y][x]-1]

  return (float)bestnum/num;
}

int **ComputeConfusionMatrix(int xdim, int ydim, struct VMap *vmap)
{
  int i, row, ond, offd;
  int x, y;
  int r;
  int **matrix;
  int *lo;

  matrix = (int**)malloc(vmap->numclasses * sizeof(int*));
  for (i = 0; i < vmap->numclasses; i++)
    matrix[i] = (int*)calloc(vmap->numclasses, sizeof(int));

  for (y = 0; y < ydim; y++){
    for (x = 0; x < xdim; x++){
      if (vmap->classes[y][x] == NULL)
	continue;

      row = vmap->winnerclass[y][x]-1;
      for (i = 0; i < vmap->numclasses; i++)
	matrix[i][row] += vmap->classes[y][x][i];
    }
  }

  ond = 0;
  offd = 0;
  lo = GetSortedLabelIndex();
  for (x = 0; x < vmap->numclasses; x++)
    fprintf(stdout, " %d", lo[x]);
  fprintf(stdout, "\n");
  for (x = 0; x < vmap->numclasses; x++)
    fprintf(stdout, " %s", GetLabel(lo[x]));
  fprintf(stdout, "\n");
  for (y = 0; y < vmap->numclasses; y++){
    r = 0;
    for (x = 0; x < vmap->numclasses; x++){
      r+= matrix[lo[y]-1][lo[x]-1];
      if (x == y)
	ond += matrix[lo[y]-1][lo[x]-1];
      else
	offd += matrix[lo[y]-1][lo[x]-1];
      fprintf(stdout, " %4d", matrix[lo[y]-1][lo[x]-1]);
    }
    if (r > 0)
      fprintf(stdout, " #%.4f", (float)100*matrix[lo[y]-1][lo[y]-1]/r);
    fprintf(stdout, "\n");
  }
  fprintf(stdout, "On diagonal: %d\n", ond);
  fprintf(stdout, "Off diagonal: %d\n", offd);
  if (ond > 0)
    fprintf(stdout, "Confusion: %f\n", (float)offd*100/ond);


  return matrix;
}


//The following assumes a hexagonal map
float GetClusteringPerformance(struct Parameters parameters, struct VMap vmap)
{
  int x, y, i, j, n, W;
  int xdim, ydim;
  float Pi, P;
  int offset[9];
  int best, all, numlabels;
  int mid, id, nPi;
  struct Codebook *neuron;

  xdim = parameters.map.xdim;
  ydim = parameters.map.ydim;

  offset[0] = 0;
  offset[1] = 1;
  offset[2] = -1;
  offset[3] = xdim;
  offset[4] = -xdim+1;
  offset[5] = -xdim;
  offset[6] = -xdim-1;
  if (parameters.map.topology == TOPOL_RECT){
    n = 9;
    offset[7] = xdim-1;
    offset[8] = xdim+1;
  }
  else if (parameters.map.topology == TOPOL_HEXA)
    n = 7;
  else{
    fprintf(stderr, "Unsupported Neighbourhood in function GetClusteringPerformance()\n");
    return -1;
  }

  W = 0;
  P = 0.0;
  numlabels = GetNumLabels();
  for (y = 0; y < ydim; y++){
    for (x = 0; x < xdim; x++){
      mid = y*xdim+x;
      best = vmap.winnerclass[y][x]-1;
      if (best < 0)
	continue;
      Pi = 0.0;
      nPi = 0;
      for (i = 0; i < n; i++){
	id = mid+offset[i];
	if (id < 0 || id >= xdim*ydim)
	  continue;

	neuron = &parameters.map.codes[id];
	if (abs(neuron->x - parameters.map.codes[mid].x) <= 1 &&
	    abs(neuron->y - parameters.map.codes[mid].y) <= 1 &&
	    vmap.classes[neuron->y][neuron->x] != NULL){

	  all = 0;
	  for (j = 0; j < numlabels; j++)
	    all += vmap.classes[neuron->y][neuron->x][j];
	  Pi += (float)vmap.classes[neuron->y][neuron->x][best]/all;
	  nPi++;
	}
      }
      if (nPi > 0){
	Pi = Pi/nPi;
	P += Pi;
	W++;
      }
    }
  }
  if (W>0)
    P = P/W;
  return P;
}

/******************************************************************************
Description: 

Return value: 
******************************************************************************/
void ComputeRetrievalPerformance(struct Parameters parameters, int classifyflag)
{
  int flag = 0;
  struct VMap vmap;
  struct VMap tvmap; //for the test set
  float R, P;
  int C, n, nnum;
  int winnerx, winnery;
  struct Graph *gptr;
  struct Node *node;
  struct Winner winner = {0};
  struct Map *map;

  if (parameters.test == NULL){
    printf("Warning: No test file given. Will use training data for testing.\n");
    parameters.test = parameters.train;
    flag = 1;
  }

  map = &parameters.map;
  /* Compute the mapping of nodes in the training and the test dataset */
  if (KstepEnabled){
    K_Step_Approximation(map, parameters.train, 1);
    if (!flag)
      K_Step_Approximation(map, parameters.test, 1);
  }
  else{
    GetNodeCoordinates(map, parameters.train);
    if (!flag)
      GetNodeCoordinates(map, parameters.test);
  }

  vmap = GetHits(parameters.map.xdim, parameters.map.ydim, parameters.train, ROOT);
  fprintf(stderr, "t2\n");
  GetClusterID(parameters.map, parameters.train, &vmap);
  if (!flag){
    tvmap = GetHits(parameters.map.xdim, parameters.map.ydim, parameters.test, ROOT);
    GetClusterID(parameters.map, parameters.test, &tvmap);
  }

  R = 0.0;
  C = 0;
  n = 0;
  //For (every node in the test set){
  for (gptr = parameters.test;gptr != NULL; gptr = gptr->next){
    for (nnum = 0; nnum < gptr->numnodes; nnum++){
      node = gptr->nodes[nnum];
      if (!IsRoot(node))
	continue;
      FindWinnerEucledianOnActiveOnly(map, node, gptr, &winner, &vmap);
      winnerx = map->codes[winner.codeno].x;
      winnery = map->codes[winner.codeno].y;
      n++;
      R += ComputeClassificationConfusion(winnerx, winnery, &vmap);
      if (classifyflag != 0)
	fprintf(stdout, "Graph:%s %s (%d,%s)", gptr->gname, GetLabel(vmap.winnerclass[winnery][winnerx]), node->label, GetLabel(node->label));
      if (node->label == vmap.winnerclass[winnery][winnerx]){
	//	fprintf(stdout, "G\n");
	C++;
      }
      //      else
      //	fprintf(stdout, "B:%E\n", winner.diff);
    }
  }
  if (flag)
    P = GetClusteringPerformance(parameters, vmap);
  else
    P = GetClusteringPerformance(parameters, tvmap);

  if (classifyflag == 0){
    printf("Retrieval performance: %f\n", 100.0*R/n);
    printf("Classification performance: %f\n", (float)100.0*C/n);
    printf("Clustering performance: %f\n", P);
  }

  //  for (n = 0; n < vmap.numclasses; n++)
  //    fprintf(stdout, "%s\n", GetLabel(n+1));

  ComputeConfusionMatrix(parameters.map.xdim, parameters.map.ydim, &vmap);

  if (flag)
    parameters.test = NULL;  /* Restore test pointer */
}

void Compute(struct Parameters parameters, int classifyflag)
{
  int i, n, x, y, r;
  int flag, ni, mi, N;
  FLOAT qerr, Si, sSi;
  char *buffer;
  int olen;
  struct Graph *graph;
  struct Node *node;
  struct Map *map;
  struct AllHits *hits = NULL, *hptr, *hprev, *harray;
  struct VMap vmap;
  struct VMap tvmap; //for the test set
  float R, P;
  int C, nnum;
  int winnerx, winnery;
  struct Graph *gptr;
  struct Winner winner = {0};

  if (parameters.train == NULL)
    return;

  map = &parameters.map;

  /* Find the winners for all nodes */
  if (parameters.map.topology == TOPOL_VQ)
    VQSet_ab(&parameters);

  if (KstepEnabled)
    qerr = K_Step_Approximation(map, parameters.train, 1);
  else
    qerr = GetNodeCoordinates(map, parameters.train);
  
  fprintf(stdout, "Qerror:%E\n", qerr);

  N = 0;
  Si = sSi = 0.0;
  olen = 0;
  buffer = MyCalloc(1024*1024, sizeof(char));
  for (y = 0; y < parameters.map.ydim; y++){
    for (x = 0; x < parameters.map.xdim; x++){
      ni = 0;
      for (graph = parameters.train; graph != NULL; graph = graph->next){
	for (n = 0; n < graph->numnodes; n++){
	  node = graph->nodes[n];
	  if (map->topology == TOPOL_VQ)            /* In VQ mode...   */
	    flag = (y*parameters.map.xdim+x == node->winner);
	  else
	    flag = (x == node->x && y == node->y);
	  if (flag){
	    hptr = (struct AllHits*)MyCalloc(1, sizeof(struct AllHits));

	    for (r = 0; r < graph->numnodes; r++)
	      if (IsRoot(graph->nodes[r]))
		break;
	    if (!IsRoot(graph->nodes[r]))
	      fprintf(stderr, "No root found\n");
	    hptr->graph = graph;
	    hptr->node = node;
	    memset(buffer, 0, olen);
	    GetStructID(hptr->graph->FanOut, graph->nodes[r], buffer);
	    olen = strlen(buffer);
	    hptr->structID = (char*)memdup(buffer, olen+1);

	    memset(buffer, 0, olen);
	    GetStructID(hptr->graph->FanOut, hptr->node, buffer);
	    olen = strlen(buffer);
	    hptr->substructID =(char*)memdup(buffer, olen+1);

	    hptr->next = hits;
	    hits = hptr;
	    ni++;
	    //  printf("%d %d %d %d\n", x, y, graph->gnum, node->nnum);
	    //  fprintf(stderr, "%s\n", graph->gname);
	  }
	}
      }

      //Sort list by structID
      if (ni > 0){
	harray = malloc(ni * sizeof(struct AllHits));
	for (i = 0, hptr = hits; hptr != NULL; hptr = hptr->next, i++)
	  memcpy(&harray[i], hptr, sizeof(struct AllHits));

	qsort(harray, ni, sizeof(struct AllHits), comparStructID);

	mi = 1;
	n = 1;
	hprev = &harray[0];
	for (i = 1; i < ni; i++){
	  if (!strcmp(harray[i].structID, hprev->structID))
	    n++;
	  else{
	    if (mi < n)
	      mi = n;
	    n = 1;
	    hprev = &harray[i];
	  }
	}
	if (mi < n)
	  mi = n;
	Si += (FLOAT)mi/ni;
      }

      //Sort list by substructID
      if (ni > 0){
	qsort(harray, ni, sizeof(struct AllHits), comparsubStructID);

	mi = 1;
	n = 1;
	hprev = &harray[0];
	for (i = 1; i < ni; i++){
	  if (!strcmp(harray[i].substructID, hprev->substructID))
	    n++;
	  else{
	    if (mi < n)
	      mi = n;
	    n = 1;
	    hprev = &harray[i];
	  }
	}
	if (mi < n)
	  mi = n;
	sSi += (FLOAT)mi/ni;
	free(harray);
	harray = NULL;
      }


      //Delete list
      while(hits != NULL){
	hptr = hits->next;
	free(hits->structID);
	free(hits->substructID);
	free(hits);
	hits = hptr;
      }
      if (ni > 0)
	N++;
    } //End x-loop
  } //End y-loop
  fprintf(stdout, "Struct mapping performance (E): %f\n", Si/N);
  fprintf(stdout, "SubStruct mapping performance (e): %f\n", sSi/N);




  flag = 0;

  if (parameters.test == NULL){
    printf("Warning: No test file given. Will use training data for testing.\n");
    parameters.test = parameters.train;
    flag = 1;
  }

  map = &parameters.map;
  /* Compute the mapping of nodes in the training and the test dataset */
  if (KstepEnabled){
    if (!flag)
      K_Step_Approximation(map, parameters.test, 1);
  }
  else{
    if (!flag)
      GetNodeCoordinates(map, parameters.test);
  }

  vmap = GetHits(parameters.map.xdim, parameters.map.ydim, parameters.train, ROOT);
  GetClusterID(parameters.map, parameters.train, &vmap);
  if (!flag){
    tvmap = GetHits(parameters.map.xdim, parameters.map.ydim, parameters.test, ROOT);
    GetClusterID(parameters.map, parameters.test, &tvmap);
  }

  R = 0.0;
  C = 0;
  n = 0;
  //For (every node in the test set){
  for (gptr = parameters.test;gptr != NULL; gptr = gptr->next){
    for (nnum = 0; nnum < gptr->numnodes; nnum++){
      node = gptr->nodes[nnum];
      if (!IsRoot(node))
	continue;
      FindWinnerEucledianOnActiveOnly(map, node, gptr, &winner, &vmap);
      winnerx = map->codes[winner.codeno].x;
      winnery = map->codes[winner.codeno].y;
      n++;
      R += ComputeClassificationConfusion(winnerx, winnery, &vmap);
      if (classifyflag != 0)
	fprintf(stdout, "Graph:%s %s (%d,%s)", gptr->gname, GetLabel(vmap.winnerclass[winnery][winnerx]), node->label, GetLabel(node->label));
      if (node->label == vmap.winnerclass[winnery][winnerx]){
	C++;
      }
    }
  }
  if (flag)
    P = GetClusteringPerformance(parameters, vmap);
  else
    P = GetClusteringPerformance(parameters, tvmap);

  if (classifyflag == 0){
    printf("Retrieval performance: %f\n", 100.0*R/n);
    printf("Classification performance: %f\n", (float)100.0*C/n);
    printf("Clustering performance: %f\n", P);
  }

  ComputeConfusionMatrix(parameters.map.xdim, parameters.map.ydim, &vmap);

  if (flag)
    parameters.test = NULL;  /* Restore test pointer */
}


/******************************************************************************
Description: 

Return value: 
******************************************************************************/
void AnalyseGraphs(struct Parameters parameters)
{
  int i,j,k;
  int nnum, imin, imax, iavg, itotal, inum, inum2, fan, maxfan;
  float fmin, fmax, favg, ftotal, fval, fvar;
  int minnodes, maxnodes;
  struct Graph *gptr;
  struct Node *node;
  char *name;

  imin = INT_MAX;
  imax = 0;
  itotal = 0;
  inum = 0;
  inum2 = 0;
  maxnodes = 0;
  minnodes = INT_MAX;
  for (gptr = parameters.train; gptr != NULL; gptr = gptr->next){
    maxfan = 0;
    if (maxnodes < gptr->numnodes)
      maxnodes = gptr->numnodes;
    if (minnodes > gptr->numnodes)
      minnodes = gptr->numnodes;
    for (nnum = 0; nnum < gptr->numnodes; nnum++){
      node = gptr->nodes[nnum];
      fan = 0;
      for (i = 0; i < gptr->FanOut; i++)
	if (node->children[i] != NULL)
	  fan++;
      if (maxfan < fan)
	maxfan = fan;
    }
    if (maxfan > imax)
      imax = maxfan;
    if (maxfan < imin)
      imin = maxfan;
    itotal += maxfan;
    inum++;
    inum2 += gptr->numnodes;
  }
  fprintf(stderr, "Total number of graphs: %d\n", inum);
  fprintf(stderr, "Total number of nodes: %d\n", inum2);
  fprintf(stderr, "Smallest graph: %d nodes\n", minnodes);
  fprintf(stderr, "Largest  graph: %d nodes\n", maxnodes);
  favg = (float)itotal/inum;
  fvar = 0.0;
  for (gptr = parameters.train; gptr != NULL; gptr = gptr->next){
    fval = gptr->FanOut - favg;
    fvar += fval * fval;
  }
  fvar /= inum;
  fprintf(stderr, "Outdegree: min=%d, max=%d, avg=%E, stddev=%E\n", imin, imax, favg, sqrt(fvar));
  //  fprintf(stderr, "%s\n", name);
}

/******************************************************************************
Description: 

Return value: 
******************************************************************************/
int SelectBiased(float *array, int num, float max)
{
  int i;
  float rval, sum;

  rval = drand48() * max;
  sum = 0.0;
  for (i = 0; i < num; i++){
    sum += array[i];
    if (rval < sum)
      break;
  }
  return i;
}

/******************************************************************************
Description: 

Return value: 
******************************************************************************/
void BalanceGraphs(struct Parameters parameters)
{
  int i,j;
  int nnum, imin, imax, itotal, inum, inum2, fan, maxfan, class, starindex;
  int numlabels;
  int *labels, **outdegrees;
  float *lrel, **orel, *ocount; 
  float ftotal;
  struct Graph *gptr, *gold;
  struct Node *node;
  struct GraphSorted{
    int num;
    int idx;
    struct Graph **graphs;
  } **graphs;


  starindex = -1;
  numlabels = GetNumLabels();
  for (i = 1; i <= numlabels; i++){
    if (!strcmp(GetLabel(i), "*")){
      starindex = i;
      break;
    }
  }

  if (starindex > 0)
    fprintf(stderr, "Number of labels: %d\n", numlabels-1);
  else
    fprintf(stderr, "Number of labels: %d\n", numlabels);

  labels = (int*)MyCalloc(numlabels, sizeof(int));
  lrel = (float*)MyCalloc(numlabels, sizeof(float));
  outdegrees = (int**)MyCalloc(numlabels, sizeof(int*));
  orel = (float**)MyCalloc(numlabels, sizeof(float*));
  ocount = (float*)MyCalloc(numlabels, sizeof(float));
  graphs = (struct GraphSorted**)MyCalloc(numlabels, sizeof(struct GraphSorted*));
  for (i = 0; i < numlabels; i++){
    outdegrees[i] = (int*)MyCalloc(parameters.train->FanOut+1, sizeof(int));
    orel[i] = (float*)MyCalloc(parameters.train->FanOut+1, sizeof(float));
    graphs[i] = (struct GraphSorted*)MyCalloc(parameters.train->FanOut+1, sizeof(struct GraphSorted));
  }

  for (gptr = parameters.train; gptr != NULL; gptr = gptr->next){
    for (nnum = 0; nnum < gptr->numnodes; nnum++){
      node = gptr->nodes[nnum];
      labels[node->label-1]++;
    }
  }
  itotal = 0;
  for (i = 0; i < numlabels; i++)
    if (i+1 != starindex)
      itotal += labels[i];

  ftotal = 0.0;
  fprintf(stderr, "Label : Frequency\n----------------\n");
  for (i = 0; i < numlabels; i++){
    if (i+1 != starindex){
      lrel[i] = (float)itotal/labels[i];
      ftotal += lrel[i];
      fprintf(stderr, "%s : %d %f %d\n", GetLabel(i+1), labels[i], lrel[i], labels[i]*100/itotal);
    }
    else
      labels[i] = 0;
  }
  fprintf(stderr, "----------------\nTotal : %d %f\n", itotal, ftotal);

  imin = INT_MAX;
  imax = 0;
  inum = 0;
  inum2 = 0;
  for (gptr = parameters.train; gptr != NULL; gptr = gptr->next){
    maxfan = 0;
    class = -1;
    for (nnum = 0; nnum < gptr->numnodes; nnum++){
      node = gptr->nodes[nnum];
      fan = 0;
      for (i = 0; i < gptr->FanOut+1; i++)
	if (node->children[i] != NULL)
	  fan++;
      if (maxfan < fan)
	maxfan = fan;
      if (node->label != starindex)
	class = node->label-1;
    }
    if (class >= 0 && maxfan < gptr->FanOut+1)
      outdegrees[class][maxfan]++;
    else
      fprintf(stderr, "Error: No class or fanin > max (%d: %d,%d)\n", class, maxfan, gptr->FanOut);

    inum++;
    inum2 += gptr->numnodes;
  }
  fprintf(stderr, "Outdegree : Labels...\n----------------\n    ");
  for (i = 0; i < numlabels; i++){
    if (i+1 != starindex){
      fprintf(stderr, " %4s", GetLabel(i+1));
    }
  }
  fprintf(stderr, "\n");
  for (j = 0; j < parameters.train->FanOut+1; j++){
    fprintf(stderr, "%3d :", j);
    for (i = 0; i < numlabels; i++){
      if (i+1 != starindex){
	fprintf(stderr, " %4d", outdegrees[i][j]);
	if (outdegrees[i][j] > 0){
	  orel[i][j] = (float)labels[i]/outdegrees[i][j];
	  ocount[i] += orel[i][j];
	}
      }
    }
    fprintf(stderr, "\n");
  }
  fprintf(stderr, "Total");
  for (i = 0; i < numlabels; i++){
    if (i+1 != starindex){
      fprintf(stderr, " %4d", labels[i]);
    }
  }
  fprintf(stderr, "\n");



  fprintf(stderr, "Outdegree : Labels...\n----------------\n   ");
  for (i = 0; i < numlabels; i++){
    if (i+1 != starindex){
      fprintf(stderr, " %5s", GetLabel(i+1));
    }
  }
  fprintf(stderr, "\n");
  for (j = 0; j < parameters.train->FanOut+1; j++){
    fprintf(stderr, "%3d :", j);
    for (i = 0; i < numlabels; i++){
      if (i+1 != starindex){
	if (orel[i][j] < 10)
	  fprintf(stderr, " ");
	if (orel[i][j] < 100)
	  fprintf(stderr, " ");
	fprintf(stderr, " %3.1f", orel[i][j]);
      }
    }
    fprintf(stderr, "\n");
  }
  fprintf(stderr, "Total");
  for (i = 0; i < numlabels; i++){
    if (i+1 != starindex){
      fprintf(stderr, " %3.1f", ocount[i]);
    }
  }
  fprintf(stderr, "\n");



  fprintf(stderr, "Number of graphs: %d\n", inum);
  fprintf(stderr, "Number of nodes: %d\n", inum2);

  for (i = 0; i < numlabels; i++){
    for (j = 0; j < parameters.train->FanOut+1; j++){
      if (outdegrees[i][j] > 0){
	graphs[i][j].graphs = (struct Graph **)MyCalloc(outdegrees[i][j], sizeof(struct Graph *));
      }
    }
  }
  for (gptr = parameters.train; gptr != NULL; gptr = gptr->next){
    maxfan = 0;
    class = -1;
    for (nnum = 0; nnum < gptr->numnodes; nnum++){
      node = gptr->nodes[nnum];
      fan = 0;
      for (i = 0; i < gptr->FanOut+1; i++)
	if (node->children[i] != NULL)
	  fan++;
      if (maxfan < fan)
	maxfan = fan;
      if (node->label != starindex)
	class = node->label-1;
    }
    graphs[class][maxfan].graphs[graphs[class][maxfan].num] = gptr;
    graphs[class][maxfan].num++;
  }
  for (gptr = parameters.train; gptr != NULL; ){
    gold = gptr;
    gptr = gptr->next;
    gold->next = NULL;
  }
  srand48(15);
  for (i = 0; i < 30000; i++){
    //    class = SelectBiased(lrel, numlabels, ftotal);
    //    fan = SelectBiased(orel[class], parameters.train->FanOut+1, ocount[class]);
    do{
      class = drand48()*numlabels;
    }while (class+1 == starindex);
    do{
      fan = drand48()*parameters.train->FanOut+1;
    }while(graphs[class][fan].num == 0);
    SaveData(stdout, graphs[class][fan].graphs[graphs[class][fan].idx]);
    graphs[class][fan].idx = (graphs[class][fan].idx+1) % graphs[class][fan].num;
    fprintf(stderr, "\r%d", i);
  }
}


/******************************************************************************
Description: 

Return value: 
******************************************************************************/
void MarkSubtreeNodes(struct Node *node, struct Graph *gptr, int *nodeflags)
{
  int i;

  nodeflags[node->nnum]++;
  for (i = 0; i < gptr->FanOut; i++){
    if (node->children[i] != NULL && nodeflags[node->children[i]->nnum] == 0)
      MarkSubtreeNodes(node->children[i], gptr, nodeflags);
  }
}

/******************************************************************************
Description: 

Return value: 
******************************************************************************/
void UnmarkSubtreeNodes(struct Node *node, struct Graph *gptr, int maxout, int *nodeflags)
{
  int i;

  nodeflags[node->nnum] = 0;
  for (i = 0; i < maxout && i < gptr->FanOut; i++){
    if (node->children[i] != NULL)
      UnmarkSubtreeNodes(node->children[i], gptr, maxout, nodeflags);
  }
}

/******************************************************************************
Description: 

Return value: 
******************************************************************************/
void MarkExceedingOutdegree(struct Graph *gptr, int maxout, int *nodeflags)
{
  int nnum, i;
  struct Node *node;

  if (gptr->FanOut <= maxout)
    return;

  for (nnum = 0; nnum < gptr->numnodes; nnum++){
    node = gptr->nodes[nnum];

    if (node == NULL)
      continue;

    for (i = maxout; i < gptr->FanOut; i++){
      if (node->children[i] != NULL)
	MarkSubtreeNodes(node->children[i], gptr, nodeflags);
    }
  }
}

/******************************************************************************
Description: 

Return value: 
******************************************************************************/
void UnmarkNeededNodes(struct Graph *gptr, int maxout, int *nodeflags)
{
  int i, nnum;
  struct Node *node;

  for (nnum = 0; nnum < gptr->numnodes; nnum++){
    node = gptr->nodes[nnum];

    if (node == NULL)
      continue;

    if (nodeflags[node->nnum] != 0)
      continue;

    for (i = 0; i < maxout && i < gptr->FanOut; i++){
      if (node->children[i] != NULL)
	UnmarkSubtreeNodes(node->children[i], gptr, maxout, nodeflags);
    }
  }
}

/******************************************************************************
Description: 

Return value: 
******************************************************************************/
void Truncate(struct Parameters parameters, int maxout)
{
  int i, idx;
  int nnum, inum, fan, maxfan, count;
  struct Graph *gptr;
  struct Node *node, **nodearray;
  int numexceed, maxnodes;
  int *childflags, *nodeflags = NULL;


  fprintf(stderr, "Analysing data...");
  fflush(stderr);

  maxfan = 0;
  maxnodes = 0;
  /* Obtain max outdegree and max number of nodes of a graph */
  for (gptr = parameters.train; gptr != NULL; gptr = gptr->next){
    if (maxfan < gptr->FanOut)
      maxfan = gptr->FanOut;
    if (maxnodes < gptr->numnodes)
      maxnodes = gptr->numnodes;
  }
  /* Flags indicating actually existing slots where children are found */
  childflags = (int*)MyCalloc(maxfan, sizeof(int));
  nodearray = (struct Node **)MyCalloc(maxnodes, sizeof(struct Node *));

  /* Computing the actual max out-degree */
  numexceed = 0;
  inum = 0;
  for (gptr = parameters.train; gptr != NULL; gptr = gptr->next){
    if (gptr->FanOut > maxout)
      inum++;
    for (nnum = 0; nnum < gptr->numnodes; nnum++){
      fan = 0;
      node = gptr->nodes[nnum];
      for (i = 0; i < gptr->FanOut; i++){
	if (node->children[i] != NULL){
	  childflags[i]++;
	  fan++;
	}
      }
      if (fan > maxout)
	numexceed++;
    }
  }
  fan = 0;
  for (i = 0; i < maxfan; i++)
    if (childflags[i] > 0)
      fan++;
  fprintf(stderr, "done\n");

  if (numexceed)
    fprintf(stderr, "%d nodes seem to exceed the max outdegree of %d\n", numexceed, maxout);
  if (fan < maxfan)
    fprintf(stderr, "Dataset specified %d as the maximum outdegree while the actual max outdegree is %d.\n", maxfan, fan);

  if (maxfan <= maxout){
    fprintf(stderr, "No nodes exceeded the max outdegree of %d\n", maxout);
  }
  else{
    fprintf(stderr, "Truncating...");
    fflush(stderr);

    nodeflags = MyMalloc(maxnodes * sizeof(int));

    for (gptr = parameters.train; gptr != NULL; gptr = gptr->next){
      memset(nodeflags, 0, maxnodes * sizeof(int));
      MarkExceedingOutdegree(gptr, maxout, nodeflags);

      /*
      fprintf(stderr, "\n");
      for (nnum = 0; nnum < gptr->numnodes; nnum++)
	fprintf(stderr, "%2d ", gptr->nodes[nnum]->nnum);
      fprintf(stderr, "\n");
      for (nnum = 0; nnum < gptr->numnodes; nnum++)
	fprintf(stderr, "%2d ", nodeflags[nnum]);
      fprintf(stderr, "\n");
      */
      UnmarkNeededNodes(gptr, maxout, nodeflags);

      /*
      fprintf(stderr, "\n");
      for (nnum = 0; nnum < gptr->numnodes; nnum++)
	fprintf(stderr, "%2d ", nodeflags[nnum]);
      fprintf(stderr, "\n");
      */

      /* Removing selected nodes */
      memset(nodearray, 0, maxnodes * sizeof(struct Node*));
      for (nnum = 0; nnum < gptr->numnodes; nnum++){
	if (nodeflags[gptr->nodes[nnum]->nnum] == 0)
	  nodearray[nnum] = gptr->nodes[nnum];
      }

      /* Consolidating node array */
      for (idx = 0, nnum = 0; nnum < gptr->numnodes; nnum++){
	if (nodearray[nnum] == NULL)
	  continue;
	nodearray[idx++] = nodearray[nnum];
      }

      /* Renumber nodes if necessary */
      if (idx != nnum){
	for (nnum = 0; nnum < idx; nnum++){
	  nodearray[nnum]->nnum = idx-nnum-1;
	}
      }

      /* Assign new array of nodes to graph */
      memcpy(gptr->nodes, nodearray, idx * sizeof(struct Node *));
      if (maxout < gptr->FanOut)
	gptr->FanOut = maxout;
      gptr->numnodes = idx;
    }

    fprintf(stderr, "done\n");
  }
 
  fprintf(stderr, "Saving...");
  SaveData(stdout, parameters.train);
  fprintf(stderr, "done.");

  free(nodeflags);
  free(childflags);
  free(nodearray);
}


/******************************************************************************
Description: 

Return value: 
******************************************************************************/
void ListDistances(struct Parameters parameters)
{
  int n, nc;
  FLOAT qerr;
  struct Graph *graph;
  struct Node *node;
  struct Map *map;
  struct Winner winner;
  time_t t;
  FLOAT *mu;
  UNSIGNED tend;
  UNSIGNED noc;  /* Number of codebooks in the map */
  FLOAT *codebook, *sample;
  UNSIGNED i;
  FLOAT diff, difference, dist;

  if (parameters.train == NULL)
    return;

  map = &parameters.map;

  /* Find the winners for all nodes */
  if (KstepEnabled)
    qerr = K_Step_Approximation(map, parameters.train, 1);
  else
    qerr = GetNodeCoordinates(map, parameters.train);

  t = time(NULL);
  printf("#Generated: %s", ctime(&t));
  printf("#Dataset: %s\n", parameters.datafile);
  printf("#Network: %s\n", parameters.inetfile);
  printf("#Network is of size: %d x %d = %d\n", map->xdim, map->ydim, map->xdim*map->ydim);
  printf("#Note: All distances are squared values\n");

  for (graph = parameters.train; graph != NULL; graph = graph->next){
    printf("\ngraph:%s\n", graph->gname);
    for (n = 0; n < graph->numnodes; n++)
      if (IsRoot(graph->nodes[n]))
	break;
    printf("class:%s\n", GetLabel(graph->nodes[n]->label));
    for (n = 0; n < graph->numnodes; n++){
      node = graph->nodes[n];
      FindWinnerEucledian(map, node, graph, &winner);  //Find best match at location
      printf("  node:%d\n", node->nnum);
      if (node->x != map->codes[winner.codeno].x && node->y != map->codes[winner.codeno].y)
	fprintf(stderr, "Internal error\n");
      printf("  Coordinate of best winner:%d %d\n", map->codes[winner.codeno].x, map->codes[winner.codeno].y);
      printf("  Distance to best winner:%lE\n", winner.diff);
      printf("  Distances to direct neighbors (x y distance):\n");
      //      printf("  # Listing data as follows: Coords of        \n");
      //      printf("  # neighbor, distance between node and codebook\n");
      //      printf("  #---------------------------------------------\n");

      tend = graph->dimension;
      mu = node->mu;

      noc = map->xdim * map->ydim;
      sample = node->points;
      for (nc = 0; nc < noc; nc++){  /* For every codebook of the map */

	dist  = ComputeHexaDistance(node->x, node->y, map->codes[nc].x, map->codes[nc].y);
	if (dist > 1.1 ||  dist < 0.9)
	  continue;

	codebook = map->codes[nc].points;
	difference = 0.0;

	/* Compute the difference between codebook and input entry */
	for (i = 0; i < tend; i++){
	  diff = codebook[i] - sample[i];
	  difference += diff * diff * mu[i];
	}
	printf("    %d %d %lE\n", map->codes[nc].x, map->codes[nc].y,difference);
      }
    }
  }
}


/******************************************************************************
Description: 

Return value: 
******************************************************************************/
void CreateWebSOMOutput(struct Parameters parameters)
{
  int n;
  FLOAT qerr;
  struct Graph *graph;
  struct Node *node;
  struct Map *map;
  time_t t;
  int *hits;
  int root, dimlabel;
  int grid = 6;
  int x, y, x1, y1, flag;

  if (parameters.train == NULL)
    return;

  map = &parameters.map;
  dimlabel = (int)ceil((double)map->xdim/grid)*(int)ceil((double)map->ydim/grid);
  fprintf(stderr, ">>>>%d %d %d %d\n", (int)ceil(10.1), (int)ceil(10.0), (int)ceil(10.7), dimlabel);

  /* Find the winners for all nodes */
  if (KstepEnabled)
    qerr = K_Step_Approximation(map, parameters.train, 1);
  else
    qerr = GetNodeCoordinates(map, parameters.train);

  hits = (int*)MyMalloc(map->xdim*map->ydim*sizeof(int));
  t = time(NULL);
  printf("format=nodelabel,label\n");
  printf("dim_label=%d\n", dimlabel);
  printf("#Generated: %s", ctime(&t));
  printf("#Dataset: %s\n", parameters.datafile);
  printf("#Network: %s\n", parameters.inetfile);
  printf("#Network is of size: %d x %d = %d\n", map->xdim, map->ydim, map->xdim*map->ydim);
  printf("#Smoothing grid: %d\n", grid);

  for (graph = parameters.train; graph != NULL; graph = graph->next){
    memset(hits, 0, map->xdim*map->ydim*sizeof(int));
    for (n = 0; n < graph->numnodes; n++){
      node = graph->nodes[n];
      if (IsRoot(node))
	root = n;

      hits[node->x + node->y * map->xdim] = 1;
    }
    printf("graph:%s\n", graph->gname);
    for (y = 0; y < map->ydim; ){  /* For every codebook of the map */
      for (x = 0; x < map->xdim; ){  /* For every codebook of the map */
	flag = 0;
	for (x1 = x; x1 < x+grid; x1++){
	  for (y1 = y; y1 < y+grid; y1++){
	    if (x1 < map->xdim && y1 < map->ydim)
	      flag += hits[x1 + y1 * map->xdim];
	  }
	}
	x += grid;
	if (flag)
	  flag = 1;
	printf(" %d", flag);
      }
      y += grid;
    }

    printf(" %s\n", GetLabel(graph->nodes[root]->label)); /* class */
  }
}


/******************************************************************************
Description: main

Return value: zero
******************************************************************************/
int main(int argc, char **argv)
{
  UNSIGNED i, mode, maxout;
  int x = -1, y = -1;
  struct Parameters parameters;

  mode = 0;
  memset(&parameters, 0, sizeof(struct Parameters));
  for (i = 1; i < argc; i++){
    if (!strncmp(argv[i], "-cin", 2))
      GetArg(TYPE_STRING, argc, argv, i++, &parameters.inetfile);
    else if (!strncmp(argv[i], "-din", 2))
      GetArg(TYPE_STRING, argc, argv, i++, &parameters.datafile);
    else if (!strncmp(argv[i], "-tin", 2))
      GetArg(TYPE_STRING, argc, argv, i++, &parameters.testfile);
    else if (!strncmp(argv[i], "-mode", 4)){
      i++;
      if (!strcmp(argv[i], "contextual"))
	mode |= CONTEXTUAL;
      else if (!strcmp(argv[i], "context"))
	mode |= CONTEXT;
      else if (!strcmp(argv[i], "mapnodetype"))
	mode |= MAPNODETYPE;
      else if (!strcmp(argv[i], "maplabel"))
	mode |= MAPNODELABEL;
      else if (!strcmp(argv[i], "mapgraph"))
	mode |= MAPGRAPH;
      else if (!strcmp(argv[i], "mapcluster"))
	mode |= SHOWCLUSTERING;
      else if (!strcmp(argv[i], "showgraph"))
	mode |= SHOWGRAPH;
      else if (!strcmp(argv[i], "showsubgraph"))
	mode |= SHOWSUBGRAPHS;
      else if (!strcmp(argv[i], "precision"))
	mode |= PRECISION;
      else if (!strcmp(argv[i], "retrievalperformance"))
	mode |= RETRIEVALPERF;
      else if (!strcmp(argv[i], "classify"))
	mode |= CLASSIFY;
      else if (!strcmp(argv[i], "analyse"))
	mode |= ANALYSE;
      else if (!strcmp(argv[i], "balance"))
	mode |= BALANCE;
      else if (!strcmp(argv[i], "distances"))
	mode |= DISTANCES;
      else if (!strcmp(argv[i], "truncate")){
	mode |= TRUNCATE;
	if (i+1 < argc)
	  maxout = atoi(argv[++i]);
	else{
	  fprintf(stderr, "Error: Missing value for mode truncate.\nSyntax is '-mode truncate <n>'.\n");
	  exit(0);
	}
      }
      else if (!strcmp(argv[i], "websom"))
	mode |= WEBSOM;
      else
	fprintf(stderr, "Warning: Mode '%s' is not known\n", argv[i]);
    }
    //    else if (!strcmp(argv[i], "-quiet"))
    //      stderr = freopen("/dev/null", "wb", stderr);
    else if (!strcmp(argv[i], "-x"))
      GetArg(TYPE_INT, argc, argv, i++, &x);
    else if (!strcmp(argv[i], "-y"))
      GetArg(TYPE_INT, argc, argv, i++, &y);
    else if (!strcmp(argv[i], "-mu1"))
      GetArg(TYPE_FLOAT, argc, argv, i++, &parameters.mu1);
    else if (!strcmp(argv[i], "-mu2"))
      GetArg(TYPE_FLOAT, argc, argv, i++, &parameters.mu2);
    else if (!strcmp(argv[i], "-mu3"))
      GetArg(TYPE_FLOAT, argc, argv, i++, &parameters.mu3);
    else if (!strcmp(argv[i], "-mu4"))
      GetArg(TYPE_FLOAT, argc, argv, i++, &parameters.mu4);
    else if (!strncmp(argv[i], "-verbose", 5))
      parameters.verbose = 1;
    else if (!strcmp(argv[i], "-undirected"))
      parameters.undirected = 1;
    else if (!strcmp(argv[i], "-help") || !strcmp(argv[i], "-h") || !strcmp(argv[i], "-?")){
      Usage();
    }
    else
      fprintf(stderr, "Warning: Ignoring unrecognized command line option '%s'\n", argv[i]);

    if (CheckErrors() != 0)
      break;
  }

  if (parameters.verbose != 0){
    PrintSoftwareInfo(stderr); /* Print a nice header with software and */
    PrintSystemInfo(stderr);   /* hardware information */
  }

  /* Check that we have useful initial data */
  if (mode == 0)
    AddError("No test mode given. Nothing to do.");

  if (CheckErrors() == 0 && parameters.datafile)    /* No errors so far ... */
    parameters.train = LoadData(parameters.datafile); /* Load the dataset */

  if (CheckErrors() == 0 && parameters.testfile != NULL)
    parameters.test = LoadData(parameters.testfile); /* Load the test set */

  if (CheckErrors() == 0 && parameters.inetfile)    /* No errors so far ... */
    LoadMap(&parameters);      /* Load map data */

  if (CheckErrors() == 0 && parameters.undirected) /* treat as undirected */
    ConvertToUndirectedLinks(parameters.train);

  if (CheckErrors() == 0)      /* No errors so far ... */
    PrepareData(&parameters); /* Prepare data for processing*/

  if (parameters.train->FanIn > 0){
    fprintf(stderr, "Contextual data detected. K-step approximation enabled.\n");
    KstepEnabled = 1;
  }


  if (CheckErrors() == 0){     /* If there were no errors so far then      */
    if (mode & CONTEXTUAL)
      ClassifyAndWriteDatafile(parameters.map, parameters.train);
    if (mode & CONTEXT)
      ClassifyAndWriteDatafileWithParents(parameters.map, parameters.train);
    if (mode & MAPNODELABEL)
      MapNodeLabel(parameters.map, parameters.train);
    if (mode & MAPNODETYPE)
      ;
    if (mode & MAPGRAPH)
      MapGraph(parameters, x, y);
    if (mode & SHOWGRAPH)
      VisualizeGraph(parameters);
    if (mode & SHOWCLUSTERING)
      VisualizeClustering(parameters);
    if (mode & SHOWSUBGRAPHS)
      VisualizeSubGraphs(parameters);
    if (mode & PRECISION)
      ComputePrecision(parameters);
    if (mode & RETRIEVALPERF)
      Compute(parameters, 0);
    if (mode & CLASSIFY)
      ComputeRetrievalPerformance(parameters, 1);
    if (mode & ANALYSE)
      AnalyseDataset(parameters);
    //      AnalyseGraphs(parameters);
    if (mode & BALANCE)
      BalanceGraphs(parameters);
    if (mode & DISTANCES)
      ListDistances(parameters);
    if (mode & TRUNCATE)
      Truncate(parameters, maxout);
    if (mode & WEBSOM)
      CreateWebSOMOutput(parameters);
  }

  Cleanup(&parameters);         /* Free allocated memory and flush errors */

  if (parameters.verbose != 0)
    fprintf(stderr, "all done.\n");

  return 0;
}

/* End of file */
