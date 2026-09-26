/*
 * Copyright (C) 2025 atsb / Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * =======================================================================
 *
 * Multithreaded workers based on SDL
 *
 * =======================================================================
 */

#ifdef USE_SDL3
#include <SDL3/SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include "header/common.h"

#ifndef R_MAX_THREADS
#define R_MAX_THREADS 8
#endif

typedef struct
{
	size_t row_start;
	size_t row_end;
	void (*fn)(size_t start, size_t end, void *user);
	void *user;
} rowjob_t;

static unsigned
R_NumHWThreads(void)
{
	unsigned u;
	int n;

#ifdef USE_SDL3
	n = SDL_GetNumLogicalCPUCores();
#else
	n = SDL_GetCPUCount();
#endif

	u = (n > 0) ? (unsigned)n : 1u;
	if (u > R_MAX_THREADS)
	{
		u = R_MAX_THREADS;
	}

	return u;
}

static int
R_RowWorker(void *p)
{
	const rowjob_t *j = (const rowjob_t *)p;
	j->fn(j->row_start, j->row_end, j->user);
	return 0;
}

void
R_ParallelTasks(size_t rows, size_t min_rows_per_task,
	void (*fn)(size_t, size_t, void *), void *user)
{
	size_t chunk, t, jobcount, threads;
	SDL_Thread *th[R_MAX_THREADS];
	rowjob_t jobs[R_MAX_THREADS];

	threads = R_NumHWThreads();

	if (threads <= 1 || rows <= min_rows_per_task)
	{
		fn(0, rows, user);
		return;
	}

	if ((threads * min_rows_per_task) > rows)
	{
		threads = rows / min_rows_per_task + 1;
	}

	chunk = (rows + threads - 1) / threads;
	jobcount = 0;

	for (t = 0; t < threads; ++t)
	{
		size_t start, end;
		char name[32];

		start = t * chunk;
		end = start + chunk;

		if (start >= rows)
		{
			break;
		}

		if (end > rows)
		{
			end = rows;
		}

		jobs[jobcount].row_start = start;
		jobs[jobcount].row_end = end;
		jobs[jobcount].fn = fn;
		jobs[jobcount].user = user;

		Com_sprintf(name, sizeof(name), "R_Worker_" YQ2_COM_PRIdS, jobcount);
		th[jobcount] = SDL_CreateThread(R_RowWorker, name, &jobs[jobcount]);

		if (!th[jobcount])
		{
			Com_Printf("%s: Failed to create thread "YQ2_COM_PRIdS": %s\n", __func__, jobcount, SDL_GetError());
			fn(start, rows, user);
			break;
		}

		++jobcount;
	}

	for (t = 0; t < jobcount; ++t)
	{
		if (th[t])
		{
			SDL_WaitThread(th[t], NULL);
		}
	}

	if (jobcount)
	{
		Com_DPrintf("%s: threads " YQ2_COM_PRIdS "\n", __func__, jobcount);
	}
}
