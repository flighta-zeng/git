// graphic_win32.cpp : Defines the entry point for the application.
//
#include "stdafx.h"
#include "graphic_win32.h"
#include "lib_shape.hpp"

#include <stdio.h>
#include <sys/timeb.h>
#include <time.h>

#define MAX_LOADSTRING 100
//#define KEY_ENTER      13
//#define IDC_EDIT_MEMO 4001

#define MAX_LAYER_NUMBER   5

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szAppName[MAX_LOADSTRING];			    // The app name
TCHAR szAppVersion[MAX_LOADSTRING];			    // The app version No.
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

HWND   g_hWnd = 0;

struct layer *g_layer = NULL;
struct rect g_present_rect;
POINT g_previous_point;
POINT g_present_point;
POINT g_window_size;
double g_scale = 1;
int g_left_x = 0;
int g_top_y = 0;
bool g_fill = false;
bool g_endpoint = false;
bool need_show_zoom_area = false;

struct show_info g_info = {&g_left_x, &g_top_y, &g_scale};


// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);

void OnLButtonDown( HWND hWnd, WPARAM wParam, LPARAM lParam );

void OnLButtonUp( HWND hWnd, WPARAM wParam, LPARAM lParam, SIZE sizeClient );

void OnChar(HWND hWnd, WPARAM wParam, LPARAM lParam, SIZE sizeClient);

void OnMouseMove( HWND hWnd, WPARAM wParam, LPARAM lParam );

int OnMouseWheel( HWND hWnd, WPARAM wParam, LPARAM lParam );

int show_polygon(HDC hdc, point_node *head, int point_num);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings 
	LoadString(hInstance, IDS_APP_NAME, szAppName, MAX_LOADSTRING);
	LoadString(hInstance, IDS_APP_VERSION, szAppVersion, MAX_LOADSTRING);
	swprintf( szTitle, L"%s  %s", szAppName, szAppVersion);

	LoadString(hInstance, IDS_APP_NAME, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_MAINFRAME));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= NULL;
	wcex.hCursor		= NULL;
	wcex.hbrBackground	= (HBRUSH)GetStockObject(BLACK_BRUSH);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= NULL;

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable
						
	g_hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
	if (!g_hWnd)
	{
		return FALSE;
	}

	ShowWindow(g_hWnd, SW_NORMAL); /* show window as full screen */
	UpdateWindow(g_hWnd);

	return TRUE;
}

void OnCreate(HWND hWnd)
{
	int status; 
	
	char filename[] = "\0";

	/* Read file to link */
	status = f_in(filename, &g_layer);
	if (status == 0)
	{
		exit (0);
	}

	/* need know the best rectangle */
	get_rect(g_layer, &g_present_rect);
}

void OnPaint(HWND hWnd, SIZE sizeClient, bool fill, bool show_endpoint)
{
	HDC hdc;
	struct layer *layer;
	struct node *tmp_node;
	struct shape *tmp_shape;
	PAINTSTRUCT ps = {0};
	point_node *head = NULL;
	point_node *points = NULL;

	static int size = 10;

	hdc = BeginPaint(hWnd, &ps);
	
	g_window_size.x = sizeClient.cx;
	g_window_size.y = sizeClient.cy;
	calc_scale(&g_present_rect, &g_window_size, &g_scale);

	g_left_x = g_present_rect.lt.x;
	g_top_y = g_present_rect.lt.y;

	int dx;
	int dy;
	dx = abs(g_present_rect.rb.x - g_present_rect.lt.x);
	dy = abs(g_present_rect.lt.y - g_present_rect.rb.y);	

	SetMapMode (hdc, MM_ISOTROPIC);
	SetWindowExtEx (hdc, dx, dy, NULL);
	SetViewportExtEx (hdc, sizeClient.cx, -sizeClient.cy, NULL);
	//SetROP2(hdc, R2_XORPEN); 

	SetWindowOrgEx(hdc, g_present_rect.lt.x, g_present_rect.lt.y, NULL);

	if (need_show_zoom_area)
	{
		HGDIOBJ hpenOld = NULL;
	    hpenOld = SelectObject (hdc,CreatePen (PS_SOLID, 0, RGB (255, 255, 255)));
		
		POINT p1 = g_previous_point;
		POINT p2 = g_present_point;

		rect select_rect;
		select_rect.lt.x = min(p1.x, p2.x);
		select_rect.lt.y = max(p1.y, p2.y);
		select_rect.rb.x = max(p1.x, p2.x);
		select_rect.rb.y = min(p1.y, p2.y);

		MoveToEx(hdc, select_rect.lt.x, select_rect.lt.y, NULL);
		LineTo (hdc, select_rect.rb.x, select_rect.lt.y);
		LineTo (hdc, select_rect.rb.x, select_rect.rb.y);
		LineTo (hdc, select_rect.lt.x, select_rect.rb.y);
		LineTo (hdc, select_rect.lt.x, select_rect.lt.y);

		DeleteObject (SelectObject (hdc, hpenOld));
	}

	int layer_num = 0;
	layer = g_layer;
	while (layer != NULL)
	{
		layer_num++;

		if (!layer->b_visible)
		{
			layer = layer->next;
			continue;
		}

		/* set color */
		COLORREF nColor = RGB( 0, 0, 0 );
		switch (layer_num % MAX_LAYER_NUMBER)
		{
		case 1:	
			nColor = RGB (255, 0, 0);
			break;
		case 2:
			nColor = RGB (0, 0, 255);
			break;
		case 3:
			nColor = RGB (0, 255, 0);
			break;
		case 4:
			nColor = RGB (255, 255, 0);
			break;
		default:
			nColor = RGB (0, 255, 255);
			break;
		}

		/* set pen */
		HGDIOBJ hpenOld = NULL;
		hpenOld = SelectObject (hdc,CreatePen (PS_SOLID, 0, nColor));

		/* set brush */
		HBRUSH hBrush = NULL;
		hBrush = CreateSolidBrush( nColor );
		HBRUSH hOldBrush = (HBRUSH) SelectObject( hdc, hBrush );

		if (layer->contour_item != NULL)
		{
			tmp_node = layer->contour_item;
		}
		else
		{
			tmp_node = layer->item;
		}
		while(tmp_node != NULL)
		{
			tmp_shape = tmp_node->item;
			int point_num = 0;
			bool b_need_break = false;
			bool b_need_fill = false;

			b_need_fill = (fill && tmp_node->complicate);

			if (b_need_fill && tmp_node->points == NULL)
			{
				b_need_break = true;
			}
			while (tmp_shape != NULL)
			{
				if(tmp_shape->shape_type == 'L')
				{
					if (b_need_fill)
					{
						if (b_need_break)
						{
							
							if (point_num == 0)
							{	
								point_node *p_node = new point_node;
								p_node->p.x = tmp_shape->seg.ps.x;
								p_node->p.y = tmp_shape->seg.ps.y;
								p_node->next = NULL;
								points = p_node;
								head = p_node;
								point_num ++;
							}

							point_node *p_node = new point_node;
							p_node->p.x = tmp_shape->seg.pe.x;
							p_node->p.y = tmp_shape->seg.pe.y;
							p_node->next = NULL;
							points->next = p_node;
							points = p_node;
							point_num ++;
						}
					}
					else
					{
						MoveToEx(hdc, tmp_shape->seg.ps.x, tmp_shape->seg.ps.y, NULL);
						LineTo (hdc, tmp_shape->seg.pe.x, tmp_shape->seg.pe.y);
						if (show_endpoint)
						{
							Rectangle(hdc, tmp_shape->seg.pe.x-size, tmp_shape->seg.pe.y+size, 
								tmp_shape->seg.pe.x+size, tmp_shape->seg.pe.y-size);
						}
					}
				}
				else
				{
					if (b_need_fill && b_need_break)
					{
						if (point_num == 0)
						{
							point_node *pt_node = new point_node;
							pt_node->p.x = tmp_shape->cur.ps.x;
							pt_node->p.y = tmp_shape->cur.ps.y;
							pt_node->next = NULL;
							point_num ++;
							head = pt_node;
							points = pt_node;
						}
						break_arc_to_point(tmp_shape, &points, &point_num);
					}
					else
					{
						SetArcDirection(hdc, tmp_shape->cur.arc_direction);
						Arc(hdc,
							tmp_shape->cur.center.x - (int)(tmp_shape->cur.arc_radius), tmp_shape->cur.center.y + (int)(tmp_shape->cur.arc_radius),   
							tmp_shape->cur.center.x + (int)(tmp_shape->cur.arc_radius), tmp_shape->cur.center.y - (int)(tmp_shape->cur.arc_radius),
							tmp_shape->cur.ps.x, tmp_shape->cur.ps.y,   
							tmp_shape->cur.pe.x, tmp_shape->cur.pe.y);
						if (show_endpoint)
						{
							Rectangle(hdc, tmp_shape->cur.pe.x-size, tmp_shape->cur.pe.y+size, tmp_shape->cur.pe.x+size, tmp_shape->cur.pe.y-size);
						}
					}
				}
				tmp_shape = tmp_shape->next;
			}

			if (b_need_fill && b_need_break)
			{
				tmp_node->points = head;
				tmp_node->point_num = point_num;
			}

			if (b_need_fill)
			{	
				show_polygon(hdc, tmp_node->points, tmp_node->point_num);
			}
			tmp_node = tmp_node->next;
		}

		layer = layer->next;
		DeleteObject (SelectObject (hdc, hpenOld));	
	}

	EndPaint(hWnd, &ps);
}

int show_polygon(HDC hdc, point_node *head, int point_num)
{
	POINT *points = new POINT[point_num];
	point_node *pt_node;
	point_node *pre_node;

	int i = 0;
	pt_node = head;
	while (pt_node != NULL)
	{
		points[i].x = pt_node->p.x;
		points[i++].y = pt_node->p.y;

		/*
		if (i == 1)
		{
			MoveToEx(hdc, pt_node->p.x, pt_node->p.y, NULL);
		}
		else
		{
			LineTo (hdc, pt_node->p.x, pt_node->p.y);
		}
		*/

		pre_node = pt_node;
		pt_node = pt_node->next;
	}

	SetPolyFillMode(hdc, WINDING);
	Polygon(hdc, points, i);

	return (1);
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static SIZE sizeClient = {0};

	switch (message)
	{
	case WM_SIZE:
		sizeClient.cx = LOWORD (lParam);
		sizeClient.cy = HIWORD (lParam);
		break;
	case WM_CREATE:
		OnCreate(hWnd);
		break;
	case WM_LBUTTONDOWN:
		OnLButtonDown( hWnd, wParam, lParam );
		break;
	case WM_LBUTTONUP:
		OnLButtonUp( hWnd, wParam, lParam, sizeClient );
		break;
	case WM_PAINT:
		OnPaint(hWnd, sizeClient, g_fill, g_endpoint);
		break;
	case WM_CHAR:
		OnChar(hWnd, wParam, lParam, sizeClient);
		break;
	case 0x020a/*WM_MOUSEHWHEEL*/:
		OnMouseWheel(hWnd, wParam, lParam);
		break;
	case WM_MOUSEMOVE:
		OnMouseMove( hWnd, wParam, lParam );
		break;	
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

int OnMouseWheel( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
#define MIN_WHEEL_DELAY 1.0

	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	int xPos = LOWORD(lParam); /* Pos get from MouseWheel is different with ButtonUp, it is from screen, ButtonUp is current rect */
    int yPos = HIWORD(lParam);

	static struct _timeb current_time;
	static struct _timeb last_time;
	static int first_time = 1;
	double diff;
	double scale = 0.7;

	// ignore if cursor not in window
	RECT wr;
	GetWindowRect( hWnd, &wr );
	if( xPos < wr.left+5 || xPos > wr.right-5 || yPos < wr.top+25 || yPos > wr.bottom-5 )
	{
		return(0);
	}

	POINT cur_pt;
	POINT new_pt;
	cur_pt.x = g_present_point.x;
	cur_pt.y = g_present_point.y;
	new_pt.x = g_present_point.x;
	new_pt.y = g_present_point.y;

	// get current time
	_ftime( &current_time );

	if( first_time )
	{
		diff = 999.0;
		first_time = 0;
	}
	else
	{
		// get elapsed time since last wheel event
		diff = difftime( current_time.time, last_time.time );
		double diff_mil = (double)(current_time.millitm - last_time.millitm)*0.001;
		diff = diff + diff_mil;
	}

	g_left_x = g_present_rect.lt.x;
	g_top_y = g_present_rect.lt.y;
	if( diff > MIN_WHEEL_DELAY )
	{
		// first wheel movement in a while
		// center window on cursor then center cursor
		//POINT p;
		//GetCursorPos( &p );		// cursor pos in screen coords

		//InvalidateRect( hWnd, NULL, TRUE );
	}
	else
	{
		// serial movements, zoom in or out
		if( zDelta > 0 )
		{
			DPtoLP(&cur_pt, &g_info);

			g_present_rect.lt.x = (int)(scale * g_present_rect.lt.x);
			g_present_rect.lt.y = (int)(scale * g_present_rect.lt.y);
			g_present_rect.rb.x = (int)(scale * g_present_rect.rb.x);
			g_present_rect.rb.y = (int)(scale * g_present_rect.rb.y);

			calc_scale(&g_present_rect, &g_window_size, &g_scale);

			g_left_x = g_present_rect.lt.x;
			g_top_y = g_present_rect.lt.y;
			DPtoLP(&new_pt, &g_info);		
			int delta_x;
			int delta_y;
			delta_x = new_pt.x - cur_pt.x;
			delta_y = new_pt.y - cur_pt.y;
			g_present_rect.lt.x = g_present_rect.lt.x - delta_x;
			g_present_rect.rb.x = g_present_rect.rb.x - delta_x;
			g_present_rect.lt.y = g_present_rect.lt.y - delta_y;
			g_present_rect.rb.y = g_present_rect.rb.y - delta_y;
		}
		else if( zDelta < 0 )
		{
			DPtoLP(&cur_pt, &g_info);

			g_present_rect.lt.x = (int)((2 - scale) * g_present_rect.lt.x);
			g_present_rect.lt.y = (int)((2 - scale) * g_present_rect.lt.y);
			g_present_rect.rb.x = (int)((2 - scale) * g_present_rect.rb.x);
			g_present_rect.rb.y = (int)((2 - scale) * g_present_rect.rb.y);

			calc_scale(&g_present_rect, &g_window_size, &g_scale);

			g_left_x = g_present_rect.lt.x;
			g_top_y = g_present_rect.lt.y;
			DPtoLP(&new_pt, &g_info);		
			int delta_x;
			int delta_y;
			delta_x = new_pt.x - cur_pt.x;
			delta_y = new_pt.y - cur_pt.y;
			g_present_rect.lt.x = g_present_rect.lt.x - delta_x;
			g_present_rect.rb.x = g_present_rect.rb.x - delta_x;
			g_present_rect.lt.y = g_present_rect.lt.y - delta_y;
			g_present_rect.rb.y = g_present_rect.rb.y - delta_y;
		}
		InvalidateRect( hWnd, NULL, TRUE );
	}
	last_time = current_time;
	return (0);
}

void OnMouseMove( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
	g_present_point.x = LOWORD(lParam);
	g_present_point.y = HIWORD(lParam);
	if (wParam & MK_LBUTTON)
	{
		DPtoLP(&g_present_point, &g_info);
	}
}

void OnChar(HWND hWnd, WPARAM wParam, LPARAM lParam, SIZE sizeClient)
{
	int repeat_count;
	repeat_count = LOWORD(lParam);
	if (repeat_count == 1)
	{
		POINT lt;
		lt.x = 0;
		lt.y = 0;
		POINT rb;
		rb.x = (int)(sizeClient.cx);
		rb.y = (int)(sizeClient.cy);
		switch (wParam)
		{
		case 'r':
		case 'R':
			{
				int status; 
				char filename[] = "\0";

				/* Read file to link */
				status = f_in(filename, &g_layer);
				if (status == 0)
				{
					exit (0);
				}
				get_rect(g_layer, &g_present_rect);
				InvalidateRect( hWnd, NULL, TRUE );
				break;
			}
		case 'g':
		case 'G':
			get_rect(g_layer, &g_present_rect);
			InvalidateRect( hWnd, NULL, TRUE );
			break;
		case 's':
		case 'S':
			lt.x -= (rb.x - lt.x)/4;
			rb.x -= (rb.x - lt.x)/4;
			break;
		case 'e':
		case 'E':
			lt.y -= (rb.y - lt.y)/4;
			rb.y -= (rb.y - lt.y)/4;
			break;
		case 'f':
		case 'F':
			lt.x += (rb.x - lt.x)/4;
			rb.x += (rb.x - lt.x)/4;
			break;
		case 'd':
		case 'D':
			lt.y += (rb.y - lt.y)/4;
			rb.y += (rb.y - lt.y)/4;
			break;
		case 'w':
		case 'W':
			if (!g_fill && !g_endpoint)
			{
				g_fill = !g_fill;
			}
			else
			{
				if (g_fill && !g_endpoint)
				{
					g_fill = !g_fill;
					g_endpoint = !g_endpoint;
				}
				else
				{
					g_fill = false;
					g_endpoint = false;
				}
			}
			break;
		case 'x':
		case 'X':
			PostQuitMessage(0);
			break;
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
			{
				int i;
				i = wParam - '1';

				int j = 0;
				layer *tmp_layer = g_layer;
				while (tmp_layer != NULL)
				{
					if (j++ == i)
					{
						tmp_layer->b_visible = !tmp_layer->b_visible;
					}
					tmp_layer = tmp_layer->next;
				}
			}
			break;
		default:
			break;
		}

		switch (wParam)
		{
		case 's':
		case 'S':
		case 'd':
		case 'D':
		case 'f':
		case 'F':
		case 'e':
		case 'E':
		case 'w':
		case 'W':
			{
				DPtoLP(&lt, &g_info);
				DPtoLP(&rb, &g_info);
				g_present_rect.lt.x = lt.x;
				g_present_rect.lt.y = lt.y;
				g_present_rect.rb.x = rb.x;
				g_present_rect.rb.y = rb.y;
				InvalidateRect( hWnd, NULL, TRUE );
			}
			break;
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case 'c':
		case 'C':
			InvalidateRect( hWnd, NULL, TRUE );
			break;
		default:
			break;
		}
	}

	if (need_show_zoom_area && (wParam == 'a' || wParam == 'A'))
	{
		InvalidateRect( hWnd, NULL, TRUE );
	}
}

void OnLButtonDown( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
	POINT p;
	p.x = LOWORD(lParam);
	p.y = HIWORD(lParam);

	DPtoLP(&p, &g_info);

	need_show_zoom_area = true;
	g_previous_point = p;
}


void OnLButtonUp( HWND hWnd, WPARAM wParam, LPARAM lParam, SIZE sizeClient )
{
	POINT p;
	p.x = LOWORD(lParam);
	p.y = HIWORD(lParam);

	DPtoLP(&p, &g_info);
	
	/* get the move value in device unit */
	int dx;
	int dy;
	dx = (int)((p.x - g_previous_point.x) /  g_scale);
	dx = abs(dx);
	dy = (int)((p.y - g_previous_point.y) / g_scale);
	dy = abs(dy);

	int min_mouse_move  = 5;
	if (need_show_zoom_area)
	{
		if(dx > min_mouse_move || dy > min_mouse_move)
		{
			/* create new draw area */
			g_present_rect.lt.x = min(g_previous_point.x, p.x);
			g_present_rect.lt.y = max(g_previous_point.y, p.y);
			g_present_rect.rb.x = max(g_previous_point.x, p.x);
			g_present_rect.rb.y = min(g_previous_point.y, p.y);
		}

		need_show_zoom_area = false;
		InvalidateRect( hWnd, NULL, TRUE );
	}
}

