void LoadGlobals(char *fileName);
void LoadClusters(char *fileName, int **clusterList, int *num_verts, int skel_type);
void LoadJointList(char *fileName, struct QDataJoint_s *jointList, int num_verts);

#define NUM_CLUSTERS 8

#define NOT_JOINTED -1

#include "Joints.h"

