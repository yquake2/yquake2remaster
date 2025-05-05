/*
 * Copyright (C) 1997-2001 Id Software, Inc.
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
 * Shared code for generate GL shaders
 *
 * =======================================================================
 */

#include "../ref_shared.h"

const char*
glshader_version(int major_version, int minor_version)
{
#ifdef YQ2_GL3_GLES3
	if (major_version == 2)
	{
		return "#version 100\nprecision mediump float;\n";
	}
	else if (major_version == 3)
	{
		switch(minor_version)
		{
			case 0: return "#version 300 es\nprecision mediump float;\n";
			case 1: return "#version 310 es\nprecision mediump float;\n";
			case 2: return "#version 320 es\nprecision mediump float;\n";
			default: return "#version 320 es\nprecision mediump float;\n";
		}
	}
#else // Desktop GL
	if (major_version == 2)
	{
		switch(minor_version)
		{
			case 0: return "#version 110\n";
			case 1: return "#version 120\n";
			default: return "#version 120\n";
		}
	}
	else if (major_version == 3)
	{
		switch(minor_version)
		{
			case 0: return "#version 130\n";
			case 1: return "#version 140\n";
			case 2: return "#version 150\n";
			case 3: return "#version 330\n";
			default: return "#version 330\n";
		}
	}
	else if (major_version == 4)
	{
		switch(minor_version)
		{
			case 0: return "#version 400\n";
			case 1: return "#version 410\n";
			case 2: return "#version 420\n";
			case 3: return "#version 430\n";
			case 4: return "#version 440\n";
			case 5: return "#version 450\n";
			case 6: return "#version 460\n";
			default: return "#version 460\n";
		}
	}
#endif

	/* some unknown version */
	return "#version 100\nprecision mediump float;\n";
}
