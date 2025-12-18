/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "r_local.h"

image_t		r_textures[MAX_GLTEXTURES];
int			r_textures_count;

int		gl_tex_solid_format = 3;
int		gl_tex_alpha_format = 4;

int		gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
int		gl_filter_max = GL_LINEAR;

static void R_LoadTGA(char* name, byte** pic, int* width, int* height);

static mte = false;


void R_EnableMultiTexture()
{
	for (int i = MIN_TEXTURE_MAPPING_UNITS; i != -1; i--)
	{
		gl_state.current_texture[i] = -1;
		R_SelectTextureUnit(i);
		glEnable(GL_TEXTURE_2D);
	}
}

void R_DisableMultiTexture()
{
	// disable everything but diffuse
	for (int i = MIN_TEXTURE_MAPPING_UNITS; i != 0; i--)
	{
		R_SelectTextureUnit(i);
		glDisable(GL_TEXTURE_2D);
	}
	R_SelectTextureUnit(TMU_DIFFUSE);
}


void R_SelectTextureUnit( unsigned int tmu )
{
	if ( tmu == gl_state.current_tmu )
		return;

	glActiveTexture(GL_TEXTURE0 + tmu);
	gl_state.current_tmu = tmu;
}


void R_BindTexture(int texnum)
{
	if (r_nobind->value)
	{
		return;
	}

	if ( gl_state.current_texture[gl_state.current_tmu] == texnum)
		return;

	rperf.texture_binds[gl_state.current_tmu]++;

	gl_state.current_texture[gl_state.current_tmu] = texnum;
	glBindTexture(GL_TEXTURE_2D, texnum);
}

void R_MultiTextureBind(unsigned int tmu, int texnum)
{
	R_SelectTextureUnit( tmu );

	if ( gl_state.current_texture[tmu] == texnum )
		return;

	R_BindTexture( texnum );
}

typedef struct
{
	char *name;
	int	minimize, maximize;
} glmode_t;

glmode_t modes[] = {
	{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};

#define NUM_GL_MODES (sizeof(modes) / sizeof (glmode_t))

typedef struct
{
	char *name;
	int mode;
} gltmode_t;

gltmode_t gl_alpha_modes[] = {
	{"default", 4},
	{"GL_RGBA", GL_RGBA},
	{"GL_RGBA8", GL_RGBA8},
	{"GL_RGB5_A1", GL_RGB5_A1},
	{"GL_RGBA4", GL_RGBA4},
	{"GL_RGBA2", GL_RGBA2},
};

#define NUM_GL_ALPHA_MODES (sizeof(gl_alpha_modes) / sizeof (gltmode_t))

gltmode_t gl_solid_modes[] = {
	{"default", 3},
	{"GL_RGB", GL_RGB},
	{"GL_RGB8", GL_RGB8},
	{"GL_RGB5", GL_RGB5},
	{"GL_RGB4", GL_RGB4},
	{"GL_R3_G3_B2", GL_R3_G3_B2},
#ifdef GL_RGB2_EXT
	{"GL_RGB2", GL_RGB2_EXT},
#endif
};

#define NUM_GL_SOLID_MODES (sizeof(gl_solid_modes) / sizeof (gltmode_t))

/*
===============
R_SetTextureMode
===============
*/
void R_SetTextureMode( char *string )
{
	int		i;
	image_t	*glt;

	for (i=0 ; i< NUM_GL_MODES ; i++)
	{
		if ( !Q_stricmp( modes[i].name, string ) )
			break;
	}

	if (i == NUM_GL_MODES)
	{
		ri.Printf (PRINT_ALL, "bad filter name\n");
		return;
	}

	gl_filter_min = modes[i].minimize;
	gl_filter_max = modes[i].maximize;

	// change all the existing mipmap texture objects
	for (i=0, glt=r_textures ; i<r_textures_count ; i++, glt++)
	{
		if ( glt->type != it_gui && glt->type != it_sky )
		{
			R_BindTexture (glt->texnum);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		}
	}
}

/*
===============
R_SetTextureAlphaMode
===============
*/
void R_SetTextureAlphaMode( char *string )
{
	int		i;

	for (i=0 ; i< NUM_GL_ALPHA_MODES ; i++)
	{
		if ( !Q_stricmp( gl_alpha_modes[i].name, string ) )
			break;
	}

	if (i == NUM_GL_ALPHA_MODES)
	{
		ri.Printf (PRINT_ALL, "bad alpha texture mode name\n");
		return;
	}

	gl_tex_alpha_format = gl_alpha_modes[i].mode;
}

/*
===============
R_TextureSolidMode
===============
*/
void R_TextureSolidMode( char *string )
{
	int		i;

	for (i=0 ; i< NUM_GL_SOLID_MODES ; i++)
	{
		if ( !Q_stricmp( gl_solid_modes[i].name, string ) )
			break;
	}

	if (i == NUM_GL_SOLID_MODES)
	{
		ri.Printf (PRINT_ALL, "bad solid texture mode name\n");
		return;
	}

	gl_tex_solid_format = gl_solid_modes[i].mode;
}

/*
===============
R_TextureList_f
===============
*/
void R_TextureList_f(void)
{
	int		i;
	image_t	*image;
	int		texels;

	ri.Printf (PRINT_ALL, "------------------\n");
	texels = 0;

	for (i=0, image=r_textures ; i<r_textures_count ; i++, image++)
	{
		if (image->texnum <= 0)
			continue;
		texels += image->upload_width*image->upload_height;
		switch (image->type)
		{
		case it_model:
			ri.Printf (PRINT_ALL, "MDL ");
			break;
		case it_sprite:
			ri.Printf (PRINT_ALL, "SPR ");
			break;
		case it_texture:
			ri.Printf (PRINT_ALL, "TEX ");
			break;
		case it_gui:
			ri.Printf (PRINT_ALL, "GUI ");
			break;
		case it_font:
			ri.Printf(PRINT_ALL, "FONT ");
			break;
		case it_sky:
			ri.Printf(PRINT_ALL, "SKY ");
			break;
		default:
			ri.Printf (PRINT_ALL, "?   ");
			break;
		}

		ri.Printf (PRINT_ALL, "%i: [%ix%i %s]: %s\n",
			i, image->upload_width, image->upload_height, (image->has_alpha ? "RGBA" : "RGB"), image->name);
	}
	ri.Printf (PRINT_ALL, "\nTotal texel count (not counting mipmaps): %i\n", texels);
	ri.Printf(PRINT_ALL, "Total %i out of %i textures in use\n\n", r_textures_count, MAX_GLTEXTURES);
}

/*
===============
R_UploadTexture32

Returns has_alpha
===============
*/

static int		upload_width, upload_height;
static char		*upload_name;

static int IsPowerOfTwo(int x)
{
	if (x == 0)
		return 0;
	return (x & (x - 1)) == 0;
}

qboolean R_UploadTexture32(unsigned* data, int width, int height, qboolean mipmap)
{
	int			samples;
	int			i, c;
	byte		*scan;
	int			comp;
	int			scaled_width, scaled_height;

	if (!IsPowerOfTwo(width) || !IsPowerOfTwo(height))
	{
		ri.Error(ERR_FATAL, "Texture \"%s\" [%i x %i] dimensions are not power of two.\n", upload_name, width, height);
	}

	// let people sample down the world textures for speed
	// but don't allow for too blurry textures, refuse to downscale 256^2px textures
	if (mipmap && width >= 256 && height >= 256)
	{
		scaled_width = width;
		scaled_height = height;

		scaled_width >>= (int)r_picmip->value;
		scaled_height >>= (int)r_picmip->value;
		
		if (scaled_width < 256)
			scaled_width = 256;
		if (scaled_height < 256)
			scaled_height = 256;

		upload_width = scaled_width;
		upload_height = scaled_height;
	}
	else
	{
		upload_width = width;
		upload_height = height;
	}

	// scan the texture for any non-255 alpha
	c = width * height;
	scan = ((byte*)data) + 3;
	samples = gl_tex_solid_format;
	for (i = 0; i < c; i++, scan += 4)
	{
		if (*scan != 255)
		{
			samples = gl_tex_alpha_format;
			break;
		}
	}

	if (samples == gl_tex_solid_format) 
	{
		comp = gl_tex_solid_format; // rgb
	}
	else if (samples == gl_tex_alpha_format)
	{
		comp = gl_tex_alpha_format; // rgba
	}
	else // weirdo
	{
		ri.Error(ERR_FATAL, "Texture \"%s\" is not RGB or RGBA (unknown number of texture components %i)\n", upload_name, samples);
		comp = samples;
	}

	if (upload_width != width || upload_height != height)
	{
		upload_width = width;
		upload_height = height;
		glTexImage2D(GL_TEXTURE_2D, 0, comp, upload_width, upload_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
#if 0
		byte* scaled = malloc(sizeof(byte) * upload_width * upload_height * comp);
		if (!scaled)
			ri.Error(ERR_FATAL, "malloc failed\n");

		R_ResampleTexture(data, width, height, scaled, upload_width, upload_height); // this wil crash with too large textures
		glTexImage2D(GL_TEXTURE_2D, 0, comp, upload_width, upload_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
		free(scaled);
#endif
	}
	else
		glTexImage2D(GL_TEXTURE_2D, 0, comp, upload_width, upload_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	if (mipmap)
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}
	return (samples == gl_tex_alpha_format);
}

/*
================
R_AllocTexture()

find a free image_t
================
*/
static image_t* R_AllocTexture()
{
	image_t* image;
	int			i;

	for (i = 0, image = r_textures; i < r_textures_count; i++, image++)
	{
		if (!image->texnum)
			break;
	}

	if (i == r_textures_count)
	{
		if (r_textures_count == MAX_GLTEXTURES)
			ri.Error(ERR_DROP, "MAX_GLTEXTURES");
		r_textures_count++;
	}
	return &r_textures[i];
}

/*
================
R_LoadTexture

This is also used as an entry point for the generated code textures
================
*/
image_t *R_LoadTexture(char *name, byte *pixels, int width, int height, texType_t type, int bits)
{
	image_t* image = R_AllocTexture();

	if (strlen(name) >= sizeof(image->name))
		ri.Error (ERR_DROP, "R_LoadTexture: \"%s\" has too long name", name);

	strcpy (image->name, name);

	image->registration_sequence = registration_sequence;

	upload_name = image->name;

	image->width = width;
	image->height = height;
	image->type = type;

	image->texnum = TEXNUM_IMAGES + (image - r_textures);

	if (pixels == NULL)
	{
		image->has_alpha = bits == 32 ? true : false;
		image->upload_width = image->width;
		image->upload_height = image->height;
		image->texnum = image->texnum;
	}
	else
	{
		R_BindTexture(image->texnum);
		image->has_alpha = R_UploadTexture32((unsigned*)pixels, width, height, (image->type != it_font && image->type != it_gui && image->type != it_sky));
		image->upload_width = upload_width;
		image->upload_height = upload_height;
	}
	
	image->sl = 0;
	image->sh = 1;
	image->tl = 0;
	image->th = 1;

	return image;
}

/*
===============
R_FindTexture

Finds or loads the given image
===============
*/
image_t	*R_FindTexture(char *name, texType_t type, qboolean load)
{
	image_t	*image;
	int		i, len;
	byte	*pic;
	int		width, height;

	if (!name)
	{
		ri.Error(ERR_DROP, "R_FindTexture: NULL name");
		return NULL;
	}

	len = strlen(name);
	if (len < 5)
	{
		ri.Error(ERR_DROP, "R_FindTexture: bad name: %s", name);
		return NULL;
	}
		
	// look for it
	for (i=0, image=r_textures ; i<r_textures_count ; i++,image++)
	{
		if (!strcmp(name, image->name))
		{
			image->registration_sequence = registration_sequence;
			return image;
		}
	}

	if (!load)
	{
		return NULL; // not found and no loading allowed
	}

	//
	// load the pic from disk
	//
	pic = NULL;
	if (!strcmp(name+len-4, ".tga"))
	{
		R_LoadTGA (name, &pic, &width, &height);
		if (!pic)
		{
//			ri.Printf(PRINT_LOW, "R_FindTexture: couldn't load %s\n", name);
			return r_texture_missing;
		}
		image = R_LoadTexture (name, pic, width, height, type, 32);
	}
	else
	{
		ri.Printf(PRINT_LOW, "R_FindTexture: weird file %s\n", name);
		return r_texture_missing;
	}

	if (pic)
		free(pic);

	return image;
}



/*
===============
R_RegisterSkin
===============
*/
struct image_s *R_RegisterSkin (char *name)
{
	return R_FindTexture (name, it_model, true);
}


/*
================
R_FreeUnusedTextures

Any image that was not touched on this registration sequence will be freed.
================
*/
void R_FreeUnusedTextures()
{
	int		i;
	image_t	*image;

	// never free code textures
	r_texture_white->registration_sequence = registration_sequence;
	r_texture_missing->registration_sequence = registration_sequence;
	r_texture_particle->registration_sequence = registration_sequence;
	r_texture_view->registration_sequence = registration_sequence;

	for (i = 0, image = r_textures; i < r_textures_count; i++, image++)
	{
		if (image->registration_sequence == registration_sequence)
			continue;		// used this sequence
		if (!image->registration_sequence)
			continue;		// free image_t slot
		if (image->type == it_gui)
			continue;		// don't free gui pics

		if (image->type == it_font)
		{
			image->registration_sequence = registration_sequence;
			continue;		// don't free fonts
		}

		// free it
		glDeleteTextures (1, &image->texnum);
		memset (image, 0, sizeof(*image));
	}
}

/*
===============
R_InitTextures
===============
*/
void R_InitTextures()
{
	registration_sequence = 1;
	R_InitCodeTextures();
}

/*
===============
R_FreeTextures

Frees all textures, usualy on shutdown
===============
*/
void R_FreeTextures()
{
	int		i;
	image_t	*image;

	for (i=0, image=r_textures ; i<r_textures_count ; i++, image++)
	{
		if (!image->registration_sequence)
			continue;		// free image_t slot

		
		if(glDeleteTextures) // check if gl context exists
			glDeleteTextures (1, &image->texnum); // free it
		memset (image, 0, sizeof(*image));
	}
}



/*
=========================================================

TARGA LOADING

=========================================================
*/

#define TGA_TYPE_RAW_RGB 2 // Uncompressed, RGB images
#define TGA_TYPE_RUNLENGHT_RGB 10 // Runlength encoded RGB images


typedef struct _TargaHeader {
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;


/*
=============
R_LoadTGA
=============
*/
static void R_LoadTGA(char* name, byte** pic, int* width, int* height)
{
	int		columns, rows, numPixels;
	byte* pixbuf;
	int		row, column;
	byte* buf_p;
	byte* buffer;
	int		length;
	TargaHeader		targa_header;
	byte* targa_rgba;
	byte tmp[2];

	*pic = NULL;

	//
	// load the file
	//
	length = ri.LoadFile(name, (void**)&buffer);
	if (!buffer)
	{
		//		ri.Printf (PRINT_DEVELOPER, "Bad tga file %s\n", name);
		return;
	}

	buf_p = buffer;

	targa_header.id_length = *buf_p++;
	targa_header.colormap_type = *buf_p++;
	targa_header.image_type = *buf_p++;

	tmp[0] = buf_p[0];
	tmp[1] = buf_p[1];
	targa_header.colormap_index = LittleShort(*((short*)tmp));
	buf_p += 2;
	tmp[0] = buf_p[0];
	tmp[1] = buf_p[1];
	targa_header.colormap_length = LittleShort(*((short*)tmp));
	buf_p += 2;
	targa_header.colormap_size = *buf_p++;
	targa_header.x_origin = LittleShort(*((short*)buf_p));
	buf_p += 2;
	targa_header.y_origin = LittleShort(*((short*)buf_p));
	buf_p += 2;
	targa_header.width = LittleShort(*((short*)buf_p));
	buf_p += 2;
	targa_header.height = LittleShort(*((short*)buf_p));
	buf_p += 2;
	targa_header.pixel_size = *buf_p++;
	targa_header.attributes = *buf_p++;

	if (targa_header.image_type != TGA_TYPE_RAW_RGB && targa_header.image_type != TGA_TYPE_RUNLENGHT_RGB)
	{
		ri.Error(ERR_DROP, "R_LoadTGA: '%s' Only type 2 and 10 targa RGB images supported\n", name);
	}

	if (targa_header.colormap_type != 0 || (targa_header.pixel_size != 32 && targa_header.pixel_size != 24))
		ri.Error(ERR_DROP, "R_LoadTGA: Only 32 or 24 bit images supported (no colormaps)\n");

	columns = targa_header.width;
	rows = targa_header.height;
	numPixels = columns * rows;

	if (width)
		*width = columns;
	if (height)
		*height = rows;

	targa_rgba = malloc(numPixels * 4);
	*pic = targa_rgba;

	if (targa_header.id_length != 0)
		buf_p += targa_header.id_length;  // skip TARGA image comment

	if (targa_header.image_type == TGA_TYPE_RAW_RGB)
	{
		for (row = rows - 1; row >= 0; row--)
		{
			pixbuf = targa_rgba + row * columns * 4;
			for (column = 0; column < columns; column++)
			{
				unsigned char red, green, blue, alphabyte;
				switch (targa_header.pixel_size)
				{
				case 24:

					blue = *buf_p++;
					green = *buf_p++;
					red = *buf_p++;
					*pixbuf++ = red;
					*pixbuf++ = green;
					*pixbuf++ = blue;
					*pixbuf++ = 255;
					break;
				case 32:
					blue = *buf_p++;
					green = *buf_p++;
					red = *buf_p++;
					alphabyte = *buf_p++;
					*pixbuf++ = red;
					*pixbuf++ = green;
					*pixbuf++ = blue;
					*pixbuf++ = alphabyte;
					break;
				}
			}
		}
	}
	else if (targa_header.image_type == TGA_TYPE_RUNLENGHT_RGB)
	{
		unsigned char red, green, blue, alphabyte, packetHeader, packetSize, j;
		for (row = rows - 1; row >= 0; row--)
		{
			pixbuf = targa_rgba + row * columns * 4;
			for (column = 0; column < columns; )
			{
				packetHeader = *buf_p++;
				packetSize = 1 + (packetHeader & 0x7f);
				if (packetHeader & 0x80) // run-length packet
				{
					switch (targa_header.pixel_size)
					{
					case 24:
						blue = *buf_p++;
						green = *buf_p++;
						red = *buf_p++;
						alphabyte = 255;
						break;
					case 32:
						blue = *buf_p++;
						green = *buf_p++;
						red = *buf_p++;
						alphabyte = *buf_p++;
						break;
					}

					for (j = 0; j < packetSize; j++)
					{
						*pixbuf++ = red;
						*pixbuf++ = green;
						*pixbuf++ = blue;
						*pixbuf++ = alphabyte;
						column++;
						if (column == columns) // run spans across rows
						{
							column = 0;
							if (row > 0)
								row--;
							else
								goto breakOut;
							pixbuf = targa_rgba + row * columns * 4;
						}
					}
				}
				else // non run-length packet
				{
					for (j = 0; j < packetSize; j++)
					{
						switch (targa_header.pixel_size)
						{
						case 24:
							blue = *buf_p++;
							green = *buf_p++;
							red = *buf_p++;
							*pixbuf++ = red;
							*pixbuf++ = green;
							*pixbuf++ = blue;
							*pixbuf++ = 255;
							break;
						case 32:
							blue = *buf_p++;
							green = *buf_p++;
							red = *buf_p++;
							alphabyte = *buf_p++;
							*pixbuf++ = red;
							*pixbuf++ = green;
							*pixbuf++ = blue;
							*pixbuf++ = alphabyte;
							break;
						}
						column++;
						if (column == columns)  // pixel packet run spans across rows
						{
							column = 0;
							if (row > 0)
								row--;
							else
								goto breakOut;
							pixbuf = targa_rgba + row * columns * 4;
						}
					}
				}
			}
		breakOut:;
		}
	}
	ri.FreeFile(buffer);
}
