#ifndef __D3DGLOBAL__
#define __D3DGLOBAL__

#define PI 3.141592654

#define	ANIM_FORWARD	0
#define	ANIM_BACKWARD	1
#define	ANIM_PINGPONG	2
#define	ANIM_PUNCH		3

#define PALTYPE_RAW  0
#define PALTYPE_PCX  1
#define PALTYPE_JASC 2
#define PALTYPE_MS   3
#define PALTYPE_BMP  4

#define PAL_QUAKE		0
#define PAL_QUAKE2		1
#define PAL_HEXEN2		2
#define PAL_FROMFILE	3

typedef unsigned char  u_char;

typedef float scalar_t;
typedef BOOL qboolean;

typedef struct
{
	scalar_t  x;
	scalar_t  y;
	scalar_t  z;
} vec3_t;

typedef struct
{
	scalar_t  x;
	scalar_t  y;
	scalar_t  z;
	scalar_t  s;
	scalar_t  t;
} vec5_t;

extern LPDIRECTDRAW lpDD;
//extern LPDIRECT3DRM2 d3drm2;
extern LPDIRECT3DRMFRAME scene;

extern LPDIRECT3DRMMESH	bBox;

extern HTREEITEM master;
//unsigned int interOptions = 0;
extern double interThresh;
//UINT interCalcState = 0;
extern BOOL interNext;

typedef struct
{
	HTREEITEM	head;
	int			bFrame, eFrame;
} treehead_t;

typedef struct
{
	char id[16];
} framegroup_t;

extern int playMode;

//Multiple Selection Frame Structures

typedef struct
{
	int frameIndex;
	BOOL start, end;
} frameMember;

typedef struct
{
	int numFrames;
	int info;
	frameMember *frames;
} frameStruct;

extern frameStruct  frameSels;
extern BOOL			usePunch;

extern BOOL PICK_ON;

typedef struct
{
	unsigned char r;
	unsigned char g;
	unsigned char b;
} pal_t;

typedef float	vec_t[3];

typedef struct Placement_s
{
	vec_t origin;
	vec_t direction;
	vec_t up;
} Placement_t;

typedef struct M_SkeletalJoint_s
{
	int children;		// must be the first field
	Placement_t model;	// relative to the model, used for dynamic software rotation
	Placement_t parent;	// relative to the parent joint (or model in case of root joint), used for
						// inverse kinematics
	qboolean inUse;
} M_SkeletalJoint_t;

typedef struct ArrayedListNode_s
{
	int data;
	int next;
	int inUse;
} ArrayedListNode_t;

typedef struct ModelSkeleton_s
{
	M_SkeletalJoint_t rootJoint[8];
	ArrayedListNode_t rootNode[64];
} ModelSkeleton_t;

#define PAL_SIZE		256

typedef struct
{
	unsigned char r,g,b;
} paletteRGB_t;

extern BOOL LSHFT_DN, ZOOM_ON, PICK_ON;

#endif

//__D3DGLOBAL__
