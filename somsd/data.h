#ifndef DATA_H_DEFINED
#define DATA_H_DEFINED

UNSIGNED AddLabel(char *label);
char* GetLabel(UNSIGNED index);
UNSIGNED GetNumLabels();
int *GetSortedLabelIndex();
void ClearLabels();
void UpdateChildrensLocation(struct Graph *gptr, struct Node *node);
void UpdateChildrensLocationVQ(struct Graph *gptr, struct Node *node);
void UpdateChildrenAndParentLocation(struct Graph *gptr, struct Node *node);
void UpdateAllChildrensLocation(struct Graph *graphs);
void PrepareData(struct Parameters *parameters);
struct Graph *RandomizeGraphOrder(struct Graph *graph);
FLOAT K_Step_Approximation(struct Map *map, struct Graph *gptr, int mode);
FLOAT GetNodeCoordinates(struct Map *map, struct Graph *gptr);
void SetNodeDepth(struct Graph *gptr);
void IncreaseDimension(struct Graph *graph, int newdim, int component);
void ConvertToUndirectedLinks(struct Graph *train);
UNSIGNED IsRoot(struct Node *node);
UNSIGNED IsLeaf(struct Node *node);
UNSIGNED IsIntermediate(struct Node *node);
UNSIGNED GetNodeType(struct Node *node);
void FreeGraphs(struct Graph *graph);
void FreeMap(struct Map *map);
void ClearParameters(struct Parameters* parameters); /* Free all memory      */
void Cleanup(struct Parameters* parameters);


#endif
