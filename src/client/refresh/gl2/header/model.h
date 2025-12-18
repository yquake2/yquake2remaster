/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

/*

d*_t structures are on-disk representations
m*_t structures are in-memory

*/

/*
==============================================================================

BRUSH MODELS

==============================================================================
*/

#define	DEFAULT_LMSHIFT	4

//
// in memory representation
//
// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct
{
	vec3_t		position;
} mvertex_t;

typedef struct
{
	vec3_t		mins, maxs;
	vec3_t		origin;		// for sounds or lights
	float		radius;
	int			headnode;
	int			visleafs;		// not including the solid leaf 0
	int			firstface, numfaces;
} mmodel_t;


#define	SIDE_FRONT	0
#define	SIDE_BACK	1
#define	SIDE_ON		2


// braxi -- added unused SURF_ flags for completness
#define	SURF_PLANEBACK		2
#define	SURF_DRAWSKY		4
//#define SURF_DRAWSPRITE	8 // was never used ?
#define SURF_DRAWTURB		0x10
//#define SURF_DRAWTILED	0x20
//#define SURF_DRAWBACKGROUND	0x40 // used in soft renderer
#define SURF_UNDERWATER		0x80

typedef struct
{
	unsigned int	v[2];
} medge_t;

typedef struct mtexinfo_s
{
	float		vecs[2][4];
	int			flags;
	int			numframes;
	struct mtexinfo_s	*next;		// animation chain
	image_t		*image;
} mtexinfo_t;

typedef struct polyvert_s
{
	vec3_t	pos;
	float	alpha;
	float	texCoord[2];
	float	lmTexCoord[2];  // lightmap texture coordinate (sometimes unused)
	vec3_t	normal;
} polyvert_t;

typedef struct glpoly_s
{
	struct	glpoly_s	*next;
	struct	glpoly_s	*chain;
	int		numverts;
	int		flags;					// for SURF_UNDERWATER (not needed anymore?)
	polyvert_t	verts[4]; // variable sized
} poly_t;

typedef struct msurface_s
{
	int			visframe;		// should be drawn when node is crossed

	cplane_t	*plane;
	int			flags;

	int			firstedge;	// look up in model->surfedges[], negative numbers are backwards edges
	int			numedges;

	int			firstvert;	// exp rendering
	int			numverts;	// exp rendering
	
	short		texturemins[2];
	short		extents[2];

	int			light_s, light_t;	// gl lightmap coordinates

	poly_t		*polys;				// multiple if warped
	struct msurface_s	*texturechain;

	mtexinfo_t	*texinfo;
	
	// DECOUPLED_LM
	unsigned short	lm_width;
	unsigned short	lm_height;
	float		lmvecs[2][4];
	float		lmvlen[2];
	short		lmshift;

	// lighting info
	int				dlightframe;
	unsigned int	dlightbits;

	int			lightMapTextureNum;
	byte		styles[MAX_LIGHTMAPS_PER_SURFACE];
//	float		cached_light[MAX_LIGHTMAPS_PER_SURFACE];	// values currently used in lightmap
	byte		*samples;		// ptr to lightmap data [numstyles*surfsize]
} msurface_t;

typedef struct mnode_s
{
// common with leaf
	int			contents;		// -1, to differentiate from leafs
	int			visframe;		// node needs to be traversed if current
	
	// for bounding box culling
	vec3_t		mins, maxs;

	struct mnode_s	*parent;

// node specific
	cplane_t	*plane;
	struct mnode_s	*children[2];	

	unsigned int		firstsurface; // braxi -- was unsigned short
	unsigned int		numsurfaces; // braxi -- was unsigned short
} mnode_t;



typedef struct mleaf_s
{
// common with node
	int			contents;		// wil be a negative contents number
	int			visframe;		// node needs to be traversed if current

	// for bounding box culling
	vec3_t		mins, maxs;		

	struct mnode_s	*parent;

// leaf specific
	int			cluster;
	int			area;

	msurface_t	**firstmarksurface;
	int			nummarksurfaces;
} mleaf_t;


//===================================================================

typedef struct model_s
{
	char		name[MAX_QPATH];
	int			index;		// our index to model array

	int			registration_sequence;

	modtype_t	type;

	int			numframes;
	
	int			flags;

//
// volume occupied by the model graphics
//		
	vec3_t		mins, maxs;
	float		radius;

//
// solid volume for clipping 
//
	qboolean	clipbox;
	vec3_t		clipmins, clipmaxs;

//
// brush model
//

	int			firstmodelsurface, nummodelsurfaces;
	int			lightmap;		// only for inlineModels

	int			numInlineModels;
	mmodel_t	*inlineModels;

	int			numplanes;
	cplane_t	*planes;

	int			numleafs;		// number of visible leafs, not counting 0
	mleaf_t		*leafs;

	int			numvertexes;
	mvertex_t	*vertexes;

	int			numedges;
	medge_t		*edges;

	int			numnodes;
	int			firstnode;
	mnode_t		*nodes;

	int			numtexinfo;
	mtexinfo_t	*texinfo;

	int			numsurfaces;
	msurface_t	*surfaces;

	int			numsurfedges;
	int			*surfedges;

	int			nummarksurfaces;
	msurface_t	**marksurfaces;

	dbsp_vis_t		*vis;

	byte		*lightdata;
	int			lightdatasize;

//
// for alias models and sprites
//
	image_t		*images[32];

	int			extradatasize;
	void		*extradata;

	int			cullDist;	// don't draw if farther than this

	md3Header_t	*md3[MD3_MAX_LODS];	// only if type == MOD_MD3
	vertexbuffer_t *vb[MD3_MAX_SURFACES];
} model_t;

//============================================================================

void	R_InitModels();
void	R_FreeAllModels();
void	R_FreeModel(model_t* mod);

model_t *R_ModelForName (char *name, qboolean crash);
mleaf_t* Mod_BSP_PointInLeaf(vec3_t p, model_t* model); // mleaf_t* Mod_BSP_PointInLeaf(float* p, model_t* model);
byte	*Mod_BSP_ClusterPVS (int cluster, model_t *model);

void	Cmd_modellist_f (void);

void	*Hunk_Begin (int maxsize, char *name);
void	*Hunk_Alloc (int size);
int		Hunk_End (void);
void	Hunk_Free (void *base);


