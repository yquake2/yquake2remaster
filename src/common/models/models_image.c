/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (c) 1998 Trey Harrison (SiN View)
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
 * The embeded models images
 *
 * =======================================================================
 */

#include "models.h"

static byte *
Mod_LoadSPRImage(const char *mod_name, int texture_index, byte *buffer, int modfilelen,
	int *width, int *height)
{
	const dq1sprite_t pinsprite;
	const byte *curr_pos;
	int i, size;
	byte *pic;

	if (modfilelen < sizeof(pinsprite))
	{
		Com_Printf("%s: %s has incorrect header size (%i should be " YQ2_COM_PRIdS ")\n",
				__func__, mod_name, modfilelen, sizeof(pinsprite));
		return NULL;
	}

	for (i = 0; i < sizeof(pinsprite) / sizeof(int); i++)
	{
		((int *)&pinsprite)[i] = LittleLong(((int *)buffer)[i]);
	}

	if (pinsprite.version != IDQ1SPRITE_VERSION)
	{
		Com_Printf("%s: %s has wrong version number (%i should be %i)\n",
				__func__, mod_name, pinsprite.version, ALIAS_VERSION);
		return NULL;
	}

	if (pinsprite.nframes <= texture_index)
	{
		Com_Printf("%s: Sprite %s has %d only textures\n",
			__func__, mod_name, pinsprite.nframes);
		return NULL;
	}

	curr_pos = (byte*)buffer + sizeof(dq1sprite_t);
	for (i = 0; i < pinsprite.nframes; i++)
	{
		int skin_type;

		/* skip type / int */
		/* 0 = simple, !0 = group */
		/* this program can't read models composed of group frames! */
		skin_type = LittleLong(((int *)curr_pos)[0]);
		if (skin_type)
		{
			Com_Printf("%s: model %s has unsupported skin type %d\n",
				__func__, mod_name, skin_type);
			return NULL;
		}

		*width = LittleLong(((int *)curr_pos)[3]);
		*height = LittleLong(((int *)curr_pos)[4]);
		size = (*width) * (*height);

		if (i == texture_index)
		{
			curr_pos += sizeof(int) * 5;

			pic = malloc(size);
			memcpy(pic, curr_pos, size);

			Com_DPrintf("%s Loaded embeded %s#%d image %dx%d\n",
				__func__, mod_name, texture_index, *width, *height);

			return pic;
		}

		curr_pos += sizeof(int) * 5 + size;

		if (curr_pos > ((byte *)buffer + modfilelen))
		{
			Com_Printf("%s: sprite %s is too short\n",
				__func__, mod_name);
			return NULL;
		}
	}
	return NULL;
}

static byte *
Mod_LoadBSPImage(const char *mod_name, int texture_index, byte *raw, int len,
	int *width, int *height, int *bitsPerPixel)
{
	int miptex_offset, miptex_size, texture_offset,
		image_offset, size, ident;
	byte *pic;
	dq1mipheader_t *miptextures;
	dq1miptex_t *texture;

	/* get BSP format ident */
	ident = LittleLong(*(unsigned *)raw);

	miptex_offset = LittleLong(
		((int *)raw)[LUMP_BSP29_MIPTEX * 2 + 1]); /* text info lump pos */
	miptex_size = LittleLong(
		((int *)raw)[LUMP_BSP29_MIPTEX * 2 + 2]); /* text info lump size */

	if (miptex_offset >= len)
	{
		Com_Printf("%s: Map %s has broken miptex lump\n",
			__func__, mod_name);
		return NULL;
	}

	miptextures = (dq1mipheader_t *)(raw + miptex_offset);

	if (miptextures->numtex < texture_index)
	{
		Com_Printf("%s: Map %s has %d only textures\n",
			__func__, mod_name, miptextures->numtex);
		return NULL;
	}

	texture_offset = LittleLong(miptextures->offset[texture_index]);
	if (texture_offset < 0)
	{
		Com_Printf("%s: Map %s image is not attached\n",
			__func__, mod_name);
		return NULL;
	}

	if (texture_offset > miptex_size)
	{
		Com_Printf("%s: Map %s has wrong texture position\n",
			__func__, mod_name);
		return NULL;
	}

	texture = (dq1miptex_t *)(raw + miptex_offset + texture_offset);
	*width = LittleLong(texture->width);
	*height = LittleLong(texture->height);
	image_offset = LittleLong(texture->offset1);
	size = (*width) * (*height);

	if (image_offset <= 0)
	{
		Com_Printf("%s: Map %s is external image\n",
			__func__, mod_name);
		return NULL;
	}

	if ((size < 0) ||
		(image_offset > miptex_size) ||
		((image_offset + size) > miptex_size))
	{
		Com_Printf("%s: Map %s has wrong texture image position\n",
			__func__, mod_name);
		return NULL;
	}

	if (ident == BSPHL1VERSION)
	{
		byte *src, *dst, *palette;
		size_t i, palette_offset;

		/*
		 * Half-Life 1 stored custom palette information for each mip texture,
		 * so we seek past the last mip texture (and past the 256 2-byte denominator)
		 * to grab the RGB values for this texture
		 */
		palette_offset = LittleLong(texture->offset8);
		palette_offset += (*width >> 3) * (*height >> 3) + 2;

		if ((palette_offset > miptex_size) ||
			((palette_offset + 768) > miptex_size))
		{
			Com_Printf("%s: Map %s has wrong palette image position\n",
				__func__, mod_name);
			return NULL;
		}

		palette = (raw + miptex_offset + texture_offset + palette_offset);

		*bitsPerPixel = 32;
		dst = pic = malloc(size * 4);

		src = (raw + miptex_offset + texture_offset + image_offset);
		for (i = 0; i < size; i++)
		{
			byte value;

			value = src[i];
			dst[0] = palette[value * 3 + 0];
			dst[1] = palette[value * 3 + 1];
			dst[2] = palette[value * 3 + 2];
			dst[3] = value == 255 ? 0 : 255;
			dst += 4;
		}
	}
	else
	{
		*bitsPerPixel = 8;
		pic = malloc(size);
		memcpy(pic, (raw + miptex_offset + texture_offset + image_offset), size);
	}

	Com_DPrintf("%s Loaded embeded %s image %dx%d\n",
		__func__, texture->name, *width, *height);

	return pic;
}

static byte *
Mod_LoadMDLImage(const char *mod_name, int texture_index, byte *raw, int len,
	int *width, int *height, int *bitsPerPixel)
{
	byte *images, *pic;
	dmdx_t *pheader;
	size_t size;

	if (len < sizeof(dmdx_t))
	{
		Com_DPrintf("%s: Short file %s in cache\n",
			__func__, mod_name);
		return NULL;
	}

	pheader = (dmdx_t *)raw;
	if (pheader->ident != IDALIASHEADER)
	{
		Com_DPrintf("%s: Incorrect file %s in cache\n",
			__func__, mod_name);
		return NULL;
	}

	if (pheader->num_skins <= texture_index)
	{
		Com_Printf("%s: Map %s has %d only textures\n",
			__func__, mod_name, pheader->num_skins);
		return NULL;
	}

	size = (pheader->skinheight * pheader->skinwidth * pheader->num_imgbit / 8);

	images = (byte *)pheader + pheader->ofs_imgbit;
	images += (texture_index * size);

	pic = malloc(size);
	memcpy(pic, images, size);

	*width = pheader->skinwidth;
	*height = pheader->skinheight;
	*bitsPerPixel = pheader->num_imgbit;

	Com_DPrintf("%s Loaded embeded %s image %dx%d\n",
		__func__, mod_name, *width, *height);

	return pic;
}

byte *
Mod_LoadEmbdedImage(const char *mod_name, int texture_index, byte *raw, int len,
	int *width, int *height, int *bitsPerPixel)
{
	if (len < sizeof(unsigned))
	{
		return NULL;
	}

	switch (LittleLong(*(unsigned *)raw))
	{
		case IDALIASHEADER:
			return Mod_LoadMDLImage(mod_name, texture_index, raw, len,
				width, height, bitsPerPixel);
		case BSPQ1VERSION:
			/* fall through */
		case BSPHL1VERSION:
			return Mod_LoadBSPImage(mod_name, texture_index, raw, len,
				width, height, bitsPerPixel);
		case IDQ1SPRITEHEADER:
			*bitsPerPixel = 8;
			return Mod_LoadSPRImage(mod_name, texture_index, raw, len,
				width, height);
		default:
			return NULL;
	}
}
