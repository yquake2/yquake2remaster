#include <GL/gl.h>
#include "render.h"

void my_stbtt_print(float x, float y, char *text);
void my_stbtt_initfont(void);
void upload_texture(GLubyte *pixels, int width, int height);
void reshape(int w, int h);
