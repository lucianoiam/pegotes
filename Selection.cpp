#include <windows.h>
#include <shlobj.h>
#include "Selection.h"
#include "Clip.h"
#include "Config.h"
#include "Util.h"
#include "Capture.h"
#include "Settings.h"

HWND CreateSelWin (HWND);
static void PaintSelection (HWND, const RECT *);
static void GetRectFromMouseCoord (RECT *, POINT *, POINT *);

bool	bSelecting;
RECT	selRect, lastSelRect, updateRect;
POINT	startpnt;
HWND	shWnd;

extern int ScrWidth, ScrHeight;

// CreateSelWin
// Create selection window
HWND CreateSelWin (HWND phWnd)
{
	HWND	hwnd;

	hwnd = CreateWindowEx (WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
							WIN_CLASS_SEL, WIN_NAME_SEL, WS_POPUP | WS_MAXIMIZE,
							CW_USEDEFAULT, CW_USEDEFAULT, ScrWidth, ScrHeight,
							phWnd, NULL, GetModuleHandle (NULL), NULL);

	if (hwnd == NULL)
		return NULL;

	m_pSetLayeredWindowAttributes (hwnd, 0, 32, LWA_ALPHA);
	RegisterHotKey (hwnd, 0x02, 0, VK_ESCAPE);
	ShowWindow (hwnd, SW_SHOWNOACTIVATE);

	return hwnd;
}

// LRESULT CALLBACK SelWinProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
// Selection window message handling
LRESULT CALLBACK SelWinProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int			i, n;
	POINT		endpnt;
	CClipWindow	*cw;
	Bitmap		*bitmap;

	switch (msg)
	{
		case WM_HOTKEY:
		case WM_RBUTTONUP:
			PostMessage (hwnd, WM_CLOSE, 0, 0);
			break;
		case WM_DISPLAYCHANGE:					// Maximize on screen change
			ShowWindow (hwnd, SW_MAXIMIZE);
			break;
		case WM_LBUTTONDOWN:					// Start selecting
			bSelecting = TRUE;
			startpnt.x = LOWORD (lParam);
			startpnt.y = HIWORD (lParam);
			break;
		case WM_MOUSEMOVE:						// Selecting
			if (bSelecting)
			{
				endpnt.x = LOWORD(lParam);
				endpnt.y = HIWORD(lParam);
				GetRectFromMouseCoord (&selRect, &startpnt, &endpnt);
				UnionRect (&updateRect, &lastSelRect, &selRect);
				memcpy (&lastSelRect, &selRect, sizeof(RECT));
				InvalidateRect (hwnd, &updateRect, true);	// Redraw
			}
			break;
		case WM_LBUTTONUP:						// End selecting
			bSelecting = FALSE;
			if (((selRect.right-selRect.left) < CLIP_MIN_WIDTH)
				|| ((selRect.bottom-selRect.top) < CLIP_MIN_HEIGHT))
			{
				ZeroMemory (&selRect, sizeof (RECT));
				InvalidateRect (hwnd, NULL, true);
				break;
			}

			if (!KEY_DOWN(VK_SHIFT))
			{
				SaveRectangle (&n, &selRect, &bitmap);
				cw = new CClipWindow (selRect.left-1, selRect.top-1, n,
								bitmap, Settings.Language, true, GetParent(hwnd));
				for (i = 32; i > 1; i -= 8)
					m_pSetLayeredWindowAttributes (hwnd, 0, i, LWA_ALPHA);
			}
			else
			{
				SaveRectangle (NULL, &selRect, &bitmap);
				delete bitmap;
			};

			ZeroMemory (&selRect, sizeof (RECT));
			PostMessage (hwnd, WM_CLOSE, 0, 0);
			break;
		case WM_PAINT:							// Redraw
			PaintSelection (hwnd, &selRect);
			break;
		case WM_CLOSE:
			UnregisterHotKey (hwnd, 0x02);
			DestroyWindow (hwnd);
			shWnd = NULL;
			break;
		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	return 0;
}

// static void PaintSelection (HWND hWnd, const RECT *selRect)
// Paint selection rectangle
static void PaintSelection (HWND hWnd, const RECT *selRect)
{
	int			w, h;
	PAINTSTRUCT	ps;
	HDC			hdcMem;
	HBITMAP		hbmMem;
	HRGN		region;

	w = selRect->right - selRect->left;
	h = selRect->bottom - selRect->top;

	ps.hdc = GetDC (hWnd);
    hdcMem = CreateCompatibleDC (ps.hdc);			// Create mem DC
    hbmMem = CreateCompatibleBitmap (ps.hdc, w, h);	// Create mem bitmap
    SelectObject (hdcMem, hbmMem);

	BeginPaint (hWnd, &ps);

	region = CreateRectRgn (0, 0,					// Paint mem DC
		selRect->right-selRect->left, selRect->bottom-selRect->top);
	FillRgn (hdcMem, region, (HBRUSH)GetStockObject(WHITE_BRUSH));
	FrameRgn (hdcMem, region, (HBRUSH)GetStockObject(GRAY_BRUSH), 1, 1);

	BitBlt (ps.hdc, selRect->left, selRect->top,	// Blit mem DC to screen DC
			w, h, hdcMem, 0, 0, SRCCOPY);

	EndPaint (hWnd, &ps);

	DeleteObject (region);

	ReleaseDC (hWnd, ps.hdc);
	DeleteDC (hdcMem);
	DeleteObject (hbmMem);
}

// static void GetRectFromMouseCoord (RECT *rect, POINT *start, POINT *end)
// Converts two mouse points into a rectangle
static void GetRectFromMouseCoord (RECT *rect, POINT *start, POINT *end)
{
	if (start->x < end->x)
	{
		rect->left = start->x;
		rect->right = end->x;
	}
	else
	{
		rect->left = end->x;
		rect->right = start->x;
	}

	if (start->y < end->y)
	{
		rect->top = start->y;
		rect->bottom = end->y;
	}
	else
	{
		rect->top = end->y;
		rect->bottom = start->y;
	}
}
