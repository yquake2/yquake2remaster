/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// r_misc.c

#include "r_local.h"

extern void R_DrawText(int x, int y, int alignX, int fontId, float scale, vec4_t color, char* text);
extern int R_GetFontHeight(int fontId);

static byte dottexture[8][8] =
{
	{0,0,0,0,0,0,0,0},
	{0,0,1,1,0,0,0,0},
	{0,1,1,1,1,0,0,0},
	{0,1,1,1,1,0,0,0},
	{0,0,1,1,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
};

/*
==================
R_InitCodeTextures
==================
*/
void R_InitCodeTextures()
{
	int		x,y;
	byte	data[8][8][4];

	//
	// default white texture
	//
	for (x = 0; x < 8; x++)
	{
		for (y = 0; y < 8; y++)
		{
			data[y][x][0] = 255;
			data[y][x][1] = 255;
			data[y][x][2] = 255;
			data[y][x][3] = 255;
		}
	}
	r_texture_white = R_LoadTexture("$white", (byte*)data, 8, 8, it_texture, 32);

	//
	// dummy used for fbo
	//
	r_texture_view = R_LoadTexture("$view", NULL, vid.width, vid.height, it_texture, 32);


	//
	// particle texture
	//
	for (x=0 ; x<8 ; x++)
	{
		for (y=0 ; y<8 ; y++)
		{
			data[y][x][0] = 255;
			data[y][x][1] = 255;
			data[y][x][2] = 255;
			data[y][x][3] = dottexture[x][y]*255;
		}
	}
	r_texture_particle = R_LoadTexture("$default_particle", (byte *)data, 8, 8, it_sprite, 32);

	//
	// also use this for bad textures, but without alpha
	//
	for (x=0 ; x<8 ; x++)
	{
		for (y=0 ; y<8 ; y++)
		{
			data[y][x][0] = 0;
			data[y][x][1] = dottexture[x&3][y&3]*255;
			data[y][x][2] = 0;
			data[y][x][3] = 255;
		}
	}
	r_texture_missing = R_LoadTexture ("$default", (byte *)data, 8, 8, it_texture, 32);
	//r_texture_missing = R_FindTexture("textures/makkon/conc_f01_moss1", it_texture, true);
}


/* 
============================================================================== 
 
						SCREEN SHOTS 
 
============================================================================== 
*/ 

typedef struct _TargaHeader {
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;


/* 
================== 
R_ScreenShot_f
================== 
*/  
void R_ScreenShot_f (void) 
{
	byte		*buffer;
	char		picname[80]; 
	char		checkname[MAX_OSPATH];
	int			i, c, temp;
	FILE		*f;

	buffer = malloc(vid.width * vid.height * 3 + 18);
	if (!buffer)
	{
		ri.Printf(PRINT_ALL, "R_ScreenShot_f: failed to allocate pixels\n");
		return;
	}

	// create the screenshots directory if it doesn't exist
	Com_sprintf (checkname, sizeof(checkname), "%s/screenshots", ri.GetGameDir());
	Sys_Mkdir (checkname);

// 
// find a file name to save it to 
// 
	strcpy(picname,"shot000.tga");

	for (i=0 ; i<=999; i++) 
	{ 
		picname[5] = i/10 + '0'; 
		picname[6] = i%10 + '0'; 
		Com_sprintf (checkname, sizeof(checkname), "%s/screenshots/%s", ri.GetGameDir(), picname);
		f = fopen (checkname, "rb");
		if (!f)
			break;	// file doesn't exist
		fclose (f);
	} 
	if (i==1000) 
	{
		ri.Printf (PRINT_ALL, "R_ScreenShot_f: Couldn't create a file\n"); 
		free(buffer);
		return;
 	}


	memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = vid.width&255;
	buffer[13] = vid.width>>8;
	buffer[14] = vid.height&255;
	buffer[15] = vid.height>>8;
	buffer[16] = 24;	// pixel size

	glPixelStorei(GL_PACK_ALIGNMENT, 1); //fix from yquake2
	glReadPixels (0, 0, vid.width, vid.height, GL_RGB, GL_UNSIGNED_BYTE, buffer+18 ); 

	// swap rgb to bgr
	c = 18+vid.width*vid.height*3;
	for (i=18 ; i<c ; i+=3)
	{
		temp = buffer[i];
		buffer[i] = buffer[i+2];
		buffer[i+2] = temp;
	}

	f = fopen (checkname, "wb");
	fwrite (buffer, 1, c, f);
	fclose (f);


	free (buffer);
	ri.Printf (PRINT_ALL, "Wrote %s\n", picname);
} 

/*
** GL_Strings_f
*/
void GL_Strings_f( void )
{
	ri.Printf (PRINT_ALL, "GL_VENDOR: %s\n", gl_config.vendor_string );
	ri.Printf (PRINT_ALL, "GL_RENDERER: %s\n", gl_config.renderer_string );
	ri.Printf (PRINT_ALL, "GL_VERSION: %s\n", gl_config.version_string );
//	ri.Printf (PRINT_ALL, "GL_EXTENSIONS: %s\n", gl_config.extensions_string );
}

/*
** GL_SetDefaultState
*/
void GL_SetDefaultState( void )
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	R_UnbindProgram();

	glClearColor (1,0, 0.5 , 0.5);
	R_SetCullFace(GL_FRONT);

	R_MultiTextureBind(TMU_LIGHTMAP, 0);
	glEnable(GL_TEXTURE_2D);

	R_MultiTextureBind(TMU_DIFFUSE, 0);
	glEnable(GL_TEXTURE_2D);

	R_AlphaTest(true);
	glAlphaFunc(GL_GREATER, 0.1);

	R_DepthTest(false);
	R_CullFace(false);
	R_Blend(false);

	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);

	R_SetTextureMode( r_texturemode->string );
	R_SetTextureAlphaMode( r_texturealphamode->string );
	R_TextureSolidMode( r_texturesolidmode->string );

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	R_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GL_UpdateSwapInterval();
}

void GL_UpdateSwapInterval( void )
{
	if ( r_swapinterval->modified )
	{
		r_swapinterval->modified = false;

		if ( !gl_state.stereo_enabled ) 
		{
#ifdef _WIN32
			if ( qwglSwapIntervalEXT )
				qwglSwapIntervalEXT( r_swapinterval->value );
#endif
		}
	}
}

//just sorta sticking the profiling here
#ifdef _WIN32
LARGE_INTEGER qpc_freq; //Initialized in R_Init
LARGE_INTEGER qpc_samples[NUM_PROFILES];
#endif

double lastsamples[NUM_PROFILES][NUM_TIMESAMPLES]; //in MS
static int currentsample; //0 - (NUM_TIMESAMPLES-1)

const char* stagenames[NUM_PROFILES] =
{
	"start",
	"setup",
	"world",
	"entities",
	"debug",
	"alpha surfaces",
	"particles",
	"total"
};

void R_StartProfiling()
{
#ifdef WIN32
	QueryPerformanceCounter(&qpc_samples[0]);
#endif
}

void R_ProfileAtStage(profiletype_e stage)
{
#ifdef WIN32
		QueryPerformanceCounter(&qpc_samples[stage]);
#endif
}

void R_FinishProfiling()
{
	int i;
#ifdef WIN32
	//Make all readings relative to the previous reading
	for (i = STAGE_TOTAL - 1; i > STAGE_START; i--)
	{
		qpc_samples[i].QuadPart -= qpc_samples[i-1].QuadPart;
	}
	//Total is all time
	qpc_samples[STAGE_TOTAL].QuadPart -= qpc_samples[0].QuadPart;

	for (i = 1; i < NUM_PROFILES; i++)
	{
		//Convert the time readings into MS doubles. 
		//Overflow shouldn't be likely due to the subtraction of the initial reading, meaning that the QPC readings should be very small.
		lastsamples[i][currentsample] = (double)qpc_samples[i].QuadPart * 1000 / qpc_freq.QuadPart;
	}
#endif
	currentsample = (currentsample + 1) % NUM_TIMESAMPLES;
}

static double R_AvgSample(int stage)
{
	double avg = 0;
	for (int i = 0; i < NUM_TIMESAMPLES; i++)
	{
		avg += lastsamples[stage][i];
	}

	return avg / NUM_TIMESAMPLES;
}

void R_DrawProfilingReport()
{
#ifdef WIN32
	vec4_t color;
	float fontscale = 0.25;
	float x, y, h;

	x = vid.width - 10;
	y = 32 + 250;
	h = R_GetFontHeight(0) * fontscale;

	Vector4Set(color, 0, 0, 0, 0.35f);

	R_ProgUniform4f(LOC_COLOR4, 0, 0, 0, 0.5);
	R_DrawFill(vid.width - 175, 45 + 230, 175, (NUM_PROFILES - 1) * h + 16);

	Vector4Set(color, 1, 1, 1, 1.0);
	for (int i = 1; i < NUM_PROFILES; i++)
	{
		R_DrawText(x, y, 2, 0, fontscale, color, va("%s: %.3f ms", stagenames[i], R_AvgSample(i)));
		y += h;
	}
#endif
}
