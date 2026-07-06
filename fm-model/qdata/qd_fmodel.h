#ifndef FMODEL_H
#define FMODEL_H
#include "fmodel.h"
#endif
#include "qd_Skeletons.h"

typedef float vec3_t[3];

typedef struct
{
	int		numnormals;
	vec3_t	normalsum;
} fmvertexnormals_t;

typedef struct
{
	vec3_t				v;
	int					lightnormalindex;
	fmvertexnormals_t	vnorm;
} fmtrivert_t;

#define FRAME_NAME_LEN (16)

typedef struct
{
	vec3_t		mins, maxs;
	char		name[FRAME_NAME_LEN];
	fmtrivert_t	v[MAX_FM_VERTS];
	struct QD_SkeletalJoint_s joints[NUM_CLUSTERS];
	struct QD_SkeletalJoint_s references[NUM_REFERENCES];
} fmframe_t;

extern fmframe_t	g_frames[MAX_FM_FRAMES];

extern fmheader_t	fmheader;
extern char			cdarchive[1024];				// set by $fmcd
extern char			cdpartial[1024];				// set by $fmcd
extern char			cddir[1024];					// set by $fmcd

void GrabFrame (char *frame);
void H_printf(char *fmt, ...);
char *FindFrameFile (char *frame);
