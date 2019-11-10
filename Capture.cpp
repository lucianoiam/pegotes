#include <windows.h>
#include <gdiplus.h>
#include <shlobj.h>
#include "Capture.h"
#include "Util.h"
#include "Config.h"
#include "Clip.h"
#include "Settings.h"
#include "resource.h"
#include "Locale.h"

using namespace Gdiplus;

EncoderParameters encParms;
LPCWSTR Extensions[MAX_ENCODERS] = {L".png", L".jpg"};
LPCWSTR MIMETypes[MAX_ENCODERS] = {L"image/png", L"image/jpeg"};

extern Settings_t Settings;
extern int ScrWidth, ScrHeight;

// int SaveRectangle (int *number, RECT *rect, Bitmap **bitmap)
// Saves a screen rectangle
int SaveRectangle (int *number, RECT *rect, Bitmap **bitmap)
{
	int			n, w, h, i, ret = 0;
	HWND		hDesktopWnd;
	HDC			hDesktopDC;
	HBITMAP		hbm;
	HDC			hdc;
	CLSID		encClsid;
	wchar_t		swFileName[MAX_PATH], file[32];

	w = rect->right - rect->left + 2;			// Prevent border artifacts
	h = rect->bottom - rect->top + 2;			// GDI+ bug

	// Grab screen rectangle
	hDesktopWnd = GetDesktopWindow ();			// Get desktop DC and bitmap
	hDesktopDC = GetDC (hDesktopWnd);
	hdc = CreateCompatibleDC (hDesktopDC);		// Create mem DC and bitmap
	hbm = CreateCompatibleBitmap (hDesktopDC, w, h);
	SelectObject (hdc, hbm);
	BitBlt (hdc, 0, 0, w, h, hDesktopDC,		// Copy desktop bitmap to mem bitmap
			rect->left-1, rect->top-1, SRCCOPY);

	*bitmap = new Bitmap (hbm, NULL);			// Copy GDI bitmap to GDI+ bitmap
#ifndef _DEBUG

	n = rand() % MAX_CLIPS;

	i = 0;
	do {
		if (number  != NULL)
		{
			// Save bitmap to application data folder
			wsprintf (file, L"%s%04d%s", DEFAULT_CLIP_FILENAME, n, Extensions[0]);	
			*number = n;
			SHGetSpecialFolderPath (NULL, swFileName, CSIDL_LOCAL_APPDATA, FALSE);
			wcscat (swFileName, L"\\" VENDOR_NAME L"\\" APP_NAME L"\\" IMAGES_FOLDER L"\\");
		}
		else
		{
			// Save bitmap to desktop
			wsprintf (file, L"%s%04d%s", DEFAULT_CLIP_FILENAME, n, Extensions[Settings.Encoder]);
			SHGetSpecialFolderPath (NULL, swFileName, CSIDL_DESKTOP, FALSE);
			wcscat (swFileName, L"\\");
		}

		wcscat (swFileName, file);
		i++;
	}
	while ((i < MAX_CLIPS) && (GetFileAttributes (swFileName) != INVALID_FILE_ATTRIBUTES));

	if (i == MAX_CLIPS)
		return -1;

	GetEncoderClsid	(MIMETypes[Settings.Encoder], &encClsid);
	if ((*bitmap)->Save (swFileName, &encClsid, &encParms) != Ok)
		ret = -1;

#endif
	ReleaseDC (hDesktopWnd, hDesktopDC);		// Free GDI objects
	DeleteDC (hdc);
	DeleteObject (hbm);

	return ret;
}

// int SaveDesktop (BOOL dialog)
// Saves a screenshot
int SaveDesktop (BOOL dialog)
{
	int				ret = 0;
#ifndef _DEBUG
	HWND				hDesktopWnd;
	HDC					hDesktopDC;
	HDC					hdc;
	HBITMAP				hbm;
	OPENFILENAME		ofn;
	CLSID				encClsid;
	wchar_t				swFileName[MAX_PATH];
	wchar_t				swFilter[32] = L"";

	hDesktopWnd = GetDesktopWindow ();
	hDesktopDC = GetDC (hDesktopWnd);
	hdc = CreateCompatibleDC (hDesktopDC);
	hbm = CreateCompatibleBitmap (hDesktopDC, ScrWidth, ScrHeight);
	SelectObject (hdc, hbm);
	BitBlt (hdc, 0, 0, ScrWidth, ScrHeight, hDesktopDC, 0, 0, SRCCOPY);

	Bitmap* desktopbmp = new Bitmap (hbm, NULL); 
 
	ReleaseDC (hDesktopWnd, hDesktopDC);
	DeleteDC (hdc);
	DeleteObject (hbm);

	GetEncoderClsid	(MIMETypes[Settings.Encoder], &encClsid);

	if (dialog)
	{
		ZeroMemory(&ofn, sizeof(ofn));
		wcscpy (swFileName, DEFAULT_DESKTOP_FILENAME);

		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = NULL;
		ofn.lpstrFilter = SAVE_FILE_TYPES;
		ofn.nFilterIndex = 1+Settings.Encoder;
		ofn.lpstrFile = swFileName;
		ofn.nMaxFile = MAX_PATH;
		ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT;
		ofn.lpstrDefExt = Extensions[Settings.Encoder]+1;
		ofn.lpstrTitle = StrTable[IDS_SAVE_DESKTOP];
		ofn.lpstrCustomFilter = swFilter;
		ofn.nMaxCustFilter = 32;

		if (!GetSaveFileName (&ofn))
			return -1;

		for (int i = 0; i < MAX_ENCODERS; i++)
			if (wcscmp (2+swFilter, Extensions[i]) == 0)
			{
				GetEncoderClsid	(MIMETypes[i], &encClsid);
				break;
			}
	}
	else
	{
		SHGetSpecialFolderPath (NULL, swFileName, CSIDL_DESKTOP, FALSE);

		wcscat (swFileName, L"\\");
		wcscat (swFileName, DEFAULT_DESKTOP_FILENAME);
		wcscat (swFileName, Extensions[Settings.Encoder]);
	}

	if (desktopbmp->Save (swFileName, &encClsid, &encParms) != Ok)
		ret = -1;
	delete desktopbmp;
#endif
	return ret;
}
