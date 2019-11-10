#include <windows.h>
#include <ole2.h>
#include <shlobj.h>
#include <string.h>
#include "Clip.h"
#include "Config.h"
#include "DropSource.h"
#include "Util.h"
#include "Settings.h"
#include "TrayIcon.h"
#include "Capture.h"
#include "resource.h"
#include "Locale.h"

LRESULT CALLBACK ClipWinProc (HWND, UINT, WPARAM, LPARAM);

extern Settings_t Settings;
extern int ScrWidth, ScrHeight;

// CClipWindow::CClipWindow (int x, int y, int n, Bitmap *bm, int lang, bool show, HWND phWnd)
// Creates clip window
CClipWindow::CClipWindow (int x, int y, int n, Bitmap *bm, int lang, bool show, HWND phWnd)
{
	int		i, j;
	ULONG	avg = 0;
	Color	*px = new Color ();
	wchar_t	regfolder[255];
	
	// Init info
	m_id = n;
	m_GpBitmap = bm;
	m_Locked = 1;
	m_OnTop = 1;
	m_Hidden = 0;
	m_Menu = NULL;
	m_Shadow = NULL;
	m_HiQuality = true;
	m_Name[0] = '\0';
	ResetSize ();

	this->m_x = x;
	this->m_y = y;

	// Get image brightness at size handle area
	for (j = m_h-CLIP_HANDLE_SIZE; j < m_h; j++)
	{
		for (i = m_w-CLIP_HANDLE_SIZE; i < m_w; i++)
		{
			m_GpBitmap->GetPixel (i, j, px);
			avg += (px->GetR () + px->GetG() + px->GetB()) / 3;
		}
	}

	avg /= CLIP_HANDLE_SIZE*CLIP_HANDLE_SIZE;
	if (avg > BRIGHTNESS_THRESHOLD)
		m_GpPenBorder = new Pen (Color (0xFF202020));
	else
		m_GpPenBorder = new Pen (Color (0xFFE0E0E0));
	m_GpPenHandle = new Pen (Color (0xFF808080));

	// Create clip window
	m_hWnd = CreateWindowEx (WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
							WIN_CLASS_CLIP, WIN_NAME_CLIP, WS_POPUP,
							x, y, m_w, m_h, phWnd, NULL, GetModuleHandle (NULL), NULL );
	if (m_hWnd == NULL)
		return;
	if (m_id < CLIP_ID_STATIC)
	{
		m_texthWnd = CreateWindowEx (0, L"EDIT", NULL, WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
									0, m_h-CLIP_TEXT_HEIGHT, m_w, CLIP_TEXT_HEIGHT,
									m_hWnd, (HMENU)IDC_NAME_EDIT, GetModuleHandle (NULL), 0);
		if (m_texthWnd == NULL)
			return;		// TODO: return generates access violation

	    HFONT hfDefault = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
		SendMessage (m_texthWnd, WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
		SendMessage (m_texthWnd, EM_LIMITTEXT, MAX_CLIP_TEXT, 0);
	}

	SetWindowLongPtr (m_hWnd, GWLP_USERDATA, (long)this);		// Embed &this in window

	m_Menu = CreatePopupMenu ();
	if (m_Menu == NULL)
		return;
	SetLanguage (lang);

	if (m_id < CLIP_ID_STATIC)
	{
		wsprintf (m_Filename, L"%s%04d", DEFAULT_CLIP_FILENAME, m_id);
		wsprintf (regfolder, L"Software\\%s\\%s\\%04d", VENDOR_NAME, APP_NAME, m_id);
		RegCreateKeyEx (HKEY_CURRENT_USER, regfolder, 0, NULL,
			REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &m_Key, NULL);
		m_Locked = 0;
	}

	m_Shadow = new CWndShadow ();
	if (m_Shadow != NULL)
	{
		m_Shadow->Create (m_hWnd);
		m_Shadow->SetSharpness (8);
		m_Shadow->SetSize (2);
	}

	if (show)
		Show (1);
}

// CClipWindow::~CClipWindow ()
// Object destructor
CClipWindow::~CClipWindow ()
{
	DestroyMenu (m_Menu);
	DestroyWindow (m_hWnd);
	delete m_Shadow;
	delete m_GpPenBorder;
	delete m_GpPenHandle;
	delete m_GpBitmap;
	if (m_Key != NULL)
		RegCloseKey (m_Key);
}

// float CClipWindow::GetRatio (void)
// Returns aspect ractio
float CClipWindow::GetRatio (void)
{
	return m_r;
}

// int CClipWindow::GetID (void)
// Returns ID
int CClipWindow::GetID (void)
{
	return m_id;
}

// BOOL CClipWindow::IsHidden (void)
// Returns hidden status
BOOL CClipWindow::IsHidden (void)
{
	BOOL hidden;

	hidden = (BOOL)m_Hidden;

	return hidden;
}

// void CClipWindow::SetSize (int w, int h)
// Sets size
void CClipWindow::SetSize (int w, int h)
{
	this->m_w = w;
	this->m_h = h;
}

// void CClipWindow::ResetSize (void)
// Resets size to original bitmap size
void CClipWindow::ResetSize (void)
{
	SetSize (m_GpBitmap->GetWidth (), m_GpBitmap->GetHeight ());
	SetWindowPos (m_hWnd, HWND_TOP, 0, 0, m_GpBitmap->GetWidth (),
					m_GpBitmap->GetHeight (), SWP_NOMOVE);
	UpdateRatio ();

	Freeze ();
}

// void CClipWindow::UpdateRatio (void)
// Update aspect ratio
void CClipWindow::UpdateRatio (void)
{
	m_r = (float)m_h / (float)m_w;
}

// void CClipWindow::SetPosition (int x, int y)
// Sets position
void CClipWindow::SetPosition (int x, int y)
{
	this->m_x = x;
	this->m_y = y;
}

// CClipWindow::SetOnTop (int s)
// Set on top mode
void CClipWindow::SetOnTop (int s)
{
	MENUITEMINFO	item;
	HWND			wState;

	if (s < 2)
		m_OnTop = s;
	else
		m_OnTop = !m_OnTop;

	item.cbSize = sizeof (item);
	item.fMask = MIIM_STATE;

	if (m_OnTop)
	{
		item.fState = MFS_CHECKED;
		wState = HWND_TOPMOST;
	}
	else
	{
		item.fState = MFS_UNCHECKED;
		wState = HWND_NOTOPMOST;
	}

	SetMenuItemInfo (m_Menu, IDM_ON_TOP, FALSE, &item);		// Update popup menu
	SetWindowPos (m_hWnd, wState, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);

	Freeze ();
}

// void CClipWindow::Show (int b)
// Show or hides clip
void CClipWindow::Show (int b)
{
	SetWindowPos (m_hWnd, HWND_TOP, m_x, m_y, m_w, m_h, 0);

	m_Hidden = !b;

	if (m_Hidden)
	{
		if (m_id < CLIP_ID_STATIC)
		{
			ShowNameBox (false);
			ShowWindow (m_hWnd, SW_HIDE);
		}
	}
	else
	{
		ShowWindow (m_hWnd, SW_SHOW);
		m_Shadow->SetDarkness (125);
		m_pSetLayeredWindowAttributes (m_hWnd, 0, 224, LWA_ALPHA);
		Draw ();
	}

	Freeze ();
}

// CClipWindow::Draw (void)
// Redraw window
void CClipWindow::Draw (void)
{
	Graphics	*gdiplus;
	PAINTSTRUCT	ps;
	HDC			DC;

	InvalidateRect (m_hWnd, NULL, TRUE);
	
	DC = BeginPaint (m_hWnd, &ps);
	gdiplus = new Graphics (DC);			// Create GDI+ graphic object

	if (m_HiQuality)
		gdiplus->SetInterpolationMode (InterpolationModeHighQualityBicubic);
	else
		gdiplus->SetInterpolationMode (InterpolationModeNearestNeighbor);

	// Copy bitmap from clip instance to window
	gdiplus->DrawImage (m_GpBitmap, Rect (0, 0, m_w, m_h));

	// Draw border
	gdiplus->DrawRectangle (m_GpPenBorder, 0, 0, m_w-1, m_h-1);

	// Draw resize handle
	if (!IsWindowVisible (m_texthWnd))
		for (int i = 1; i < 4; i++)
			gdiplus->DrawLine (m_GpPenHandle, m_w-4*i, m_h-3,
								m_w-3, m_h-4*i);

	EndPaint (m_hWnd, &ps);
}

// void CClipWindow::Menu (void)
// Shows menu
void CClipWindow::Menu (void)
{
	POINT point;

	GetCursorPos (&point);
	TrackPopupMenuEx (m_Menu, TPM_CENTERALIGN | TPM_HORIZONTAL | TPM_RIGHTBUTTON,
						point.x, point.y, m_hWnd, NULL);
}

// void CClipWindow::FadeOut (void)
// Fadeout effect
void CClipWindow::FadeOut (void)
{
	int i;

	for (i = 224; i > 1; i -= 16)
	{
		m_pSetLayeredWindowAttributes (m_hWnd, 0, i, LWA_ALPHA);
		m_Shadow->SetDarkness (i-100);
	}
}

// int CClipWindow::Save (LPCWSTR fn)
// Saves bitmap
int CClipWindow::Save (LPCWSTR fn)
{
	int				ret = 0;
	CLSID			encClsid;
	Bitmap			*clip;
	OPENFILENAME	ofn;
	wchar_t			swFileName[MAX_PATH];
	wchar_t			swFilter[32] = L"";

	GetEncoderClsid	(MIMETypes[Settings.Encoder], &encClsid);

	if (fn == NULL)
	{
		ZeroMemory(&ofn, sizeof(ofn));
		wcscpy (swFileName, DEFAULT_CLIP_FILENAME);

		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = m_hWnd;
		ofn.lpstrFilter = SAVE_FILE_TYPES;
		ofn.nFilterIndex = 1+Settings.Encoder;
		ofn.lpstrFile = swFileName;
		ofn.nMaxFile = MAX_PATH;
		ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT;
		ofn.lpstrDefExt = Extensions[Settings.Encoder]+1;
		ofn.lpstrTitle = StrTable [IDS_SAVE_CLIP];
		ofn.lpstrCustomFilter = swFilter;
		ofn.nMaxCustFilter = 32;

		if (!GetSaveFileName (&ofn))					// User canceled
			return -1;

		for (int i = 0; i < MAX_ENCODERS; i++)
			if (wcscmp (2+swFilter, Extensions[i]) == 0)
			{
				GetEncoderClsid	(MIMETypes[i], &encClsid);
				break;
			}
		fn = swFileName;
	}

	//clip = GpBitmap->StretchClone (w, h);
	clip = new Bitmap (m_w, m_h);						// Create GDI+ bitmap
	Graphics graphics (clip);							// Create GDI+ graph. from bitmap
	graphics.SetInterpolationMode (InterpolationModeHighQualityBicubic);
	graphics.DrawImage (m_GpBitmap, 0, 0, m_w, m_h);	// Copy instance bitmap to GDI+
 
	if (clip->Save (fn, &encClsid, &encParms) != Ok)
		ret = -1;
	delete clip;

	return ret;
}

// void CClipWindow::Freeze (void)
// Stores parameters in registry
void CClipWindow::Freeze (void)
{
	int	data;

	if (m_Locked)
		return;

	if (m_x >= 0x7FFF)
		data = 0;
	else
		data = m_x;
	RegSetValueEx (m_Key, KEY_X, 0, REG_DWORD, (unsigned char*)&data, sizeof(DWORD));
	if (m_y >= 0x7FFF)
		data = 0;
	else
		data = m_y;
	RegSetValueEx (m_Key, KEY_Y, 0, REG_DWORD, (unsigned char*)&data, sizeof(DWORD));
	RegSetValueEx (m_Key, KEY_WIDTH, 0, REG_DWORD, (unsigned char*)&m_w, sizeof(DWORD));
	RegSetValueEx (m_Key, KEY_HEIGHT, 0, REG_DWORD, (unsigned char*)&m_h, sizeof(DWORD));
	RegSetValueEx (m_Key, KEY_ON_TOP, 0, REG_DWORD, (unsigned char*)&m_OnTop, sizeof(DWORD));
	RegSetValueEx (m_Key, KEY_HIDDEN, 0, REG_DWORD, (unsigned char*)&m_Hidden, sizeof(DWORD));
	RegSetValueEx (m_Key, KEY_NAME, 0, REG_SZ, (unsigned char*)m_Name,
							sizeof(wchar_t)*(1+wcslen(m_Name)));
}

// int CClipWindow::DragAndDrop (int action)
// Initiates a drag & drop action
int CClipWindow::DragAndDrop (int action)
{
#ifndef _DEBUG
	HRESULT		hr;
	DWORD		dwEffect;
	IDataObject	*pdto;
	wchar_t		path[MAX_PATH];

	if (m_id >= CLIP_ID_STATIC)
		return -1;

	// save bitmap to temp folder
	GetTempPath (MAX_PATH, path);
	wcscat (path, APP_NAME L"\\");
	if (GetFileAttributes (path) == INVALID_FILE_ATTRIBUTES)
		CreateDirectory (path, NULL);
	wcscat (path, m_Filename);
	wcscat (path, Extensions[Settings.Encoder]);
	Save (path);

	hr = GetUIObjectOfFile (NULL, path, IID_IDataObject, (void**)&pdto);

	if (SUCCEEDED(hr))
	{
		IDropSource *pds = new CDropSource();
		if (pds)
		{
			hr = DoDragDrop (pdto, pds, DROPEFFECT_MOVE|DROPEFFECT_COPY, &dwEffect);
			pds->Release();
		}
		pdto->Release();

		if (action == 1)
		{
			RemoveSavedData ();
			delete this;
		}
	}
#endif
	return 0;
}

// void CClipWindow::RemoveSavedData (void)
// Removes parameters from registry
void CClipWindow::RemoveSavedData (void)
{
#ifndef _DEBUG
	HKEY	pkey;
	wchar_t regfolder[255];
	wchar_t	clipfolder[16];
	wchar_t	path[MAX_PATH];

	RegCloseKey (m_Key);
	m_Key = NULL;

	wsprintf (regfolder, L"Software\\%s\\%s", VENDOR_NAME, APP_NAME);
	wsprintf (clipfolder, L"%04d", m_id);

	RegOpenKeyEx (HKEY_CURRENT_USER, regfolder, 0, KEY_WRITE, &pkey);
	RegDeleteKey (pkey, clipfolder);
	RegCloseKey (pkey);
	
	SHGetSpecialFolderPath (NULL, path, CSIDL_LOCAL_APPDATA, FALSE);

	wcscat (path, L"\\" VENDOR_NAME L"\\" APP_NAME L"\\" IMAGES_FOLDER L"\\");
	wcscat (path, m_Filename);
	wcscat (path, Extensions[0]);

	DeleteFile (path);
#endif
}

// void CClipWindow::Lock (int b)
// Prevents registry modifications
void CClipWindow::Lock (int b)
{
	m_Locked = b;
}

// void CClipWindow::SetLanguage (int lang)
// Create options menu
void CClipWindow::SetLanguage (int lang)
{
	MENUITEMINFO	item;

	int j = GetMenuItemCount (m_Menu);
	for (int i = 0; i < j; i++)
		DeleteMenu (m_Menu, 0, MF_BYPOSITION);

	item.cbSize = sizeof (item);
	// Close option
	item.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
	item.fType = MFT_STRING;
	item.wID = IDM_HIDE;
	item.dwTypeData = StrTable [IDS_HIDE];
	item.cch = wcslen (item.dwTypeData);
	InsertMenuItem (m_Menu, IDM_HIDE, FALSE, &item);
	// Hide option
	item.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
	item.fType = MFT_STRING;
	item.wID = IDM_DELETE;
	item.dwTypeData = StrTable [IDS_DELETE];
	item.cch = wcslen (item.dwTypeData);
	InsertMenuItem (m_Menu, IDM_DELETE, FALSE, &item);
	// Save option
	item.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
	item.fType = MFT_STRING;
	item.wID = IDM_SAVE;
	item.dwTypeData = StrTable [IDS_SAVE];
	item.cch = wcslen (item.dwTypeData);
	InsertMenuItem (m_Menu, IDM_SAVE, FALSE, &item);
	// Separator
	item.fMask = MIIM_FTYPE | MIIM_ID;
	item.fType = MFT_SEPARATOR;
	InsertMenuItem (m_Menu, IDM_SEPARATOR, FALSE, &item);
	// On top option
	item.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID | MIIM_STATE;
	item.fType = MFT_STRING;
	item.wID = IDM_ON_TOP;
	item.dwTypeData = StrTable [IDS_ON_TOP];
	item.cch = wcslen (item.dwTypeData);
	if (m_OnTop)
		item.fState = MFS_CHECKED;
	else
		item.fState = MFS_UNCHECKED;
	InsertMenuItem (m_Menu, IDM_ON_TOP, FALSE, &item);

	if (m_id >= CLIP_ID_STATIC)
	{
		item.fMask = MIIM_STATE;
		item.fState = MFS_DISABLED;
		SetMenuItemInfo (m_Menu, IDM_HIDE, FALSE, &item);
		SetMenuItemInfo (m_Menu, IDM_SAVE, FALSE, &item);
		item.fState = MFS_DISABLED | MFS_CHECKED;
		SetMenuItemInfo (m_Menu, IDM_ON_TOP, FALSE, &item);
	}
}

// void CClipWindow::Shadow (bool b)
// Show or hide shadow
void CClipWindow::Shadow (bool b)
{
	if (b)
		ShowWindow (m_Shadow->m_hWnd, SW_SHOWNA);
	else
		ShowWindow (m_Shadow->m_hWnd, SW_HIDE);
}

// void CClipWindow::Quality (bool b)
// Control bitmap rendering quality
void CClipWindow::Quality (bool b)
{
	m_HiQuality = b;
}

// void CClipWindow::ShowNameBox (bool show)
// Show name input text box
void CClipWindow::ShowNameBox (bool show)
{
	if (show)
	{
		SetWindowPos (m_texthWnd, HWND_TOP, 0, m_h-CLIP_TEXT_HEIGHT,
							m_w, CLIP_TEXT_HEIGHT, 0);
		ShowWindow (m_texthWnd, SW_SHOW);
		
		if (wcslen (m_Name) == 0)
		{
			SetWindowText (m_texthWnd, StrTable[IDS_ENTER_NAME]);
			m_TextChanged = false;
		}
		else
		{
			SetWindowText (m_texthWnd, m_Name);
			m_TextChanged = true;
		}

		SendMessage (m_texthWnd, EM_SETSEL, 0, -1);
		SetFocus (m_texthWnd);
	}
	else
		ShowWindow (m_texthWnd, SW_HIDE);

	Draw ();
}

// BOOL CClipWindow::IsNameBoxVisible (void)
// Returns name box visibility status
BOOL CClipWindow::IsNameBoxVisible (void)
{
	return IsWindowVisible (m_texthWnd);
}

// ClipWinProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
// Clip window message handling
LRESULT CALLBACK ClipWinProc (HWND hwnd, UINT msg,
							 WPARAM wParam, LPARAM lParam)
{
	CClipWindow		*cw;
	WINDOWPLACEMENT	wPos;
	MINMAXINFO		*wMinMax;
	RECT			*rect;
	LRESULT			ret;
	int				h;

	// Get window CClipWindow instance
	cw = (CClipWindow*)GetWindowLongPtr (hwnd, GWLP_USERDATA);
	if (cw == NULL)
		return 1;

	switch (msg)
	{
		case WM_NCLBUTTONDOWN:				// Drag & drop
			GetWindowPlacement (hwnd, &wPos);
			if (!((LOWORD(lParam) > wPos.rcNormalPosition.right - CLIP_HANDLE_SIZE) &&
					(HIWORD(lParam) > wPos.rcNormalPosition.bottom - CLIP_HANDLE_SIZE)))
			{
				if (KEY_DOWN(VK_SHIFT))
					cw->DragAndDrop (1);		// Move
				if (KEY_DOWN(VK_CONTROL))
					cw->DragAndDrop (2);		// Copy
			}

			return DefWindowProc (hwnd, msg, wParam, lParam);
		case WM_NCRBUTTONUP:				// Show popup menu
			cw->Menu ();
			break;
		case WM_CHAR:						// Key pressed
			if (cw->IsNameBoxVisible ())
			{
				if (wParam == 0x0D)			// Enter
				{
					if (cw->m_TextChanged)
					{
						GetWindowText ((HWND)lParam, cw->m_Name, MAX_CLIP_TEXT);
						cw->Show(0);
					}
				}
				else if (wParam == 0x1B)	// Escape
					cw->ShowNameBox (false);
				else						// Other key
					cw->m_TextChanged = true;
			}
			break;
		case WM_COMMAND:					// Process menu option
			switch (LOWORD (wParam))
			{
				case IDM_HIDE:
					if ((wcslen (cw->m_Name) == 0) || KEY_DOWN(VK_SHIFT))
						cw->ShowNameBox (true);
					else
						cw->Show (0);
					break;
				case IDM_DELETE:
					cw->FadeOut ();
					cw->RemoveSavedData ();
					delete cw;
					break;
				case IDM_SAVE:
					cw->Save (NULL);
					break;
				case IDM_ON_TOP:
					cw->SetOnTop (2);
					break;
				default:
					break;
			}
			break;
		case WM_SIZING:						// Proportional resizing
			if (!KEY_DOWN(VK_SHIFT))
			{
				if (wParam = WMSZ_BOTTOMRIGHT)
				{
					rect = (RECT*)lParam;
					h = (int)((float)(rect->right - rect->left)*cw->GetRatio());
					if (h < CLIP_MIN_HEIGHT)
						h = CLIP_MIN_HEIGHT;
					rect->bottom = rect->top + h;
				}
			}
			else
				cw->UpdateRatio ();
			break;
		case WM_GETMINMAXINFO:				// Window resizing restrictions
			wMinMax = (MINMAXINFO*)lParam;
			wMinMax->ptMinTrackSize.x = CLIP_MIN_WIDTH;
			wMinMax->ptMinTrackSize.y = CLIP_MIN_HEIGHT;
			wMinMax->ptMaxTrackSize.x = ScrWidth - CLIP_SCREEN_DELTA_X;
			wMinMax->ptMaxTrackSize.y = ScrHeight - CLIP_SCREEN_DELTA_Y;
			break;
		case WM_SIZE:						// Update size info
			cw->SetSize (LOWORD(lParam), HIWORD(lParam));
			break;
		case WM_NCLBUTTONDBLCLK:			// Restore original size
			cw->ResetSize ();
			break;
		case WM_PAINT:						// Redraw window
			cw->Draw ();
			break;
		case WM_ERASEBKGND:					// Tell Windows not to erase background
			return 1;
		case WM_CLOSE:						// Window close
			break;
		case WM_MOVE:
			cw->SetPosition (LOWORD(lParam), HIWORD(lParam));
			break;
		case WM_ENTERSIZEMOVE:
			cw->Shadow (false);
			cw->Quality (false);
			break;
		case WM_EXITSIZEMOVE:
			cw->Shadow (true);
			cw->Quality (true);
			cw->Draw ();
			cw->Freeze ();
			break;
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_INACTIVE)
				cw->ShowNameBox (false);
			break;
		default:
			ret = DefWindowProc (hwnd, msg, wParam, lParam);

			if (msg == WM_NCHITTEST)
			{
				if (ret == HTCLIENT)		// Catch resize handle clicks
				{
					GetWindowPlacement (hwnd, &wPos);
					if ((LOWORD(lParam) > wPos.rcNormalPosition.right - CLIP_HANDLE_SIZE) &&
						(HIWORD(lParam) > wPos.rcNormalPosition.bottom - CLIP_HANDLE_SIZE))
					{
						return HTBOTTOMRIGHT;
					}
				}
				return HTCAPTION;			// Redirect client area mouse to nonclient area
			}

			return ret;
	}

	return 0;
}

class BitmapEx : public Bitmap  
{
public:
   inline Bitmap* StretchClone (INT nWidth, INT nHeight);
public:
   virtual ~BitmapEx ();
};
inline Bitmap* BitmapEx::StretchClone (INT nWidth, INT nHeight)
{
   Bitmap* pBitmap = new Bitmap (nWidth, nHeight); 
   Graphics graphics (pBitmap);
   graphics.SetInterpolationMode (InterpolationModeHighQualityBicubic);
   graphics.DrawImage (this, 0, 0, nWidth, nHeight);
   return pBitmap;
}
