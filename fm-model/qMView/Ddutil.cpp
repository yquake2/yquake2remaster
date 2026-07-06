/*==========================================================================
 *
 *  Copyright (C) 1995-1996 Microsoft Corporation. All Rights Reserved.
 *
 *  File:       ddutil.cpp
 *  Content:    Routines for loading bitmap and palettes from resources
 *
 ***************************************************************************/
#undef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include "stdafx.h"
#include "ddutil.h"

LPDIRECTDRAW CDDHelper::lpDD;

CRGB16* CDDHelper::CreateRGB16 ( LPDIRECTDRAWSURFACE Surface)
{
    DDSURFACEDESC   ddsd;
    BYTE shiftcount;
    //get a surface despriction
    ddsd.dwSize = sizeof( ddsd );
	ddsd.dwFlags = DDSD_PIXELFORMAT;
    if (Surface->GetSurfaceDesc ( &ddsd ) != DD_OK )
        return FALSE;
    //get red
    shiftcount = 0;
	CRGB16* rgb16 = new CRGB16();
    while ( !(ddsd.ddpfPixelFormat.dwRBitMask & 1) )
    {
        ddsd.ddpfPixelFormat.dwRBitMask >>= 1;
        shiftcount++;
    }
    rgb16->depth.rgbRed = (BYTE) ddsd.ddpfPixelFormat.dwRBitMask;
    rgb16->Position.rgbRed = shiftcount;
    rgb16->Amount.rgbRed = (ddsd.ddpfPixelFormat.dwRBitMask
                                            == 0x1f) ? 3 : 2;
    //get green
    shiftcount = 0;
    while ( !(ddsd.ddpfPixelFormat.dwGBitMask & 1) )
    {
        ddsd.ddpfPixelFormat.dwGBitMask >>= 1;
        shiftcount++;
    }
    rgb16->depth.rgbGreen =(BYTE)ddsd.ddpfPixelFormat.dwGBitMask;
    rgb16->Position.rgbGreen = shiftcount;
    rgb16->Amount.rgbGreen = (ddsd.ddpfPixelFormat.dwGBitMask
                                            == 0x1f) ? 3 : 2;
    //get Blue
    shiftcount = 0;
    while ( !(ddsd.ddpfPixelFormat.dwBBitMask & 1) )
    {
        ddsd.ddpfPixelFormat.dwBBitMask >>= 1;
        shiftcount++;
    }
    rgb16->depth.rgbBlue =(BYTE)ddsd.ddpfPixelFormat.dwBBitMask;
    rgb16->Position.rgbBlue = shiftcount;
    rgb16->Amount.rgbBlue = (ddsd.ddpfPixelFormat.dwBBitMask
                                            == 0x1f) ? 3 : 2;
    return rgb16;
}

/*
 *  DDLoadBitmap
 *
 *  create a DirectDrawSurface from a bitmap resource.
 *
 */
IDirectDrawSurface * CDDHelper::DDLoadBitmap(LPCSTR szBitmap, int dx, int dy)
{
    HBITMAP             hbm;
    BITMAP              bm;
    DDSURFACEDESC       ddsd;
    IDirectDrawSurface *pdds;

    //
    //  try to load the bitmap as a resource, if that fails, try it as a file
    //
    hbm = (HBITMAP)LoadImage(GetModuleHandle(NULL), szBitmap, IMAGE_BITMAP, dx, dy, LR_CREATEDIBSECTION);

    if (hbm == NULL)
        hbm = (HBITMAP)LoadImage(NULL, szBitmap, IMAGE_BITMAP, dx, dy, LR_LOADFROMFILE|LR_CREATEDIBSECTION);

    if (hbm == NULL)
        return NULL;

    //
    // get size of the bitmap
    //
    GetObject(hbm, sizeof(bm), &bm);      // get size of bitmap

    //
    // create a DirectDrawSurface for this bitmap
    //
    ZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT |DDSD_WIDTH;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.dwWidth = bm.bmWidth;
    ddsd.dwHeight = bm.bmHeight;

    if (lpDD->CreateSurface(&ddsd, &pdds, NULL) != DD_OK)
        return NULL;

    DDCopyBitmap(pdds, hbm, 0, 0, 0, 0);

    DeleteObject(hbm);

    return pdds;
}

/*
 *  DDReLoadBitmap
 *
 *  load a bitmap from a file or resource into a directdraw surface.
 *  normaly used to re-load a surface after a restore.
 *
 */
HRESULT CDDHelper::DDReLoadBitmap(IDirectDrawSurface *pdds, LPCSTR szBitmap)
{
    HBITMAP             hbm;
    HRESULT             hr;

    //
    //  try to load the bitmap as a resource, if that fails, try it as a file
    //
    hbm = (HBITMAP)LoadImage(GetModuleHandle(NULL), szBitmap, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

    if (hbm == NULL)
        hbm = (HBITMAP)LoadImage(NULL, szBitmap, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE|LR_CREATEDIBSECTION);

    if (hbm == NULL)
    {
        OutputDebugString("handle is null\n");
        return E_FAIL;
    }

    hr = DDCopyBitmap(pdds, hbm, 0, 0, 0, 0);
    if (hr != DD_OK)
    {
        OutputDebugString("ddcopybitmap failed\n");
    }


    DeleteObject(hbm);
    return hr;
}

/*
 *  DDCopyBitmap
 *
 *  draw a bitmap into a DirectDrawSurface
 *
 */
HRESULT CDDHelper::DDCopyBitmap(IDirectDrawSurface *pdds, HBITMAP hbm, int x, int y, int dx, int dy)
{
    HDC                 hdcImage;
    HDC                 hdc;
    BITMAP              bm;
    DDSURFACEDESC       ddsd;
    HRESULT             hr;

    if (hbm == NULL || pdds == NULL)
        return E_FAIL;

    //
    // make sure this surface is restored.
    //
    pdds->Restore();

    //
    //  select bitmap into a memoryDC so we can use it.
    //
    hdcImage = CreateCompatibleDC(NULL);
    if (!hdcImage)
        OutputDebugString("createcompatible dc failed\n");
    SelectObject(hdcImage, hbm);

    //
    // get size of the bitmap
    //
    GetObject(hbm, sizeof(bm), &bm);    // get size of bitmap
    dx = dx == 0 ? bm.bmWidth  : dx;    // use the passed size, unless zero
    dy = dy == 0 ? bm.bmHeight : dy;

    //
    // get size of surface.
    //
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_HEIGHT | DDSD_WIDTH;
    pdds->GetSurfaceDesc(&ddsd);

    if ((hr = pdds->GetDC(&hdc)) == DD_OK)
    {
        StretchBlt(hdc, 0, 0, ddsd.dwWidth, ddsd.dwHeight, hdcImage, x, y, dx, dy, SRCCOPY);
        pdds->ReleaseDC(hdc);
    }

    DeleteDC(hdcImage);

    return hr;
}

//
//  DDLoadPalette
//
//  Create a DirectDraw palette object from a bitmap resoure
//
//  if the resource does not exist or NULL is passed create a
//  default 332 palette.
//
IDirectDrawPalette * CDDHelper::DDLoadPalette(LPCSTR szBitmap)
{
    IDirectDrawPalette* ddpal;
    int                 i;
    int                 n;
    int                 fh;
    HRSRC               h;
    LPBITMAPINFOHEADER  lpbi;
    PALETTEENTRY        ape[256];
    RGBQUAD *           prgb;

    //
    // build a 332 palette as the default.
    //
    for (i=0; i<256; i++)
    {
        ape[i].peRed   = (BYTE)(((i >> 5) & 0x07) * 255 / 7);
        ape[i].peGreen = (BYTE)(((i >> 2) & 0x07) * 255 / 7);
        ape[i].peBlue  = (BYTE)(((i >> 0) & 0x03) * 255 / 3);
        ape[i].peFlags = (BYTE)0;
    }

    //
    // get a pointer to the bitmap resource.
    //
    if (szBitmap && (h = FindResource(NULL, szBitmap, RT_BITMAP)))
    {
        lpbi = (LPBITMAPINFOHEADER)LockResource(LoadResource(NULL, h));
        if (!lpbi)
            OutputDebugString("lock resource failed\n");
        prgb = (RGBQUAD*)((BYTE*)lpbi + lpbi->biSize);

        if (lpbi == NULL || lpbi->biSize < sizeof(BITMAPINFOHEADER))
            n = 0;
        else if (lpbi->biBitCount > 8)
            n = 0;
        else if (lpbi->biClrUsed == 0)
            n = 1 << lpbi->biBitCount;
        else
            n = lpbi->biClrUsed;

        //
        //  a DIB color table has its colors stored BGR not RGB
        //  so flip them around.
        //
        for(i=0; i<n; i++ )
        {
            ape[i].peRed   = prgb[i].rgbRed;
            ape[i].peGreen = prgb[i].rgbGreen;
            ape[i].peBlue  = prgb[i].rgbBlue;
            ape[i].peFlags = 0;
        }
    }
    else if (szBitmap && (fh = _lopen(szBitmap, OF_READ)) != -1)
    {
        BITMAPFILEHEADER bf;
        BITMAPINFOHEADER bi;

        _lread(fh, &bf, sizeof(bf));
        _lread(fh, &bi, sizeof(bi));
        _lread(fh, ape, sizeof(ape));
        _lclose(fh);

        if (bi.biSize != sizeof(BITMAPINFOHEADER))
            n = 0;
        else if (bi.biBitCount > 8)
            n = 0;
        else if (bi.biClrUsed == 0)
            n = 1 << bi.biBitCount;
        else
            n = bi.biClrUsed;

        //
        //  a DIB color table has its colors stored BGR not RGB
        //  so flip them around.
        //
        for(i=0; i<n; i++ )
        {
            BYTE r = ape[i].peRed;
            ape[i].peRed  = ape[i].peBlue;
            ape[i].peBlue = r;
        }
    }

    lpDD->CreatePalette(DDPCAPS_8BIT, ape, &ddpal, NULL);

    return ddpal;
}

/*
 * DDColorMatch
 *
 * convert a RGB color to a pysical color.
 *
 * we do this by leting GDI SetPixel() do the color matching
 * then we lock the memory and see what it got mapped to.
 */
DWORD CDDHelper::DDColorMatch(IDirectDrawSurface *pdds, COLORREF rgb)
{
    COLORREF rgbT;
    HDC hdc;
    DWORD dw = CLR_INVALID;
    DDSURFACEDESC ddsd;
    HRESULT hres;

    //
    //  use GDI SetPixel to color match for us
    //
    if (rgb != CLR_INVALID && pdds->GetDC(&hdc) == DD_OK)
    {
        rgbT = GetPixel(hdc, 0, 0);             // save current pixel value
        SetPixel(hdc, 0, 0, rgb);               // set our value
        pdds->ReleaseDC(hdc);
    }

    //
    // now lock the surface so we can read back the converted color
    //
    ddsd.dwSize = sizeof(ddsd);
    while ((hres = pdds->Lock(NULL, &ddsd, 0, NULL)) == DDERR_WASSTILLDRAWING)
        ;

    if (hres == DD_OK)
    {
        dw  = *(DWORD *)ddsd.lpSurface;                     // get DWORD
        dw &= (1 << ddsd.ddpfPixelFormat.dwRGBBitCount)-1;  // mask it to bpp
        pdds->Unlock(NULL);
    }

    //
    //  now put the color that was there back.
    //
    if (rgb != CLR_INVALID && pdds->GetDC(&hdc) == DD_OK)
    {
        SetPixel(hdc, 0, 0, rgbT);
        pdds->ReleaseDC(hdc);
    }

    return dw;
}

/*
 * DDSetColorKey
 *
 * set a color key for a surface, given a RGB.
 * if you pass CLR_INVALID as the color key, the pixel
 * in the upper-left corner will be used.
 */
HRESULT CDDHelper::DDSetColorKey(IDirectDrawSurface *pdds, COLORREF rgb)
{
    DDCOLORKEY          ddck;

    ddck.dwColorSpaceLowValue  = DDColorMatch(pdds, rgb);
    ddck.dwColorSpaceHighValue = ddck.dwColorSpaceLowValue;
    return pdds->SetColorKey(DDCKEY_SRCBLT, &ddck);
}

static char out[256];

char * CDDHelper::TraceError(HRESULT ddrval)
{
	switch(ddrval)
	{
		case D3DRM_OK : break;
		case D3DRMERR_BADALLOC :
			return("Out of Memory");
			break;
		case D3DRMERR_BADDEVICE :
			return("Device is not compatible with render");
			break;
		case D3DRMERR_BADFILE :
			return("Data file is corrupt");
			break;
		case D3DRMERR_BADMAJORVERSION :
			return("Bad DLL major version");
			break;
		case D3DRMERR_BADMINORVERSION :
			return("Bad DLL minor version");
			break;
		case D3DRMERR_BADOBJECT :
			return("Object expected in argument");
			break;
		case D3DRMERR_BADTYPE :
			return("Bad argument type passed");
			break;
		case D3DRMERR_BADVALUE :
			return("Bad argument value passed");
			break;
		case D3DRMERR_FACEUSED :
			return("Face already used in a mesh");
			break;
		case D3DRMERR_FILENOTFOUND :
			return("File cannot be opened");
			break;
		case D3DRMERR_NOTDONEYET :
			return("Unimplemented");
			break;
		case D3DRMERR_NOTFOUND :
			return("Object not found in specified place");
			break;
		case D3DRMERR_UNABLETOEXECUTE :
			return("Unable to carry out procedure");
			break;

	}

	sprintf(out, "Unknown: %d", ddrval);
	return (out);
}

