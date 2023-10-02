#define	VERTEXSIZE	7

/* in memory representation */

typedef struct glpoly_s
{
	struct  glpoly_s *next;
	struct  glpoly_s *chain;
	int numverts;
	int flags; /* for SURF_UNDERWATER (not needed anymore?) */
	float verts[4][VERTEXSIZE]; /* variable sized (xyz s1t1 s2t2) */
} mpoly_t;

typedef struct msurface_s
{
	int visframe; /* should be drawn when node is crossed */

	cplane_t *plane;
	int flags;

	int firstedge;          /* look up in model->surfedges[], negative numbers */
	int numedges;           /* are backwards edges */

	short texturemins[2];
	short extents[2];
	short lmshift;

	int light_s, light_t;           /* gl lightmap coordinates */
	int dlight_s, dlight_t;         /* gl lightmap coordinates for dynamic lightmaps */

	mpoly_t *polys;                /* multiple if warped */
	struct  msurface_s *texturechain;
	struct  msurface_s *lightmapchain;

	mtexinfo_t *texinfo;

	/* decoupled lm */
	float	lmvecs[2][4];
	float	lmvlen[2];

	/* lighting info */
	int dlightframe;
	int dlightbits;

	int lightmaptexturenum;
	byte styles[MAXLIGHTMAPS];
	float cached_light[MAXLIGHTMAPS];       /* values currently used in lightmap */
	byte *samples;                          /* [numstyles*surfsize] */
} msurface_t;
