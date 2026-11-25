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
 * Multithreaded workers and thread pool based on SDL
 *
 * =======================================================================
 */

#ifndef DEDICATED_ONLY
#ifdef USE_SDL3
#include <SDL3/SDL.h>
typedef SDL_Mutex q2_mutex_t;
typedef SDL_Condition q2_cond_t;
#define Q2_CreateMutex SDL_CreateMutex
#define Q2_DestroyMutex SDL_DestroyMutex
#define Q2_LockMutex SDL_LockMutex
#define Q2_UnlockMutex SDL_UnlockMutex
#define Q2_CreateCond SDL_CreateCondition
#define Q2_DestroyCond SDL_DestroyCondition
#define Q2_SignalCond SDL_SignalCondition
#define Q2_BroadcastCond SDL_BroadcastCondition
#define Q2_WaitCond SDL_WaitCondition
#else
#include <SDL2/SDL.h>
typedef SDL_mutex q2_mutex_t;
typedef SDL_cond q2_cond_t;
#define Q2_CreateMutex SDL_CreateMutex
#define Q2_DestroyMutex SDL_DestroyMutex
#define Q2_LockMutex SDL_LockMutex
#define Q2_UnlockMutex SDL_UnlockMutex
#define Q2_CreateCond SDL_CreateCond
#define Q2_DestroyCond SDL_DestroyCond
#define Q2_SignalCond SDL_CondSignal
#define Q2_BroadcastCond SDL_CondBroadcast
#define Q2_WaitCond SDL_CondWait
#endif
#endif

#include "header/common.h"

#ifndef DEDICATED_ONLY
#ifndef R_MAX_THREADS
#define R_MAX_THREADS 16
#endif

typedef struct
{
	size_t row_start;
	size_t row_end;
	void (*fn)(size_t start, size_t end, void *user);
	void *user;
} rowjob_t;

static int threads_num = -1;
static qboolean pool_initialized = false;
static q2_mutex_t *pool_mutex = NULL;
static q2_cond_t *pool_cond_work = NULL;
static q2_cond_t *pool_cond_done = NULL;
static SDL_Thread *pool_workers[R_MAX_THREADS];
static int pool_num_workers = 0;
static qboolean pool_shutdown = false;
static qboolean pool_has_work = false;

static rowjob_t pool_jobs[R_MAX_THREADS];
static size_t pool_job_count = 0;
static size_t pool_next_job = 0;
static size_t pool_jobs_completed = 0;

static unsigned
R_NumHWThreads(void)
{
	unsigned u;
	int n;

	if (threads_num > 0)
	{
		return threads_num;
	}

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

	threads_num = u;

	return u;
}

static int
R_ThreadPoolWorker(void *data)
{
	(void)data;

	while (1)
	{
		size_t job_index = (size_t)-1;

		Q2_LockMutex(pool_mutex);
		while (!pool_has_work && !pool_shutdown)
		{
			Q2_WaitCond(pool_cond_work, pool_mutex);
		}

		if (pool_shutdown)
		{
			Q2_UnlockMutex(pool_mutex);
			break;
		}

		if (pool_next_job < pool_job_count)
		{
			job_index = pool_next_job++;
		}

		Q2_UnlockMutex(pool_mutex);

		if (job_index != (size_t)-1)
		{
			const rowjob_t *j = &pool_jobs[job_index];
			j->fn(j->row_start, j->row_end, j->user);

			Q2_LockMutex(pool_mutex);
			pool_jobs_completed++;
			if (pool_jobs_completed == pool_job_count)
			{
				Q2_SignalCond(pool_cond_done);
			}
			Q2_UnlockMutex(pool_mutex);
		}
	}

	return 0;
}

static void
R_InitThreadPool(unsigned int threads)
{
	unsigned int i;

	if (pool_initialized)
	{
		return;
	}

	pool_mutex = Q2_CreateMutex();
	pool_cond_work = Q2_CreateCond();
	pool_cond_done = Q2_CreateCond();

	if (!pool_mutex || !pool_cond_work || !pool_cond_done)
	{
		Com_Printf("%s: Failed to create mutex or condition variables: %s\n",
			__func__, SDL_GetError());
		if (pool_mutex) { Q2_DestroyMutex(pool_mutex); pool_mutex = NULL; }
		if (pool_cond_work) { Q2_DestroyCond(pool_cond_work); pool_cond_work = NULL; }
		if (pool_cond_done) { Q2_DestroyCond(pool_cond_done); pool_cond_done = NULL; }
		return;
	}

	pool_num_workers = (threads > 1) ? (int)(threads - 1) : 0;
	if (pool_num_workers > R_MAX_THREADS - 1)
	{
		pool_num_workers = R_MAX_THREADS - 1;
	}

	pool_shutdown = false;
	pool_has_work = false;

	for (i = 0; i < (unsigned int)pool_num_workers; ++i)
	{
		char name[32];
		Com_sprintf(name, sizeof(name), "Q2Worker_%u", i);
		pool_workers[i] = SDL_CreateThread(R_ThreadPoolWorker, name, NULL);
		if (!pool_workers[i])
		{
			Com_Printf("%s: Failed to create worker thread %u: %s\n",
				__func__, i, SDL_GetError());
			pool_num_workers = (int)i;
			break;
		}
	}

	pool_initialized = true;
}

void
R_ShutdownThreadPool(void)
{
	int i;

	if (!pool_initialized)
	{
		return;
	}

	Q2_LockMutex(pool_mutex);
	pool_shutdown = true;
	Q2_BroadcastCond(pool_cond_work);
	Q2_UnlockMutex(pool_mutex);

	for (i = 0; i < pool_num_workers; ++i)
	{
		if (pool_workers[i])
		{
			SDL_WaitThread(pool_workers[i], NULL);
			pool_workers[i] = NULL;
		}
	}

	if (pool_mutex)
	{
		Q2_DestroyMutex(pool_mutex);
		pool_mutex = NULL;
	}
	if (pool_cond_work)
	{
		Q2_DestroyCond(pool_cond_work);
		pool_cond_work = NULL;
	}
	if (pool_cond_done)
	{
		Q2_DestroyCond(pool_cond_done);
		pool_cond_done = NULL;
	}

	pool_initialized = false;
	pool_num_workers = 0;
}
#endif

void
R_ParallelTasks(size_t rows, size_t min_rows_per_task,
	void (*fn)(size_t, size_t, void *), void *user, const char *func_name)
{
#ifndef DEDICATED_ONLY
	size_t chunk, t, jobcount, threads;

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

	if (!pool_initialized)
	{
		R_InitThreadPool(threads);
	}

	Q2_LockMutex(pool_mutex);
	while (pool_has_work)
	{
		Q2_WaitCond(pool_cond_done, pool_mutex);
	}

	pool_job_count = 0;
	for (t = 0; t < threads; ++t)
	{
		size_t start, end;

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

		pool_jobs[pool_job_count].row_start = start;
		pool_jobs[pool_job_count].row_end = end;
		pool_jobs[pool_job_count].fn = fn;
		pool_jobs[pool_job_count].user = user;

		++pool_job_count;
	}

	jobcount = pool_job_count;
	pool_next_job = 0;
	pool_jobs_completed = 0;
	pool_has_work = true;

	Q2_BroadcastCond(pool_cond_work);

	while (1)
	{
		size_t job_index = (size_t)-1;

		if (pool_next_job < pool_job_count)
		{
			job_index = pool_next_job++;
		}

		if (job_index != (size_t)-1)
		{
			Q2_UnlockMutex(pool_mutex);

			const rowjob_t *j = &pool_jobs[job_index];
			j->fn(j->row_start, j->row_end, j->user);

			Q2_LockMutex(pool_mutex);
			pool_jobs_completed++;
			if (pool_jobs_completed == pool_job_count)
			{
				pool_has_work = false;
				Q2_SignalCond(pool_cond_done);
				Q2_UnlockMutex(pool_mutex);
				break;
			}
		}
		else
		{
			if (pool_jobs_completed == pool_job_count)
			{
				pool_has_work = false;
				Q2_UnlockMutex(pool_mutex);
				break;
			}
			Q2_WaitCond(pool_cond_done, pool_mutex);
			if (pool_jobs_completed == pool_job_count)
			{
				pool_has_work = false;
				Q2_UnlockMutex(pool_mutex);
				break;
			}
		}
	}

	Com_DPrintf("%s: %s threads " YQ2_COM_PRIdS " # " YQ2_COM_PRIdS "/" YQ2_COM_PRIdS "\n",
		__func__, func_name, jobcount, min_rows_per_task, rows);
#else
	fn(0, rows, user);
#endif
}
