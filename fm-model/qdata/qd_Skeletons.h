#ifndef QD_SKELETONS_H
#define QD_SKELETONS_H

#include "Placement.h"

typedef float vec3_t[3];
typedef double vec3d_t[3];
typedef int qboolean;


typedef struct Placement_d_s
{
	vec3d_t origin;
	vec3d_t direction;
	vec3d_t up;
} Placement_d_t;

typedef struct QD_SkeletalJoint_s
{
	Placement_d_t placement;
	vec3d_t		rotation;
} QD_SkeletalJoint_t;

#define NUM_CLUSTERS 8

typedef struct IntListNode_s
{
	int data;
	struct IntListNode_s *next;
} IntListNode_t;  // gaak

typedef struct Skeletalfmheader_s
{
	int type;
	int clustered;
	int references;

	int *clusters[NUM_CLUSTERS];
	IntListNode_t *vertLists[NUM_CLUSTERS];
	int num_verts[NUM_CLUSTERS + 1];
	int new_num_verts[NUM_CLUSTERS + 1];

	float scaling[3];
	float rotation[3];
	float translation[3];
} Skeletalfmheader_t;

#define SKELETAL_NAME_MAX 32

extern Skeletalfmheader_t g_skelModel;

void ClearSkeletalModel();
void GrabModelTransform(char *frame);
void GrabSkeletalFrame(char *frame);
void GrabReferencedFrame(char *frame);

// Reference Stuff
#define NUM_REFERENCES 8

#define REF_MAX_POINTS	16
#define REF_MAX_STRLEN	32

// We're assuming no more than 16 reference points, with no more than 32 characters in the name
extern char RefPointNameList[REF_MAX_POINTS][REF_MAX_STRLEN];
extern int	RefPointNum;

#endif
