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

#ifdef USE_SDL3
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#endif

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

		R_Printf(PRINT_ALL, "Supported keys:\n");
		R_Printf(PRINT_ALL, "  ESC, q - Quit\n");
		R_Printf(PRINT_ALL, "  0      - Next mesh\n");
		R_Printf(PRINT_ALL, "  =      - Next skin\n");
		R_Printf(PRINT_ALL, "  -      - Previous skin\n");
		R_Printf(PRINT_ALL, "  w / s  - Rotate X (+5 / -5)\n");
		R_Printf(PRINT_ALL, "  d / a  - Rotate Y (+5 / -5)\n");
		R_Printf(PRINT_ALL, "  z / c  - Rotate Z (+5 / -5)\n");
		R_Printf(PRINT_ALL, "  x      - Next render mode\n");
		R_Printf(PRINT_ALL, "  v / b  - Zoom out / in\n");

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

int
main(int argc, char *argv[])
{
	SDL_Window *window;
	SDL_GLContext context;
	int running = 1;
	static int n = 0; /* The current frame */
	static float interp = 0.0;
	double curent_time = 0;
	double last_time = 0;

	if (argc < 2)
	{
		R_Printf(PRINT_ALL, "usage: %s <filename.md2> [<texture.png>]\n", argv[0]);
		return 0;
	}

	// quake swap prepere
	Swap_Init();

#ifdef USE_SDL3
	if (!SDL_Init(SDL_INIT_VIDEO))
#else
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
#endif
	{
		fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
		return 1;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);

#ifdef USE_SDL3
	window = SDL_CreateWindow("MD2/MDL/MD5 Model", 640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
#else
	window = SDL_CreateWindow("MD2/MDL/MD5 Model", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
#endif
	context = window ? SDL_GL_CreateContext(window) : NULL;
	if (!window || !context)
	{
		fprintf(stderr, "OpenGL window creation failed: %s\n", SDL_GetError());
		if (context)
		{
#ifdef USE_SDL3
			SDL_GL_DestroyContext(context);
#else
			SDL_GL_DeleteContext(context);
#endif
		}
		if (window)
		{
			SDL_DestroyWindow(window);
		}
		SDL_Quit();
		return 1;
	}

	init(argv[1]);

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

	last_time = (double)SDL_GetTicks() / 1000.0;

	while (running)
	{
		SDL_Event event;
		int width, height;

		while (SDL_PollEvent(&event))
		{
#ifdef USE_SDL3
			SDL_Keycode key = event.key.key;
#else
			SDL_Keycode key = event.key.keysym.sym;
#endif

#ifdef USE_SDL3
			if (event.type == SDL_EVENT_QUIT || (event.type == SDL_EVENT_KEY_DOWN && key == SDLK_ESCAPE))
#else
			if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && key == SDLK_ESCAPE))
#endif
			{
				running = 0;
			}
#ifdef USE_SDL3
			else if (event.type == SDL_EVENT_KEY_DOWN)
#else
			else if (event.type == SDL_KEYDOWN)
#endif
			{
				iskin += skin_count;
				xrotate += 360;
				yrotate += 360;
				zrotate += 360;

				switch (key)
				{
					case '0':
						imesh++;
						break;

					case '=':
						iskin++;
						break;

					case '-':
						iskin--;
						break;

					case 'w':
						xrotate += 5;
						break;

					case 's':
						xrotate -= 5;
						break;

					case 'd':
						yrotate += 5;
						break;

					case 'a':
						yrotate -= 5;
						break;

					case 'z':
						zrotate += 5;
						break;

					case 'c':
						zrotate -= 5;
						break;

					case 'x':
						irender++;
						break;

					case 'v':
						zdist -= 5;
						break;

					case 'b':
						zdist += 5;
						break;

					case 'q':
						running = 0;
						break;
				}

				if (skin_count > 0)
				{
					iskin %= skin_count;
				}
				xrotate %= 360;
				yrotate %= 360;
				zrotate %= 360;
			}
		}

		curent_time = (double)SDL_GetTicks() / 1000.0;
		/* Animate model from frames 0 to num_frames-1 */
		interp += 10 * (curent_time - last_time);
		last_time = curent_time;

		Animate(0, mem_mod->num_frames - 1, &n, &interp);

#ifdef USE_SDL3
		SDL_GetWindowSizeInPixels(window, &width, &height);
#else
		SDL_GetWindowSize(window, &width, &height);
#endif
		reshape(width, height);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glLoadIdentity();

		glTranslatef(
			0.0f, /* x */
			0.0f, /* y */
			zdist /* z */
		);
		glRotatef(xrotate, 1.0, 0.0, 0.0);
		glRotatef(yrotate, 0.0, 1.0, 0.0);
		glRotatef(zrotate, 0.0, 0.0, 1.0);

		/* Draw the model */
		RenderFrameItp(n, interp, mem_mod);

		SDL_GL_SwapWindow(window);
	}

	cleanup();

#ifdef USE_SDL3
	SDL_GL_DestroyContext(context);
#else
	SDL_GL_DeleteContext(context);
#endif
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
