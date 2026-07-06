// qMViewView.cpp : implementation of the CQMViewView class
//

#include "stdafx.h"
#include "resource.h"
#include "DDUtil.h"
#include "Matrix.h"
#include "Model.h"
#include "FlexModel.h"
#include "qMView.h"

#include "qMViewDoc.h"
#include "qMViewView.h"
#include "treectrlex.h"
#include "frametreectrl.h"
#include "skintreectrl.h"

#include "SkinPageFrm.h"
#include "JointConstraintDlg.h"
#include "JointAnglesDlg.h"
#include "ManagerTree.h"
#include "FrameManager2.h"

#include "MainFrm.h"
#include "TickerDlg.h"

extern pal_t qpal[];
extern pal_t h2pal[];

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CQMViewView

IMPLEMENT_DYNCREATE(CQMViewView, CView)

BEGIN_MESSAGE_MAP(CQMViewView, CView)
	//{{AFX_MSG_MAP(CQMViewView)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_COMMAND(ID_ALL_LOCK, OnAllLock)
	ON_COMMAND(ID_ALL_UNLOCK, OnAllUnlock)
	ON_COMMAND(ID_GOTOPOS_BACK, OnGotoposBack)
	ON_COMMAND(ID_GOTOPOS_BOTTOM, OnGotoposBottom)
	ON_COMMAND(ID_GOTOPOS_FRONT, OnGotoposFront)
	ON_COMMAND(ID_GOTOPOS_LEFT, OnGotoposLeft)
	ON_COMMAND(ID_GOTOPOS_RIGHT, OnGotoposRight)
	ON_COMMAND(ID_GOTOPOS_TOP, OnGotoposTop)
	ON_COMMAND(ID_LOCK_ROT_X, OnLockRotX)
	ON_UPDATE_COMMAND_UI(ID_LOCK_ROT_X, OnUpdateLockRotX)
	ON_COMMAND(ID_LOCK_ROT_Y, OnLockRotY)
	ON_UPDATE_COMMAND_UI(ID_LOCK_ROT_Y, OnUpdateLockRotY)
	ON_COMMAND(ID_LOCK_ROT_Z, OnLockRotZ)
	ON_UPDATE_COMMAND_UI(ID_LOCK_ROT_Z, OnUpdateLockRotZ)
	ON_COMMAND(ID_LOCK_SCALE_X, OnLockScaleX)
	ON_UPDATE_COMMAND_UI(ID_LOCK_SCALE_X, OnUpdateLockScaleX)
	ON_COMMAND(ID_LOCK_SCALE_Y, OnLockScaleY)
	ON_UPDATE_COMMAND_UI(ID_LOCK_SCALE_Y, OnUpdateLockScaleY)
	ON_COMMAND(ID_LOCK_SCALE_Z, OnLockScaleZ)
	ON_UPDATE_COMMAND_UI(ID_LOCK_SCALE_Z, OnUpdateLockScaleZ)
	ON_COMMAND(ID_LOCK_TRAN_X, OnLockTranX)
	ON_UPDATE_COMMAND_UI(ID_LOCK_TRAN_X, OnUpdateLockTranX)
	ON_COMMAND(ID_LOCK_TRAN_Y, OnLockTranY)
	ON_UPDATE_COMMAND_UI(ID_LOCK_TRAN_Y, OnUpdateLockTranY)
	ON_COMMAND(ID_LOCK_TRAN_Z, OnLockTranZ)
	ON_UPDATE_COMMAND_UI(ID_LOCK_TRAN_Z, OnUpdateLockTranZ)
	ON_COMMAND(ID_RENDER_TRANS, OnRenderTrans)
	ON_UPDATE_COMMAND_UI(ID_RENDER_TRANS, OnUpdateRenderTrans)
	ON_COMMAND(ID_RENDER_TEXTURE, OnRenderTexture)
	ON_UPDATE_COMMAND_UI(ID_RENDER_TEXTURE, OnUpdateRenderTexture)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_COMMAND(ID_GOTOPOS_WEAPON, OnGotoposWeapon)
	ON_COMMAND(ID_TICKER_OPTIONS, OnTickerOptions)
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_COMMAND(ID_ANIMATE_STEP_BACK, OnAnimateStepBack)
	ON_COMMAND(ID_ANIMATE_STEP_FORE, OnAnimateStepFore)
	ON_COMMAND(ID_ANIM_INTERTOGGLE, OnAnimIntertoggle)
	ON_UPDATE_COMMAND_UI(ID_ANIM_INTERTOGGLE, OnUpdateAnimIntertoggle)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CQMViewView construction/destruction

CQMViewView::CQMViewView()
{
	// TODO: add construction code here
	m_clipper = NULL;
	m_device = NULL;
	m_frame = NULL;
	m_camera = NULL;
	m_viewport = NULL;
	m_scene = NULL;
	m_transtate = false;
	m_curstate = true;
	m_drag = false;
	m_r_drag = false;
	m_globalField = 0.5;
	m_interOn = false;
	m_last_x = 0;
	m_last_y = 0;
	m_r_last_x = 0;
	m_r_last_y = 0;

	m_lock_rot_x = false;
	m_lock_rot_y = false;
	m_lock_rot_z = false;
	m_lock_scale_x = false;
	m_lock_scale_y = false;
	m_lock_scale_z = false;
	m_lock_trans_x = false;
	m_lock_trans_y = false;
	m_lock_trans_z = false;
	m_d3drm_up = false;
	m_d3drm = NULL;
}

CQMViewView::~CQMViewView()
{
}

BOOL CQMViewView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
//	cs.style = WS_CHILD | WS_VISIBLE | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU
//		| FWS_ADDTOTITLE | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_MAXIMIZE;

	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CQMViewView drawing

void CQMViewView::OnDraw(CDC* pDC)
{
	CQMViewDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	// TODO: add draw code for native data here
}

/////////////////////////////////////////////////////////////////////////////
// CQMViewView diagnostics

#ifdef _DEBUG
void CQMViewView::AssertValid() const
{
	CView::AssertValid();
}

void CQMViewView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CQMViewDoc* CQMViewView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CQMViewDoc)));
	return (CQMViewDoc*)m_pDocument;
}

#endif //_DEBUG

LPDIRECT3DRM2 CQMViewView::GetD3D()
{
	return m_d3drm;
}

/////////////////////////////////////////////////////////////////////////////
// CQMViewView message handlers
BOOL CQMViewView::CreateDevice()
{
	HRESULT r = DirectDrawCreateClipper(0, &m_clipper, NULL );
	if (r != D3DRM_OK)
	{
		TRACE("failed to create D3D clipper\n");
		return FALSE;
	}

	r = m_clipper->SetHWnd( NULL, m_hWnd );
	if (r != D3DRM_OK)
	{
		TRACE("failed in SetHWnd call\n");
		return FALSE;
	}

	RECT rect;
	::GetClientRect( m_hWnd, &rect );
//	GetGUID();
	r = m_d3drm->CreateDeviceFromClipper(m_clipper, NULL, //&guid,
			rect.right, rect.bottom,
			&m_device );

	if (r!=D3DRM_OK)
	{
		AfxMessageBox("CreateDeviceFromClipper() failed");
		return FALSE;
	}

	m_device->SetQuality( D3DRMRENDER_FLAT );

	HDC hdc = ::GetDC( m_hWnd );
	int bpp = ::GetDeviceCaps( hdc, BITSPIXEL );
	::ReleaseDC( m_hWnd, hdc );

	switch ( bpp )
	{
   	case 1:
		m_device->SetShades( 4 );
		m_d3drm->SetDefaultTextureShades( 4 );
		m_device->SetDither( TRUE );
		break;
	case 8:
		m_device->SetDither( FALSE );
		break;
	case 16:
		m_device->SetShades( 32 );
		m_d3drm->SetDefaultTextureColors( 64 );
		m_d3drm->SetDefaultTextureShades( 32 );
		m_device->SetDither( FALSE );
		break;
	case 24:
	case 32:
		m_device->SetShades( 256 );
		m_d3drm->SetDefaultTextureColors( 64 );
		m_d3drm->SetDefaultTextureShades( 256 );
		m_device->SetDither( FALSE );
		break;
	}

	m_d3drm->CreateFrame( 0, &m_scene );

	if (CreateScene()==FALSE)
	{
		AfxMessageBox("CreateScene() failed");
		return FALSE;
	}

	ASSERT(m_camera);
	ASSERT(m_viewport);
	m_viewport->SetBack( D3DVALUE(8000) );

    CDDHelper::lpDD->SetCooperativeLevel( m_hWnd, DDSCL_NORMAL );

//	r = d3drm->QueryInterface(IID_IDirect3DRM2, (LPVOID *) &d3drm2);

//	if (r != D3D_OK)
//		AfxMessageBox("Cannot create reference to IID_IDirect3DRM2!\n");

	return TRUE;
}

D3DVALUE CQMViewView::ScaleMesh( LPDIRECT3DRMMESHBUILDER mesh, D3DVALUE dim)
{
	D3DRMBOX box;
	mesh->GetBox( &box );
	D3DVALUE sizex = box.max.x - box.min.x;
	D3DVALUE sizey = box.max.y - box.min.y;
	D3DVALUE sizez = box.max.z - box.min.z;
	D3DVALUE largedim=D3DVALUE(0);
	if (sizex>largedim)
		largedim=sizex;
	if (sizey>largedim)
		largedim=sizey;
	if (sizez>largedim)
		largedim=sizez;
	D3DVALUE scalefactor = dim/largedim;
	mesh->Scale( scalefactor, scalefactor, scalefactor );
	return scalefactor;
}


/*GUID* CQMViewView::GetGUID()
{
	HRESULT r;

	D3DFINDDEVICESEARCH searchdata;
	memset(&searchdata, 0, sizeof searchdata);
	searchdata.dwSize = sizeof searchdata;
	searchdata.dwFlags = D3DFDS_COLORMODEL;
	searchdata.dcmColorModel = colormodel;

	D3DFINDDEVICERESULT resultdata;
	memset( &resultdata, 0, sizeof resultdata );
	resultdata.dwSize = sizeof resultdata;

	LPDIRECTDRAW ddraw;
	r = DirectDrawCreate( 0, &ddraw, 0 );
	if (r!=DD_OK)
	{
		AfxMessageBox("DirectDrawCreate() failed");
		return 0;
	}

	LPDIRECT3D d3d;
	r = ddraw->QueryInterface( IID_IDirect3D, (void**)&d3d );
	if ( r != D3DRM_OK )
	{
		AfxMessageBox("d3drm->QueryInterface() failed\n");
		ddraw->Release();
		ddraw = NULL;
		return 0;
	}

	r=d3d->FindDevice( &searchdata, &resultdata );
	if ( r==D3D_OK )
		guid = resultdata.guid;
	else
	{
		AfxMessageBox("FindDevice() failed");
	}

	d3d->Release();
	d3d = NULL;
	ddraw->Release();
	ddraw = NULL;

	return &guid;
}*/

BOOL CQMViewView::CreateScene()
{
	m_scene->SetSceneBackgroundRGB( D3DVALUE(.3), D3DVALUE(.3), D3DVALUE(.3) );

	m_d3drm->CreateFrame (m_scene, &m_frame);
	m_frame->SetOrientation(m_scene,
							D3DVALUE(0), D3DVALUE(1), D3DVALUE(0),
							D3DVALUE(0), D3DVALUE(0), D3DVALUE(1) );

	m_frame->AddMoveCallback( UpdateDrag, (void*)this);

	// ---------DIRECTIONAL LIGHT--------
	LPDIRECT3DRMLIGHT dlight;
	m_d3drm->CreateLightRGB( D3DRMLIGHT_AMBIENT,
			D3DVALUE(1.00), D3DVALUE(1.00), D3DVALUE(1.00),
			&dlight);

	LPDIRECT3DRMFRAME2 dlightframe;
	m_d3drm->CreateFrame(m_scene, &dlightframe );
	dlightframe->AddLight( dlight );
	dlightframe->SetOrientation(m_scene,
			D3DVALUE(0), D3DVALUE(-1), D3DVALUE(1),
			D3DVALUE(0), D3DVALUE(1), D3DVALUE(0));

	dlight->Release();
	dlight=0;
	dlightframe->Release();
	dlightframe=0;

	//------ CAMERA-----------
	m_d3drm->CreateFrame(m_scene, &m_camera);
	m_camera->SetPosition(m_scene, D3DVALUE(0), D3DVALUE(0), D3DVALUE(1.0));
	m_d3drm->CreateViewport(m_device, m_camera, 0, 0,
			m_device->GetWidth(),m_device->GetHeight(),
			&m_viewport);

	m_d3drm_up = true;

	return TRUE;
}

void CQMViewView::CreateDot(vec3_t origin, int r, int g , int b)
{
	D3DRMGROUPINDEX		group;
	LPDIRECT3DRMMESH	m_originMesh;
	D3DRMVERTEX			vertexlist[12];

	unsigned			vertorder[24];
	unsigned			numread = 0;

	vertexlist[0].position.x = origin.x - 1;
	vertexlist[0].position.y = origin.y;
	vertexlist[0].position.z = origin.z;

	vertexlist[2].position.x = origin.x + 1;
	vertexlist[2].position.y = origin.y;
	vertexlist[2].position.z = origin.z;

	vertexlist[1].position.x = origin.x;
	vertexlist[1].position.y = origin.y;
	vertexlist[1].position.z = origin.z + 1;

	vertexlist[3].position.x = origin.x;
	vertexlist[3].position.y = origin.y;
	vertexlist[3].position.z = origin.z - 1;

	vertexlist[4].position.x = origin.x;
	vertexlist[4].position.y = origin.y + 1;
	vertexlist[4].position.z = origin.z;

	vertexlist[5].position.x = origin.x;
	vertexlist[5].position.y = origin.y - 1;
	vertexlist[5].position.z = origin.z;

	vertorder[0] = 1;
	vertorder[1] = 4;
	vertorder[2] = 0;

	vertorder[3] = 4;
	vertorder[4] = 3;
	vertorder[5] = 0;

	vertorder[6] = 1;
	vertorder[7] = 2;
	vertorder[8] = 4;

	vertorder[9] = 2;
	vertorder[10] = 3;
	vertorder[11] = 4;

	//////////////////

	vertorder[12] = 3;
	vertorder[13] = 5;
	vertorder[14] = 0;

	vertorder[15] = 5;
	vertorder[16] = 1;
	vertorder[17] = 0;

	vertorder[18] = 3;
	vertorder[19] = 2;
	vertorder[20] = 5;

	vertorder[21] = 2;
	vertorder[22] = 1;
	vertorder[23] = 5;

	m_d3drm->CreateMesh(&m_originMesh);

	m_originMesh->AddGroup( 24, 8, 3, vertorder, &group );
	m_originMesh->SetVertices(group, 0, 23, vertexlist );

	m_originMesh->SetGroupQuality(group, D3DRMRENDER_FLAT );
	m_originMesh->SetGroupColorRGB(group, D3DVALUE( r ), D3DVALUE( g ), D3DVALUE( b ));

	HRESULT ddrval = m_frame->AddVisual(m_originMesh);
	if (ddrval != D3DRM_OK)
		AfxMessageBox(CDDHelper::TraceError(ddrval));

	m_originMesh->Release();
}

void CQMViewView::CreateOriginVisual()
{
	vec3_t	org;

	org.z = 0;
	org.y = 0;
	org.x = 0;

	CreateDot( org, 255, 0, 0 );

	org.x = -10;
	org.y = 0;
	org.z = 0;

	CreateDot( org, 0, 255, 0 );

	org.x = 0;
	org.y = -10;
	org.z = 0;

	CreateDot( org, 0, 0, 255 );

	org.x = 0;
	org.y = 0;
	org.z = -10;

	CreateDot( org, 255, 255, 0 );
}

int CQMViewView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_drag = false;
	m_r_drag = false;

	m_last_x = 0;
	m_last_y = 0;
	m_r_last_x = 0;
	m_r_last_y = 0;

	//Setup the D3DDevice
	HRESULT r = Direct3DRMCreate((LPDIRECT3DRM*)&m_d3drm);
	if (r != D3DRM_OK)
	{
		TRACE("failed to create D3DRM object\n");
		return -1;
	}
	SetColorModel(D3DCOLOR_RGB);
	return 0;
}

void CQMViewView::OnPaint()
{
	if (m_device == NULL)
	{
		if (!CreateDevice())
		{
			PostQuitMessage(0);
		}
	}
	if (GetUpdateRect( 0, FALSE )==FALSE)
		return;

	if (m_device)
	{
		LPDIRECT3DRMWINDEVICE windev;
		PAINTSTRUCT ps;
		BeginPaint(&ps);
		if (m_device->QueryInterface(IID_IDirect3DRMWinDevice, (void**)&windev)==0)
		{
			if (windev->HandlePaint(ps.hdc)!=0)
				AfxMessageBox("windev->HandlePaint() failure");
			windev->Release();
			windev = NULL;
		}
		else
			AfxMessageBox("Failed to create Windows device to handle WM_PAINT");

		EndPaint(&ps);
	}
}

void CQMViewView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)
{
	// TODO: Add your specialized code here and/or call the base class
	CModel* theModel = GetDocument()->GetModel();
	if (theModel != NULL)
	{
		bool useTimer;
		long timerDelay;
		theModel->GetTimerData(&timerDelay, &useTimer);
		if (useTimer)
		{
			if (bActivate)
			{
				SetTimer(1, timerDelay, NULL);
			}
			else
			{
				KillTimer(1);
			}
		}
	}

	LPDIRECT3DRMWINDEVICE windev;
	if (m_device)
	{
		if (m_device->QueryInterface( IID_IDirect3DRMWinDevice, (void**)&windev)==0)
		{
			if (windev->HandleActivate((WPARAM)bActivate) )
				AfxMessageBox("windev->HandleActivate() failure");
			windev->Release();
			windev = NULL;
		}
		else
			AfxMessageBox("device->QueryInterface(WinDevice) failure");
	}
	CView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}

void CQMViewView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);

	if (!m_device)
		return;

	int width = cx;
	int height = cy;

	if (width && height)
	{
		int view_width = m_viewport->GetWidth();
		int view_height = m_viewport->GetHeight();
		int dev_width = m_device->GetWidth();
		int dev_height = m_device->GetHeight();

		if (view_width == width && view_height == height)
			return;

		int old_dither = m_device->GetDither();
		D3DRMRENDERQUALITY old_quality = m_device->GetQuality();
		int old_shades = m_device->GetShades();

		m_viewport->Release();
		m_viewport = NULL;
		m_device->Release();
		m_device = NULL;
//		GetGUID();
		m_d3drm->CreateDeviceFromClipper(m_clipper, NULL,/*&guid,*/ width, height, &m_device );

		m_device->SetDither(old_dither);
		m_device->SetQuality(old_quality);
		m_device->SetShades(old_shades);

		width = m_device->GetWidth();
		height = m_device->GetHeight();
		m_d3drm->CreateViewport(m_device, m_camera, 0, 0, width, height, &m_viewport);

		m_viewport->SetBack( D3DVALUE(8000) );
	}
}

BOOL CQMViewView::OnEraseBkgnd(CDC* pDC)
{
	COLORREF bgcolor;
	if (m_scene)
	{
		D3DCOLOR scenecolor=m_scene->GetSceneBackground();
		bgcolor=D3DCOLOR_2_COLORREF(scenecolor);
	}
	else
		bgcolor=RGB(0,0,0);

	CBrush br( bgcolor );
	CRect rc;
	GetClientRect(&rc);
	pDC->FillRect(&rc, &br);

	return TRUE;
}

/*
 *	UpdateDrag
 *
 *		Mesh animation callback function
 *
 */
void CQMViewView::UpdateDrag(LPDIRECT3DRMFRAME frame, void* data, D3DVALUE)
{
	D3DVECTOR dv;

	CQMViewView* view = (CQMViewView*)data;
	CModel* model = view->GetDocument()->GetModel();

	POINT curMouse;
	GetCursorPos(&curMouse);
	view->ScreenToClient(&curMouse);

	if (view->m_drag)
	{
		double delta_x = curMouse.x - view->m_last_x;
		double delta_y = curMouse.y - view->m_last_y;

		if (delta_x || delta_y) view->m_has_moved = TRUE;

		if (view->m_lock_rot_x) delta_x = 0;
		if (view->m_lock_rot_y) delta_y = 0;

		view->m_last_x = curMouse.x;
		view->m_last_y = curMouse.y;
		double delta_r = sqrt(delta_x * delta_x + delta_y * delta_y);
		double radius = 50;
		double denom= sqrt(radius * radius + delta_r * delta_r);
		if (fabs(delta_r) > 1)
		{
			if (((CMainFrame*)view->GetParent())->GetManipulationType() == MANIPULATE_CAMERA)
			{
				if (!(delta_r == 0 || denom == 0))
				{
					if (GetKeyState(VK_SHIFT)&0x8000)
					{
						frame->SetRotation( 0,
								D3DVALUE(0.0),
								D3DVALUE(0.0),
								D3DVALUE(1.0),
								D3DVALUE(-delta_x / 100) );
						frame->SetVelocity(0,0,0,0,true);
					}
					else
					{
						frame->SetRotation( 0,
								D3DDivide(-delta_y, delta_r),
								D3DDivide(-delta_x, delta_r),
								D3DVALUE(0.0),
								D3DDivide(delta_r, denom) );
						frame->SetVelocity(0,0,0,0,true);
					}
				}
			}
			else
			{
				if (model != NULL)
				{
					model->Drag(delta_x, delta_y);
					model->ShowFrame(view->m_d3drm, view->GetDC());
				}
			}
		}
		else
		{
			frame->SetRotation( 0, 0, 0, D3DVALUE(0.0), 0);
			frame->SetVelocity(0,0,0,0,true);
		}
	}

	if (view->m_r_drag)
	{
		double delta_x = curMouse.x - view->m_r_last_x;
		double delta_y = curMouse.y - view->m_r_last_y;

		if (delta_x || delta_y) view->m_r_has_moved = TRUE;

		view->m_r_last_x = curMouse.x;
		view->m_r_last_y = curMouse.y;

		frame->GetPosition(view->m_scene, &dv );

		if (!view->m_lock_trans_x) dv.x += D3DVALUE(delta_x / 10);
		if (!view->m_lock_trans_y) dv.y -= D3DVALUE(delta_y / 10);
		dv.z =  D3DVALUE(0.0);

		frame->SetPosition(view->m_scene, D3DVALUE(dv.x), D3DVALUE(dv.y), D3DVALUE(0) );

		if ((GetKeyState(VK_CONTROL)&0x8000) && (model != NULL))
		{
			model->SetCurScale(model->GetCurScale() + delta_y);
			view->m_camera->SetPosition(view->m_scene, D3DVALUE(0), D3DVALUE(0), D3DVALUE(model->GetCurScale()));
		}
	}

}

void CQMViewView::OnAllLock()
{
	m_lock_rot_x = true;
	m_lock_rot_y = true;
	m_lock_rot_z = true;
	m_lock_scale_x = true;
	m_lock_scale_y = true;
	m_lock_scale_z = true;
	m_lock_trans_x = true;
	m_lock_trans_y = true;
	m_lock_trans_z = true;
}

void CQMViewView::OnAllUnlock()
{
	m_lock_rot_x = false;
	m_lock_rot_y = false;
	m_lock_rot_z = false;
	m_lock_scale_x = false;
	m_lock_scale_y = false;
	m_lock_scale_z = false;
	m_lock_trans_x = false;
	m_lock_trans_y = false;
	m_lock_trans_z = false;
}

void CQMViewView::OnGotoposBack()
{
	m_frame->SetPosition(m_scene,
				D3DVALUE(0), D3DVALUE(0), D3DVALUE(0) );

	m_frame->SetRotation( 0,
				D3DVAL(0.0), D3DVAL(1.0), D3DVAL(0.0),
				D3DVAL(0.0) );

	m_frame->SetVelocity(0,0,0,0,true);
	CModel* model = GetDocument()->GetModel();
	if (model == NULL)
	{
		return;
	}
	model->SetBackOrientation(m_frame, m_scene);
}

void CQMViewView::OnGotoposBottom()
{
	m_frame->SetPosition(m_scene,
				D3DVALUE(0), D3DVALUE(0), D3DVALUE(0) );

	m_frame->SetRotation( 0,
				D3DVAL(0.0), D3DVAL(1.0), D3DVAL(0.0),
				D3DVAL(0.0) );

	m_frame->SetVelocity(0,0,0,0,true);
	CModel* model = GetDocument()->GetModel();
	if (model == NULL)
	{
		return;
	}
	model->SetBottomOrientation(m_frame, m_scene);
}

void CQMViewView::OnGotoposFront()
{
	m_frame->SetPosition(m_scene,
				D3DVALUE(0), D3DVALUE(0), D3DVALUE(0) );

	m_frame->SetRotation( 0,
				D3DVAL(0.0), D3DVAL(1.0), D3DVAL(0.0),
				D3DVAL(0.0) );

	m_frame->SetVelocity(0,0,0,0,true);
	CModel* model = GetDocument()->GetModel();
	if (model == NULL)
	{
		return;
	}
	model->SetFrontOrientation(m_frame, m_scene);
}

void CQMViewView::OnGotoposLeft()
{
	m_frame->SetPosition(m_scene,
				D3DVALUE(0), D3DVALUE(0), D3DVALUE(0) );

	m_frame->SetRotation( 0,
				D3DVAL(0.0), D3DVAL(1.0), D3DVAL(0.0),
				D3DVAL(0.0) );

	m_frame->SetVelocity(0,0,0,0,true);
	CModel* model = GetDocument()->GetModel();
	if (model == NULL)
	{
		return;
	}
	model->SetLeftOrientation(m_frame, m_scene);
}

void CQMViewView::OnGotoposRight()
{
	m_frame->SetPosition(m_scene,
				D3DVALUE(0), D3DVALUE(0), D3DVALUE(0) );

	m_frame->SetRotation( 0,
				D3DVAL(0.0), D3DVAL(1.0), D3DVAL(0.0),
				D3DVAL(0.0) );

	m_frame->SetVelocity(0,0,0,0,true);
	CModel* model = GetDocument()->GetModel();
	if (model == NULL)
	{
		return;
	}
	model->SetRightOrientation(m_frame, m_scene);
}

void CQMViewView::OnGotoposTop()
{
	m_frame->SetPosition(m_scene,
				D3DVALUE(0), D3DVALUE(0), D3DVALUE(0) );

	m_frame->SetRotation( 0,
				D3DVAL(0.0), D3DVAL(1.0), D3DVAL(0.0),
				D3DVAL(0.0) );

	m_frame->SetVelocity(0,0,0,0,true);
	CModel* model = GetDocument()->GetModel();
	if (model == NULL)
	{
		return;
	}
	model->SetTopOrientation(m_frame, m_scene);
}

void CQMViewView::OnLockRotX()
{
	m_lock_rot_x = !m_lock_rot_x;
}

void CQMViewView::OnUpdateLockRotX(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_lock_rot_x);
}

void CQMViewView::OnLockRotY()
{
	m_lock_rot_y = !m_lock_rot_y;
}

void CQMViewView::OnUpdateLockRotY(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_lock_rot_y);
}

void CQMViewView::OnLockRotZ()
{
	m_lock_rot_z = !m_lock_rot_z;
}

void CQMViewView::OnUpdateLockRotZ(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_lock_rot_z);
}

void CQMViewView::OnLockScaleX()
{
	m_lock_scale_x = !m_lock_scale_x;
}

void CQMViewView::OnUpdateLockScaleX(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_lock_scale_x);
}

void CQMViewView::OnLockScaleY()
{
	m_lock_scale_y = !m_lock_scale_y;
}

void CQMViewView::OnUpdateLockScaleY(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_lock_scale_y);
}

void CQMViewView::OnLockScaleZ()
{
	m_lock_scale_z = !m_lock_scale_z;
}

void CQMViewView::OnUpdateLockScaleZ(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_lock_scale_z);
}

void CQMViewView::OnLockTranX()
{
	m_lock_trans_x = !m_lock_trans_x;
}

void CQMViewView::OnUpdateLockTranX(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_lock_trans_x);
}

void CQMViewView::OnLockTranY()
{
	m_lock_trans_y = !m_lock_trans_y;
}

void CQMViewView::OnUpdateLockTranY(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_lock_trans_y);
}

void CQMViewView::OnLockTranZ()
{
	m_lock_trans_z = !m_lock_trans_z;
}

void CQMViewView::OnUpdateLockTranZ(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_lock_trans_z);
}

void CQMViewView::OnRenderTexture()
{
	CModel* model = GetDocument()->GetModel();
	if (model == NULL)
	{
		return;
	}
	m_curstate = !m_curstate;
	model->RenderTexture(m_d3drm, GetDC(), m_curstate);
	GetDocument()->GetModel()->ShowFrame(m_d3drm, GetDC());
}

void CQMViewView::OnUpdateRenderTexture(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_curstate);
}

void CQMViewView::OnRenderTrans()
{
	m_transtate = !m_transtate;
	CModel* model = GetDocument()->GetModel();
	if (model == NULL)
	{
		return;
	}
	model->SetTransparency(m_d3drm, GetDC(), m_transtate);

	model->ShowFrame(m_d3drm, GetDC());
}

void CQMViewView::OnUpdateRenderTrans(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_transtate);
}

int CQMViewView::PickFace( const CPoint &point)
{
	HRESULT r;
	LPDIRECT3DRMPICKEDARRAY pickarray;

	m_viewport->Pick(point.x, point.y, &pickarray);

	int faceindex=-1;
	DWORD numpicks=pickarray->GetSize();

	if(numpicks>0)
	{
		LPDIRECT3DRMVISUAL visual;
		LPDIRECT3DRMFRAMEARRAY framearray;
		D3DRMPICKDESC pickdesc;

		r=pickarray->GetPick( 0, &visual, &framearray, &pickdesc );
		if(r==D3DRM_OK)
		{
			faceindex=pickdesc.ulFaceIdx;
			visual->Release();
			visual = NULL;
			framearray->Release();
			framearray = NULL;
		}
	}
	pickarray->Release();
	pickarray = NULL;

	return faceindex;
}

void CQMViewView::PickMesh(CPoint point)
{
	HRESULT r;
	LPDIRECT3DRMPICKEDARRAY pickarray;

	m_viewport->Pick(point.x, point.y, &pickarray);

	int faceindex=-1;
	DWORD numpicks=pickarray->GetSize();

	if(numpicks>0)
	{
		LPDIRECT3DRMVISUAL visual;
		LPDIRECT3DRMFRAMEARRAY framearray;
		D3DRMPICKDESC pickdesc;

		r=pickarray->GetPick( 0, &visual, &framearray, &pickdesc );

		if(r==D3DRM_OK)
		{
			HTREEITEM item = GetDocument()->GetModel()->SelectMesh(visual);
			if (item != NULL)
			{
				((CMainFrame*)GetParent())->PickNode(item);
			}
			visual->Release();
			visual = NULL;
			framearray->Release();
			framearray = NULL;
		}
	}
	pickarray->Release();
	pickarray = NULL;
}

void CQMViewView::DrawOnSkin(UINT nFlags, CPoint point)
{
	LPDIRECT3D2 d3d;
	HRESULT r = CDDHelper::lpDD->QueryInterface(IID_IDirect3D2, (void**)&d3d);
	if (r != D3DRM_OK)
	{
		return ;
	}
	d3d->Release();

	LPDIRECT3DRMPICKED2ARRAY pickedArray = NULL;
	DWORD flags;
	D3DRMRAY ray;

	flags = D3DRMRAYPICK_IGNOREFURTHERPRIMITIVES | D3DRMRAYPICK_INTERPOLATEUV;

	ray.dvDir.x = 0.0f;
	ray.dvDir.y = 0.0f;
	ray.dvDir.z = 1.0f;

	ray.dvPos.x = 0.0f;
	ray.dvPos.y = 0.0f;
	ray.dvPos.z = 0.0f;

	HRESULT ret = m_scene->RayPick(m_camera, &ray, flags, &pickedArray);

	if (ret != D3DRM_OK)
	{
		AfxMessageBox(CDDHelper::TraceError(ret));
		return;
	}

	if (!pickedArray)
	{
		AfxMessageBox("Hit fail\n");
		return;
	}

	DWORD numpicks=pickedArray->GetSize();
//	LPDIRECT3DRMVISUAL visual;
//	LPDIRECT3DRMFRAMEARRAY framearray;
//	D3DRMPICKDESC2 pickdesc;
//	char out[128];
//	D3DRMVECTOR4D sv;
}

void CQMViewView::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetCursor(LoadCursor(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDC_HAND_CLOSED)));
	if ((GetKeyState(VK_CONTROL)&0x8000) && (GetKeyState(VK_SHIFT)&0x8000))
	{
		PickMesh(point);
		CView::OnLButtonDown(nFlags, point);
		return;
	}
/*
	if (GetKeyState(VK_CONTROL)&0x8000)
	{
		DrawOnSkin(nFlags, point);
	}
*/
	if (!m_drag)
	{
		m_drag=true;
		m_last_x = point.x;
		m_last_y = point.y;
		SetCapture();
		//ShowCursor(FALSE);
	}

	CView::OnLButtonDown(nFlags, point);
}

void CQMViewView::OnLButtonUp(UINT nFlags, CPoint point)
{
	SetCursor(LoadCursor(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDC_HAND_OPEN)));

	if (m_drag)
	{
		m_drag = false;
		ReleaseCapture();
		double delta_x = point.x - m_last_x;
		double delta_y = point.y - m_last_y;

		if (!delta_x && !delta_y)
		{
			if (!m_has_moved && GetDocument()->GetModel())
			{
				GetDocument()->GetModel()->SetCurScale(GetDocument()->GetModel()->GetCurScale() + 5);
				m_camera->SetPosition(m_scene, D3DVALUE(0), D3DVALUE(0), D3DVALUE(GetDocument()->GetModel()->GetCurScale()));
			}
		}
		m_has_moved = FALSE;
	}

	CView::OnLButtonUp(nFlags, point);
}

void CQMViewView::OnRButtonDown(UINT nFlags, CPoint point)
{
	SetCursor(LoadCursor(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDC_HAND_CLOSED)));

	if (!m_r_drag)
	{
		m_r_drag=true;
		m_r_last_x = point.x;
		m_r_last_y = point.y;
		SetCapture();
		//ShowCursor(FALSE);
	}

	CView::OnRButtonDown(nFlags, point);
}

void CQMViewView::OnRButtonUp(UINT nFlags, CPoint point)
{
	SetCursor(LoadCursor(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDC_HAND_OPEN)));

	if (m_r_drag)
	{
		m_r_drag = false;
		ReleaseCapture();
		double delta_x = point.x - m_r_last_x;
		double delta_y = point.y - m_r_last_y;

		if (!delta_x && !delta_y)
		{
			if (!m_r_has_moved && GetDocument()->GetModel())
			{
				GetDocument()->GetModel()->SetCurScale(GetDocument()->GetModel()->GetCurScale() - 5);
				m_camera->SetPosition(m_scene, D3DVALUE(0), D3DVALUE(0), D3DVALUE(GetDocument()->GetModel()->GetCurScale()));
			}
		}
		m_r_has_moved = FALSE;
	}

	CView::OnRButtonUp(nFlags, point);
}

void CQMViewView::OnGotoposWeapon()
{
	m_frame->SetPosition(m_scene,
				D3DVALUE(0), D3DVALUE(0), D3DVALUE(-50) );

	m_frame->SetRotation( 0,
				D3DVAL(0.0), D3DVAL(1.0), D3DVAL(0.0),
				D3DVAL(0.0) );

	m_frame->SetVelocity(0,0,0,0,true);
	m_frame->SetOrientation(m_scene,
					D3DVALUE(0), D3DVALUE(0), D3DVALUE(-1),
					D3DVALUE(-1), D3DVALUE(0), D3DVALUE(0) );

	m_viewport->SetField(D3DVALUE(m_globalField));
}

void CQMViewView::OnTickerOptions()
{
	CModel* theModel = GetDocument()->GetModel();
	if (theModel == NULL)
	{
		return;
	}

	CTickerDlg tDlg;
	bool useTimer;
	theModel->GetTimerData(&tDlg.m_nTickerDelay, &useTimer);
	tDlg.m_nUseTicker = useTimer;

	if (tDlg.DoModal()==IDOK)
	{
		useTimer = (tDlg.m_nUseTicker != 0);
		theModel->SetTimerData(tDlg.m_nTickerDelay, useTimer);
	}
}

void CQMViewView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	// TODO: Add your specialized code here and/or call the base class
	CModel* theModel;
	long timerDelay;
	bool useTimer;
	switch(lHint)
	{
	case QM_IDLE:
		break;
	case QM_NEW_MODEL:
		{
		CDC *pDC;
//code stolen from the onPaint method to make sure this stuff is created before we go loading into it!
	if (m_device == NULL)
	{
		if (!CreateDevice())
		{
			PostQuitMessage(0);
		}
	}
//prolly not the best place for it, but it works for now.
		theModel = (CModel*)pHint;
		((CMainFrame*)GetParent())->SetModel(theModel);
		theModel->CreateJointVisuals(m_d3drm, m_frame);
		pDC = GetDC();
		theModel->BuildMesh(m_d3drm, m_frame, m_scene, pDC);
		ReleaseDC(pDC);
		m_camera->SetPosition(m_scene, D3DVALUE(0), D3DVALUE(0), D3DVALUE(theModel->GetCurScale()));
		CreateOriginVisual();
		theModel->SetInitialOrientation(m_frame, m_scene);
		m_transtate = false;
		theModel->GetTimerData(&timerDelay, &useTimer);
		if (useTimer)
		{
			SetTimer(1, timerDelay, NULL);
		}
		}
		break;
	case QM_UPDATE_MODEL:
		theModel = GetDocument()->GetModel();
		if (theModel != NULL)
		{
			theModel->ShowFrame(m_d3drm, GetDC());
		}
		break;
	case QM_RUN_ANIMATION:
		theModel = GetDocument()->GetModel();
		if (theModel == NULL)
		{
			break;
		}
//		theModel->GetTimerData(&timerDelay, &useTimer);
//		if (useTimer)
//		{
//			SetTimer(1, timerDelay, NULL);
//		}
		break;
	case QM_STOP_ANIMATION:
//		KillTimer(1);
		break;
	case QM_DELETE_MODEL:
		KillTimer(1);
		theModel = (CModel*)pHint;
		if (theModel != NULL)
		{
			theModel->DeleteVisuals(m_frame);
		}
		((CMainFrame*)GetParent())->SetModel(NULL);
		((CMainFrame*)GetParent())->DestroyJointDialogs();
		if (theModel != NULL)
		{
			theModel->DeleteMeshs(m_frame);
		}
		break;
	case QM_CHANGE_VISUAL:
		theModel = GetDocument()->GetModel();
		if (theModel != NULL)
		{
			theModel->ChangeVisual(m_frame, (int)pHint);
		}
		break;
	case QM_ALLNODES_VISIBLE:
		theModel = (CModel*)pHint;
		if (theModel != NULL)
		{
			theModel->MakeAllNodesVisible(m_frame);
		}
		break;
	case QM_ADD_VISUAL:
		theModel = GetDocument()->GetModel();
		if (theModel != NULL)
		{
			m_frame->AddVisual(theModel->GetMesh((int)pHint));
		}
		break;
	case QM_DELETE_VISUAL:
		theModel = GetDocument()->GetModel();
		if (theModel != NULL)
		{
			m_frame->DeleteVisual(theModel->GetMesh((int)pHint));
		}
		break;
	case QM_NEW_SKIN:
		theModel = GetDocument()->GetModel();
		theModel->LoadSkin(m_d3drm, m_frame, (int)pHint, GetDC());
		theModel->ShowFrame(m_d3drm, GetDC());
//		theModel->TextureChanged();
		break;
	case QM_CHANGE_SKIN:
		theModel = GetDocument()->GetModel();
		if (theModel != NULL)
		{
			CMainFrame* mainframe = (CMainFrame*)AfxGetApp()->m_pMainWnd;
			CNodeTreeCtrl* tree = (CNodeTreeCtrl*)mainframe->GetNodeTreeCtrl();
			if (tree == NULL)
			{
				break;
			}
			HTREEITEM item = tree->GetSelectedItem();
			if (item == NULL)
			{
				break;
			}
			int nodenum = tree->GetItemData(item);
			theModel->ChangeSkin(m_d3drm, GetDC(), nodenum, (int)pHint);
			theModel->ShowFrame(m_d3drm, GetDC());
		}
		break;
	}
}

void CQMViewView::OnTimer(UINT nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
	CModel* theModel = GetDocument()->GetModel();
	if ((theModel == NULL) || !m_d3drm_up)
	{
		return;
	}
	if (theModel->Playing())
	{
		theModel->UpdateFrame(m_d3drm, GetDC(), theModel->GetPlayMode(), m_interOn);
	}
	m_d3drm->Tick(D3DVALUE(1));
	CView::OnTimer(nIDEvent);
}

void CQMViewView::OnDestroy()
{
	m_d3drm_up = false;

	CView::OnDestroy();

	// TODO: Add your message handler code here

	CModel* model = GetDocument()->GetModel();
	if (model != NULL)
	{
		model->DeleteVisuals(m_frame);
	}
	if (m_scene)
	{
		m_scene->Release();
		m_scene=0;
	}

	if (m_device)
	{
		m_device->Release();
		m_device=0;
	}

	if (m_clipper)
	{
		m_clipper->Release();
		m_clipper=NULL;
	}

	if (m_frame)
	{
		m_frame->Release();
		m_frame = NULL;
	}

	if (m_d3drm)
	{
		m_d3drm->Release();
		m_d3drm=0;
	}

	if (CDDHelper::lpDD)
	{
		CDDHelper::lpDD->Release();
		CDDHelper::lpDD = 0;
	}
}


void CQMViewView::OnAnimateStepBack()
{
	// TODO: Add your command handler code here
	CModel* model = GetDocument()->GetModel();
	if (model == NULL)
	{
		return;
	}
	model->UpdateFrame(m_d3drm, GetDC(), ANIM_BACKWARD, m_interOn);
}

void CQMViewView::OnAnimateStepFore()
{
	// TODO: Add your command handler code here
	CModel* model = GetDocument()->GetModel();
	if (model == NULL)
	{
		return;
	}
	model->UpdateFrame(m_d3drm, GetDC(), ANIM_FORWARD, m_interOn);
}

void CQMViewView::OnAnimIntertoggle()
{
	m_interOn = !m_interOn;
}

void CQMViewView::OnUpdateAnimIntertoggle(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_interOn);
}

