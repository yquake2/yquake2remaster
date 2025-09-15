/*
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 * USA.
 *
 * =======================================================================
 *
 * This file implements an interface to stb_vorbis.c for decoding
 * mp3 files. Strongly spoken this file isn't part of the sound system but
 * part of the main client. It justs converts mp3 streams into normal, raw
 * Wave stream which are injected into the backends as if they were normal
 * "raw" samples.
 *
 * =======================================================================
 */

#ifndef _WIN32
#include <sys/time.h>
#endif

#include <errno.h>

#include "../header/client.h"
#include "header/local.h"

#define MINIMP3_IMPLEMENTATION
#include "header/minimp3.h"

void
MP3_LoadAsWav(const char *filename, wavinfo_t *info, void **buffer)
{
	int total_samples = 0, allocated_samples = 0, mp3_size, size;
	mp3dec_frame_info_t frame_info;
	short *final_buffer = NULL;
	void *temp_buffer = NULL;
	unsigned char *mp3_data;
	mp3dec_t mp3d;

	*buffer = NULL;

	size = FS_LoadFile(filename, &temp_buffer);

	if (!temp_buffer)
	{
		/* No such file */
		return;
	}

	/* Initialize MP3 decoder */
	mp3dec_init(&mp3d);

	/* Estimate required buffer size dynamically */
	allocated_samples = size * sizeof(short);
	final_buffer = malloc(allocated_samples * sizeof(short));

	if (!final_buffer)
	{
		/* Allocation failed */
		FS_FreeFile(temp_buffer);
		return;
	}

	mp3_data = temp_buffer;
	mp3_size = size;

	while (mp3_size > 0)
	{
		/* Decode MP3 frame up to 1152 samples in single frame */
		int samples = mp3dec_decode_frame(&mp3d, mp3_data, mp3_size, final_buffer + total_samples, &frame_info);

		if (samples > 0)
		{
			if (total_samples == 0)
			{
				/* Initialize WAV info */
				info->rate = frame_info.hz;
				info->width = 2;
				info->channels = frame_info.channels;
				info->loopstart = -1;
				info->dataofs = 0;
			}

			total_samples += samples * frame_info.channels;

			/* Resize if buffer is too small */
			if ((total_samples + 1152) >= allocated_samples)
			{
				short *tmp;

				allocated_samples *= 2;
				tmp = realloc(final_buffer, allocated_samples * sizeof(short));
				YQ2_COM_CHECK_OOM(tmp, "realloc()",
					allocated_samples * sizeof(short))
				if (!tmp)
				{
					/* unaware about YQ2_ATTR_NORETURN_FUNCPTR? */
					FS_FreeFile(temp_buffer);
					return;
				}

				final_buffer = tmp;
			}

			mp3_data += frame_info.frame_bytes;
			mp3_size -= frame_info.frame_bytes;
		}
		else
		{
			break;
		}
	}

	if (total_samples > 0)
	{
		info->samples = total_samples;
		*buffer = Z_Malloc(info->samples * sizeof(short));
		memcpy(*buffer, final_buffer, info->samples * sizeof(short));
	}

	free(final_buffer);
	FS_FreeFile(temp_buffer);
}
