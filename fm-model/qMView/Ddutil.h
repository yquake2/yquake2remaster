/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation. All Rights Reserved.
 *
 *  File:       ddutil.cpp
 *  Content:    Routines for loading bitmap and palettes from resources
 *
 ***************************************************************************/

#define PI 3.141592654

typedef float	vec_t[3];

typedef unsigned char  u_char;

typedef float scalar_t;
typedef BOOL qboolean;

typedef struct
{
	scalar_t  x;
	scalar_t  y;
	scalar_t  z;
} vec3_t;

typedef struct
{
	scalar_t  x;
	scalar_t  y;
	scalar_t  z;
	scalar_t  s;
	scalar_t  t;
} vec5_t;

class CRGB16
{
public:
    RGBQUAD depth;
    RGBQUAD Amount;
    RGBQUAD Position;
};

class CDDHelper
{
public:
	static LPDIRECTDRAW			lpDD;

	static CRGB16*				CreateRGB16 (LPDIRECTDRAWSURFACE Surface);
	static IDirectDrawPalette * DDLoadPalette(LPCSTR szBitmap);
	static IDirectDrawSurface * DDLoadBitmap(LPCSTR szBitmap, int dx, int dy);
	static HRESULT              DDReLoadBitmap(IDirectDrawSurface *pdds, LPCSTR szBitmap);
	static HRESULT              DDCopyBitmap(IDirectDrawSurface *pdds, HBITMAP hbm, int x, int y, int dx, int dy);
	static DWORD                DDColorMatch(IDirectDrawSurface *pdds, COLORREF rgb);
	static HRESULT              DDSetColorKey(IDirectDrawSurface *pdds, COLORREF rgb);

	static char *TraceError(HRESULT ddrval);
};

