/*
 * GDI mapping mode functions
 *
 * Copyright 1993 Alexandre Julliard
 */

#include "dc.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(gdi)


/***********************************************************************
 *           MAPPING_FixIsotropic
 *
 * Fix viewport extensions for isotropic mode.
 */
void MAPPING_FixIsotropic( DC * dc )
{
    double xdim = (double)dc->vportExtX * dc->w.devCaps->horzSize /
	          (dc->w.devCaps->horzRes * dc->wndExtX);
    double ydim = (double)dc->vportExtY * dc->w.devCaps->vertSize /
	          (dc->w.devCaps->vertRes * dc->wndExtY);
    if (xdim > ydim)
    {
	dc->vportExtX = dc->vportExtX * fabs( ydim / xdim );
	if (!dc->vportExtX) dc->vportExtX = 1;
    }
    else
    {
	dc->vportExtY = dc->vportExtY * fabs( xdim / ydim );
	if (!dc->vportExtY) dc->vportExtY = 1;
    }	
}


/***********************************************************************
 *           DPtoLP16    (GDI.67)
 */
BOOL16 WINAPI DPtoLP16( HDC16 hdc, LPPOINT16 points, INT16 count )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;

    while (count--)
    {
	points->x = XDPTOLP( dc, points->x );
	points->y = YDPTOLP( dc, points->y );
        points++;
    }
    GDI_ReleaseObj( hdc );
    return TRUE;
}


/***********************************************************************
 *           DPtoLP    (GDI32.65)
 */
BOOL WINAPI DPtoLP( HDC hdc, LPPOINT points, INT count )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;

    while (count--)
    {
        if (!INTERNAL_DPTOLP( dc, points ))
	    break;
        points++;
    }
    GDI_ReleaseObj( hdc );
    return (count < 0);
}


/***********************************************************************
 *           LPtoDP16    (GDI.99)
 */
BOOL16 WINAPI LPtoDP16( HDC16 hdc, LPPOINT16 points, INT16 count )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;

    while (count--)
    {
	points->x = XLPTODP( dc, points->x );
	points->y = YLPTODP( dc, points->y );
        points++;
    }
    GDI_ReleaseObj( hdc );
    return TRUE;
}


/***********************************************************************
 *           LPtoDP    (GDI32.247)
 */
BOOL WINAPI LPtoDP( HDC hdc, LPPOINT points, INT count )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;

    while (count--)
    {
	INTERNAL_LPTODP( dc, points );
        points++;
    }
    GDI_ReleaseObj( hdc );
    return TRUE;
}


/***********************************************************************
 *           SetMapMode16    (GDI.3)
 */
INT16 WINAPI SetMapMode16( HDC16 hdc, INT16 mode )
{
    return SetMapMode( hdc, mode );
}


/***********************************************************************
 *           SetMapMode    (GDI32.321)
 */
INT WINAPI SetMapMode( HDC hdc, INT mode )
{
    INT prevMode;
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return 0;
    if (dc->funcs->pSetMapMode)
    {
        prevMode = dc->funcs->pSetMapMode( dc, mode );
        goto done;
    }

    TRACE("%04x %d\n", hdc, mode );
    
    prevMode = dc->w.MapMode;
    switch(mode)
    {
      case MM_TEXT:
	  dc->wndExtX   = 1;
	  dc->wndExtY   = 1;
	  dc->vportExtX = 1;
	  dc->vportExtY = 1;
	  break;
	  
      case MM_LOMETRIC:
      case MM_ISOTROPIC:
	  dc->wndExtX   = dc->w.devCaps->horzSize;
	  dc->wndExtY   = dc->w.devCaps->vertSize;
	  dc->vportExtX = dc->w.devCaps->horzRes / 10;
	  dc->vportExtY = dc->w.devCaps->vertRes / -10;
	  break;
	  
      case MM_HIMETRIC:
	  dc->wndExtX   = dc->w.devCaps->horzSize * 10;
	  dc->wndExtY   = dc->w.devCaps->vertSize * 10;
	  dc->vportExtX = dc->w.devCaps->horzRes / 10;
	  dc->vportExtY = dc->w.devCaps->vertRes / -10;
	  break;
	  
      case MM_LOENGLISH:
	  dc->wndExtX   = dc->w.devCaps->horzSize;
	  dc->wndExtY   = dc->w.devCaps->vertSize;
	  dc->vportExtX = 254L * dc->w.devCaps->horzRes / 1000;
	  dc->vportExtY = -254L * dc->w.devCaps->vertRes / 1000;
	  break;	  
	  
      case MM_HIENGLISH:
	  dc->wndExtX   = dc->w.devCaps->horzSize * 10;
	  dc->wndExtY   = dc->w.devCaps->vertSize * 10;
	  dc->vportExtX = 254L * dc->w.devCaps->horzRes / 1000;
	  dc->vportExtY = -254L * dc->w.devCaps->vertRes / 1000;
	  break;
	  
      case MM_TWIPS:
	  dc->wndExtX   = 144L * dc->w.devCaps->horzSize / 10;
	  dc->wndExtY   = 144L * dc->w.devCaps->vertSize / 10;
	  dc->vportExtX = 254L * dc->w.devCaps->horzRes / 1000;
	  dc->vportExtY = -254L * dc->w.devCaps->vertRes / 1000;
	  break;
	  
      case MM_ANISOTROPIC:
	  break;

      default:
	  goto done;
    }
    dc->w.MapMode = mode;
    DC_UpdateXforms( dc );
 done:
    GDI_ReleaseObj( hdc );
    return prevMode;
}


/***********************************************************************
 *           SetViewportExt    (GDI.14)
 */
DWORD WINAPI SetViewportExt16( HDC16 hdc, INT16 x, INT16 y )
{
    SIZE size;
    if (!SetViewportExtEx( hdc, x, y, &size )) return 0;
    return MAKELONG( size.cx, size.cy );
}


/***********************************************************************
 *           SetViewportExtEx16    (GDI.479)
 */
BOOL16 WINAPI SetViewportExtEx16( HDC16 hdc, INT16 x, INT16 y, LPSIZE16 size )
{
    SIZE size32;
    BOOL16 ret = SetViewportExtEx( hdc, x, y, &size32 );
    if (size) CONV_SIZE32TO16( &size32, size );
    return ret;
}


/***********************************************************************
 *           SetViewportExtEx    (GDI32.340)
 */
BOOL WINAPI SetViewportExtEx( HDC hdc, INT x, INT y, LPSIZE size )
{
    BOOL ret = TRUE;
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;
    if (dc->funcs->pSetViewportExt)
    {
        ret = dc->funcs->pSetViewportExt( dc, x, y );
        goto done;
    }
    if (size)
    {
	size->cx = dc->vportExtX;
	size->cy = dc->vportExtY;
    }
    if ((dc->w.MapMode != MM_ISOTROPIC) && (dc->w.MapMode != MM_ANISOTROPIC))
	goto done;
    if (!x || !y)
    {
        ret = FALSE;
        goto done;
    }
    dc->vportExtX = x;
    dc->vportExtY = y;
    if (dc->w.MapMode == MM_ISOTROPIC) MAPPING_FixIsotropic( dc );
    DC_UpdateXforms( dc );
 done:
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           SetViewportOrg    (GDI.13)
 */
DWORD WINAPI SetViewportOrg16( HDC16 hdc, INT16 x, INT16 y )
{
    POINT pt;
    if (!SetViewportOrgEx( hdc, x, y, &pt )) return 0;
    return MAKELONG( pt.x, pt.y );
}


/***********************************************************************
 *           SetViewportOrgEx16    (GDI.480)
 */
BOOL16 WINAPI SetViewportOrgEx16( HDC16 hdc, INT16 x, INT16 y, LPPOINT16 pt )
{
    POINT pt32;
    BOOL16 ret = SetViewportOrgEx( hdc, x, y, &pt32 );
    if (pt) CONV_POINT32TO16( &pt32, pt );
    return ret;
}


/***********************************************************************
 *           SetViewportOrgEx    (GDI32.341)
 */
BOOL WINAPI SetViewportOrgEx( HDC hdc, INT x, INT y, LPPOINT pt )
{
    BOOL ret = TRUE;
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;
    if (dc->funcs->pSetViewportOrg)
        ret = dc->funcs->pSetViewportOrg( dc, x, y );
    else
    {
        if (pt)
        {
            pt->x = dc->vportOrgX;
            pt->y = dc->vportOrgY;
        }
        dc->vportOrgX = x;
        dc->vportOrgY = y;
        DC_UpdateXforms( dc );
    }
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           SetWindowExt    (GDI.12)
 */
DWORD WINAPI SetWindowExt16( HDC16 hdc, INT16 x, INT16 y )
{
    SIZE size;
    if (!SetWindowExtEx( hdc, x, y, &size )) return 0;
    return MAKELONG( size.cx, size.cy );
}


/***********************************************************************
 *           SetWindowExtEx16    (GDI.481)
 */
BOOL16 WINAPI SetWindowExtEx16( HDC16 hdc, INT16 x, INT16 y, LPSIZE16 size )
{
    SIZE size32;
    BOOL16 ret = SetWindowExtEx( hdc, x, y, &size32 );
    if (size) CONV_SIZE32TO16( &size32, size );
    return ret;
}


/***********************************************************************
 *           SetWindowExtEx    (GDI32.344)
 */
BOOL WINAPI SetWindowExtEx( HDC hdc, INT x, INT y, LPSIZE size )
{
    BOOL ret = TRUE;
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;
    if (dc->funcs->pSetWindowExt)
    {
        ret = dc->funcs->pSetWindowExt( dc, x, y );
        goto done;
    }
    if (size)
    {
	size->cx = dc->wndExtX;
	size->cy = dc->wndExtY;
    }
    if ((dc->w.MapMode != MM_ISOTROPIC) && (dc->w.MapMode != MM_ANISOTROPIC))
	goto done;
    if (!x || !y)
    {
        ret = FALSE;
        goto done;
    }
    dc->wndExtX = x;
    dc->wndExtY = y;
    if (dc->w.MapMode == MM_ISOTROPIC) MAPPING_FixIsotropic( dc );
    DC_UpdateXforms( dc );
 done:
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           SetWindowOrg    (GDI.11)
 */
DWORD WINAPI SetWindowOrg16( HDC16 hdc, INT16 x, INT16 y )
{
    POINT pt;
    if (!SetWindowOrgEx( hdc, x, y, &pt )) return 0;
    return MAKELONG( pt.x, pt.y );
}


/***********************************************************************
 *           SetWindowOrgEx16    (GDI.482)
 */
BOOL16 WINAPI SetWindowOrgEx16( HDC16 hdc, INT16 x, INT16 y, LPPOINT16 pt )
{
    POINT pt32;
    BOOL16 ret = SetWindowOrgEx( hdc, x, y, &pt32 );
    if (pt) CONV_POINT32TO16( &pt32, pt );
    return ret;
}


/***********************************************************************
 *           SetWindowOrgEx    (GDI32.345)
 */
BOOL WINAPI SetWindowOrgEx( HDC hdc, INT x, INT y, LPPOINT pt )
{
    BOOL ret = TRUE;
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;
    if (dc->funcs->pSetWindowOrg) ret = dc->funcs->pSetWindowOrg( dc, x, y );
    else
    {
        if (pt)
        {
            pt->x = dc->wndOrgX;
            pt->y = dc->wndOrgY;
        }
        dc->wndOrgX = x;
        dc->wndOrgY = y;
        DC_UpdateXforms( dc );
    }
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           OffsetViewportOrg    (GDI.17)
 */
DWORD WINAPI OffsetViewportOrg16( HDC16 hdc, INT16 x, INT16 y )
{
    POINT pt;
    if (!OffsetViewportOrgEx( hdc, x, y, &pt )) return 0;
    return MAKELONG( pt.x, pt.y );
}


/***********************************************************************
 *           OffsetViewportOrgEx16    (GDI.476)
 */
BOOL16 WINAPI OffsetViewportOrgEx16( HDC16 hdc, INT16 x, INT16 y, LPPOINT16 pt)
{
    POINT pt32;
    BOOL16 ret = OffsetViewportOrgEx( hdc, x, y, &pt32 );
    if (pt) CONV_POINT32TO16( &pt32, pt );
    return ret;
}


/***********************************************************************
 *           OffsetViewportOrgEx    (GDI32.257)
 */
BOOL WINAPI OffsetViewportOrgEx( HDC hdc, INT x, INT y, LPPOINT pt)
{
    BOOL ret = TRUE;
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;
    if (dc->funcs->pOffsetViewportOrg)
        ret = dc->funcs->pOffsetViewportOrg( dc, x, y );
    else
    {
        if (pt)
        {
            pt->x = dc->vportOrgX;
            pt->y = dc->vportOrgY;
        }
        dc->vportOrgX += x;
        dc->vportOrgY += y;
        DC_UpdateXforms( dc );
    }
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           OffsetWindowOrg    (GDI.15)
 */
DWORD WINAPI OffsetWindowOrg16( HDC16 hdc, INT16 x, INT16 y )
{
    POINT pt;
    if (!OffsetWindowOrgEx( hdc, x, y, &pt )) return 0;
    return MAKELONG( pt.x, pt.y );
}


/***********************************************************************
 *           OffsetWindowOrgEx16    (GDI.477)
 */
BOOL16 WINAPI OffsetWindowOrgEx16( HDC16 hdc, INT16 x, INT16 y, LPPOINT16 pt )
{
    POINT pt32;
    BOOL16 ret = OffsetWindowOrgEx( hdc, x, y, &pt32 );
    if (pt) CONV_POINT32TO16( &pt32, pt );
    return ret;
}


/***********************************************************************
 *           OffsetWindowOrgEx    (GDI32.258)
 */
BOOL WINAPI OffsetWindowOrgEx( HDC hdc, INT x, INT y, LPPOINT pt )
{
    BOOL ret = TRUE;
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;
    if (dc->funcs->pOffsetWindowOrg)
        ret = dc->funcs->pOffsetWindowOrg( dc, x, y );
    else
    {
        if (pt)
        {
            pt->x = dc->wndOrgX;
            pt->y = dc->wndOrgY;
        }
        dc->wndOrgX += x;
        dc->wndOrgY += y;
        DC_UpdateXforms( dc );
    }
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           ScaleViewportExt    (GDI.18)
 */
DWORD WINAPI ScaleViewportExt16( HDC16 hdc, INT16 xNum, INT16 xDenom,
                               INT16 yNum, INT16 yDenom )
{
    SIZE size;
    if (!ScaleViewportExtEx( hdc, xNum, xDenom, yNum, yDenom, &size ))
        return FALSE;
    return MAKELONG( size.cx,  size.cy );
}


/***********************************************************************
 *           ScaleViewportExtEx16    (GDI.484)
 */
BOOL16 WINAPI ScaleViewportExtEx16( HDC16 hdc, INT16 xNum, INT16 xDenom,
                                    INT16 yNum, INT16 yDenom, LPSIZE16 size )
{
    SIZE size32;
    BOOL16 ret = ScaleViewportExtEx( hdc, xNum, xDenom, yNum, yDenom,
                                       &size32 );
    if (size) CONV_SIZE32TO16( &size32, size );
    return ret;
}


/***********************************************************************
 *           ScaleViewportExtEx    (GDI32.293)
 */
BOOL WINAPI ScaleViewportExtEx( HDC hdc, INT xNum, INT xDenom,
                                    INT yNum, INT yDenom, LPSIZE size )
{
    BOOL ret = TRUE;
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;
    if (dc->funcs->pScaleViewportExt)
    {
        ret = dc->funcs->pScaleViewportExt( dc, xNum, xDenom, yNum, yDenom );
        goto done;
    }
    if (size)
    {
	size->cx = dc->vportExtX;
	size->cy = dc->vportExtY;
    }
    if ((dc->w.MapMode != MM_ISOTROPIC) && (dc->w.MapMode != MM_ANISOTROPIC))
	goto done;
    if (!xNum || !xDenom || !xNum || !yDenom)
    {
        ret = FALSE;
        goto done;
    }
    dc->vportExtX = (dc->vportExtX * xNum) / xDenom;
    dc->vportExtY = (dc->vportExtY * yNum) / yDenom;
    if (dc->vportExtX == 0) dc->vportExtX = 1;
    if (dc->vportExtY == 0) dc->vportExtY = 1;
    if (dc->w.MapMode == MM_ISOTROPIC) MAPPING_FixIsotropic( dc );
    DC_UpdateXforms( dc );
 done:
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           ScaleWindowExt    (GDI.16)
 */
DWORD WINAPI ScaleWindowExt16( HDC16 hdc, INT16 xNum, INT16 xDenom,
                             INT16 yNum, INT16 yDenom )
{
    SIZE size;
    if (!ScaleWindowExtEx( hdc, xNum, xDenom, yNum, yDenom, &size ))
        return FALSE;
    return MAKELONG( size.cx,  size.cy );
}


/***********************************************************************
 *           ScaleWindowExtEx16    (GDI.485)
 */
BOOL16 WINAPI ScaleWindowExtEx16( HDC16 hdc, INT16 xNum, INT16 xDenom,
                                  INT16 yNum, INT16 yDenom, LPSIZE16 size )
{
    SIZE size32;
    BOOL16 ret = ScaleWindowExtEx( hdc, xNum, xDenom, yNum, yDenom,
                                     &size32 );
    if (size) CONV_SIZE32TO16( &size32, size );
    return ret;
}


/***********************************************************************
 *           ScaleWindowExtEx    (GDI32.294)
 */
BOOL WINAPI ScaleWindowExtEx( HDC hdc, INT xNum, INT xDenom,
                                  INT yNum, INT yDenom, LPSIZE size )
{
    BOOL ret = TRUE;
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;
    if (dc->funcs->pScaleWindowExt)
    {
        ret = dc->funcs->pScaleWindowExt( dc, xNum, xDenom, yNum, yDenom );
        goto done;
    }
    if (size)
    {
	size->cx = dc->wndExtX;
	size->cy = dc->wndExtY;
    }
    if ((dc->w.MapMode != MM_ISOTROPIC) && (dc->w.MapMode != MM_ANISOTROPIC))
	goto done;
    if (!xNum || !xDenom || !xNum || !yDenom)
    {
        ret = FALSE;
        goto done;
    }
    dc->wndExtX = (dc->wndExtX * xNum) / xDenom;
    dc->wndExtY = (dc->wndExtY * yNum) / yDenom;
    if (dc->wndExtX == 0) dc->wndExtX = 1;
    if (dc->wndExtY == 0) dc->wndExtY = 1;
    if (dc->w.MapMode == MM_ISOTROPIC) MAPPING_FixIsotropic( dc );
    DC_UpdateXforms( dc );
 done:
    GDI_ReleaseObj( hdc );
    return ret;
}
