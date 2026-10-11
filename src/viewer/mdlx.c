/*
 * mdx.c -- mdx model loader
 * last modification: aug. 14, 2007
 *
 * Copyright (c) 2005-2007 David HENRY
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "rendergl.h"

void
show_frames(const dmdx_t *mod)
{
	const byte *pframes;
	int i;

	pframes = (byte*)mod + mod->ofs_frames;

	for (i = 0; i < mod->num_frames; i++)
	{
		const daliasxframe_t *pframe;

		pframe = (const daliasxframe_t*)(pframes + mod->framesize * i);

		printf("#define FRAME_%s\t%d\n", pframe->name, i);
	}
}

static void
init(const char *filename)
{
	model_load(filename);
	if (mem_mod)
	{
		GLfloat lightpos[] = { 5.0f, 10.0f, 0.0f, 1.0f };

		show_frames(mem_mod);

		/* Initialize OpenGL context */
		glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
		glShadeModel(GL_SMOOTH);

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);

		glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

		return;
	}
	else
	{
		R_Printf(PRINT_ALL, "Unknown model format? \"%s\"!\n", filename);
	}

	exit (EXIT_FAILURE);
}

static void
display ()
{
	static int n = 0; /* The current frame */
	static float interp = 0.0;
	static double curent_time = 0;
	static double last_time = 0;

	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity ();

	last_time = curent_time;
	curent_time = (double)glutGet (GLUT_ELAPSED_TIME) / 1000.0;

	/* Animate model from frames 0 to num_frames-1 */
	interp += 10 * (curent_time - last_time);
	Animate(0, mem_mod->num_frames - 1, &n, &interp);

	glTranslatef(
		0.0f, /* x */
		0.0f, /* y */
		zdist /* z */
	);
	glRotatef (xrotate, 1.0, 0.0, 0.0);
	glRotatef (yrotate, 0.0, 1.0, 0.0);
	glRotatef (zrotate, 0.0, 0.0, 1.0);

	/* Draw the model */
	RenderFrameItp(n, interp, mem_mod);
	// my_stbtt_print(0, 0, "abcde ЀЀЃґуля, з'їсти, істота, Європа");

	glutSwapBuffers ();
	glutPostRedisplay ();
}

int
main(int argc, char *argv[])
{
	if (argc < 2)
	{
		R_Printf(PRINT_ALL, "usage: %s <filename.md2> [<texture.png>]\n", argv[0]);
		return 0;
	}

	// quake swap prepere
	Swap_Init();

	glutInit (&argc, argv);
	glutInitDisplayMode (GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize (640, 480);
	glutCreateWindow ("MD2/MDL/MD5 Model");

	atexit (cleanup);
	init (argv[1]);

	my_stbtt_initfont();
	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);

	if (argc > 2)
	{
		int w, h, b;
		byte *data = image_load(argv[2], &w, &h, &b);
		if (!data)
		{
			R_Printf(PRINT_ALL, "%s: failed loading!\n", argv[2]);
		}
		else
		{
			R_Printf(PRINT_ALL, "Checks %dx%d %d\n", w, h, b);
			upload_texture(data, w, h);
			free(data);
		}
	}

	glutMainLoop ();

	return 0;
}
