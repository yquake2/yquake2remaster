#include <GL/gl.h>
#include "render.h"

extern vec3_t model_center;

void PaletteInit(void);
void upload_texture(GLubyte *pixels, int width, int height);
void reshape(int w, int h);
