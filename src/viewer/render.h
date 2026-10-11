#include "../common/header/shared.h"
#include "../common/header/files.h"
#include "../client/refresh/ref_shared.h"

/* GL command packet in memory */
typedef struct mem_glcmd_s
{
	float s;
	float t;
	int index; /* vertex index */
} mem_glcmd_t;

extern dmdx_t *mem_mod;
extern int skin_count, iskin, yrotate, xrotate, zrotate, imesh, zdist, irender;

struct image_s* find_image(const char *name, imagetype_t type);
void Animate(int start, int end, int *frame, float *interp);
void RenderFrameItp(int n, float interp, const dmdx_t *mod);
struct image_s * insert_skin(const char *name, byte *data, int width, int realwidth,
	int height, int realheight, size_t size, imagetype_t type, int bits);
byte* image_load(const char* name, int *w, int *h, int *b);
void keyboard(unsigned char key, int x, int y);
void cleanup(void);
void model_load(const char *filename);
