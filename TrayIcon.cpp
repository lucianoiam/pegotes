#include <windows.h>
#include <stdio.h>
#include "TrayIcon.h"
#include "Clip.h"
#include "Selection.h"
#include "Util.h"
#include "Capture.h"
#include "resource.h"
#include "Settings.h"
#include "Locale.h"

static void ShowMenu (HWND);
BOOL CALLBACK EnumWindowsProc (HWND hwnd, LPARAM lParam);
BOOL CALLBACK ModifyWindowsProc (HWND hwnd, LPARAM lParam);
BOOL CALLBACK FindClipWindowProc (HWND hwnd, LPARAM lParam);

static int ClipCount;
static CClipWindow *FoundClip;
static UINT WM_TASKBARCREATED;
extern int ScrWidth, ScrHeight;

// LRESULT CALLBACK MainWinProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
// Main window message handling
LRESULT CALLBACK MainWinProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HMENU			menu;
	MENUITEMINFO	item;
	CClipWindow		*cw;

	if (msg == WM_TASKBARCREATED)
		CreateTrayIcon (hwnd);

	switch (msg)
	{
		case WM_DISPLAYCHANGE:					// Handle screen resolution change
			ScrWidth = LOWORD(lParam);
			ScrHeight = HIWORD(lParam);
			break;
		case WM_TOOLTIP:
			if (lParam == WM_RBUTTONUP)			// Show menu
				ShowMenu (hwnd);
			if (lParam == WM_LBUTTONDBLCLK)
				shWnd = CreateSelWin (hwnd);
			break;
		case WM_HOTKEY:
			if (shWnd == NULL)
			{
				if (KEY_DOWN(VK_SHIFT))
				{
					SaveDesktop (FALSE);
					break;
				}
				if (KEY_DOWN(VK_CONTROL))
				{
					EnumWindows (ModifyWindowsProc, 1);
					break;
				}
				if (KEY_DOWN(VK_MENU))
				{
					EnumWindows (ModifyWindowsProc, 2);
					break;
				}
				
				shWnd = CreateSelWin (hwnd);
			}
			else
				SendMessage (shWnd, WM_HOTKEY, 0, 0);
			break;
		case WM_QUIT:
		case WM_CLOSE:
			PostQuitMessage (0);
			break;
		case WM_COMMAND:						// Popup menu
			switch (LOWORD(wParam))
			{
				case IDM_SHOW_ALL:
					EnumWindows (ModifyWindowsProc, 1);
					break;
				case IDM_HIDE_ALL:
					EnumWindows (ModifyWindowsProc, 2);
					break;
				case IDM_SAVE_DESKTOP:
					SaveDesktop (TRUE);
					break;
				case IDM_OPTIONS:
					Setup (hwnd);
					break;
				case IDM_NEW:
					shWnd = CreateSelWin (hwnd);
					break;
				case IDM_HELP:
					ShowHelp ();
					break;
				case IDM_ABOUT:
					AboutBox (hwnd);
					break;
				case IDM_EXIT:
					// TODO: hacer esto bien
					PostQuitMessage (0);
					break;
				default:
					menu = GetMenu (hwnd);
					item.cbSize = sizeof (MENUITEMINFO);
					item.fMask = MIIM_DATA;
					GetMenuItemInfo (menu, LOWORD(wParam), FALSE, &item);
					cw = (CClipWindow*)item.dwItemData;
					cw->Show (1);
					break;
			}
			break;
		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	return 0;
}

// static void ShowMenu (HWND hwnd)
// System tray menu
static void ShowMenu (HWND hwnd)
{
	HMENU			menu;
	MENUITEMINFO	item;
	POINT			point;
	int				i;

	menu = GetMenu (hwnd);
	item.cbSize = sizeof (item);

	/*
		ID < MAX_MENU_CLIPS			Clip entries
		ID >= MAX_MENU_CLIPS		Program options
	*/

	// Delete dynamic options
	for (i = 0; i < MAX_MENU_CLIPS; i++)
		DeleteMenu (menu, i, MF_BYCOMMAND);
	DeleteMenu (menu, IDM_SEPARATOR_1, MF_BYCOMMAND);
	item.fMask = MIIM_STATE;
	item.fState = MFS_DISABLED;
	SetMenuItemInfo (menu, IDM_SHOW_ALL, FALSE, &item);
	SetMenuItemInfo (menu, IDM_HIDE_ALL, FALSE, &item);

	ClipCount = 0;
	EnumWindows (EnumWindowsProc, (LPARAM)menu);

	// At least 1 named clip window
	if (GetMenuItemCount (menu) > (IDM_SEPARATOR_4 - IDM_SEPARATOR_1 - 1))
	{
		// Menu insertion index
		i = GetMenuItemCount (menu) - (IDM_SEPARATOR_4 - IDM_SEPARATOR_1) + 1;
		item.fMask = MIIM_FTYPE | MIIM_ID;
		item.fType = MFT_SEPARATOR;
		item.wID = IDM_SEPARATOR_1;
		InsertMenuItem (menu, i, TRUE, &item);
	}
	if (ClipCount)
	{
		item.fMask = MIIM_STATE;
		item.fState = MFS_ENABLED;
		SetMenuItemInfo (menu, IDM_SHOW_ALL, FALSE, &item);
		SetMenuItemInfo (menu, IDM_HIDE_ALL, FALSE, &item);
	}

	GetCursorPos (&point);
	SetForegroundWindow (hwnd);
	TrackPopupMenuEx (menu, TPM_LEFTALIGN | TPM_HORIZONTAL | TPM_RIGHTBUTTON,
							point.x, point.y, hwnd, NULL);
}

// void CreateTrayIcon (HWND hwnd)
// Create or update tray icon
void CreateTrayIcon (HWND hwnd)
{
	NOTIFYICONDATA icondata;

	icondata.cbSize = sizeof(NOTIFYICONDATA);
	icondata.hWnd = hwnd;
	icondata.uID = ID_TOOLTIP_MENU;
	icondata.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
	icondata.hIcon = LoadIcon (GetModuleHandle (NULL), MAKEINTRESOURCE(IDI_ICON_SMALL));
	icondata.uCallbackMessage = WM_TOOLTIP;
	wcscpy (icondata.szTip, APP_NAME);

#ifndef _DEBUG
	Shell_NotifyIcon (NIM_ADD, &icondata);
#endif

	WM_TASKBARCREATED = RegisterWindowMessage (L"TaskbarCreated");
}

// void CreateTrayMenu (HWND hwnd)
// Set tray menu language
void CreateTrayMenu (HWND hwnd)
{
	HMENU			menu;
	MENUITEMINFO	item;

	menu = GetMenu (hwnd);

	int j = GetMenuItemCount (menu);
	for (int i = 0; i < j; i++)
		DeleteMenu (menu, 0, MF_BYPOSITION);

	// System tray menu
	item.cbSize = sizeof (item);
	// Hide all
	item.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
	item.fType = MFT_STRING;
	item.wID = IDM_HIDE_ALL;
	item.dwTypeData = StrTable[IDS_HIDE_ALL];
	item.cch = wcslen (item.dwTypeData);
	InsertMenuItem (menu, IDM_HIDE_ALL, FALSE, &item);
	// Show all
	item.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
	item.fType = MFT_STRING;
	item.wID = IDM_SHOW_ALL;
	item.dwTypeData = StrTable[IDS_SHOW_ALL];
	item.cch = wcslen (item.dwTypeData);
	InsertMenuItem (menu, IDM_SHOW_ALL, FALSE, &item);
	// New clip option
	item.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
	item.fType = MFT_STRING;
	item.wID = IDM_NEW;
	item.dwTypeData = StrTable[IDS_NEW];
	item.cch = wcslen (item.dwTypeData);
	InsertMenuItem (menu, IDM_NEW, FALSE, &item);
	// Separator
	item.fMask = MIIM_FTYPE | MIIM_ID;
	item.fType = MFT_SEPARATOR;
	InsertMenuItem (menu, IDM_SEPARATOR_3, FALSE, &item);
	// Save desktop option
	item.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
	item.fType = MFT_STRING;
	item.wID = IDM_SAVE_DESKTOP;
	item.dwTypeData = StrTable[IDS_SAVE_DESKTOP];
	item.cch = wcslen (item.dwTypeData);
	InsertMenuItem (menu, IDM_SAVE_DESKTOP, FALSE, &item);
	// Options
	item.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
	item.fType = MFT_STRING;
	item.wID = IDM_OPTIONS;
	item.dwTypeData = StrTable[IDS_OPTIONS];
	item.cch = wcslen (item.dwTypeData);
	InsertMenuItem (menu, IDM_OPTIONS, FALSE, &item);
	// Separator
	item.fMask = MIIM_FTYPE | MIIM_ID;
	item.fType = MFT_SEPARATOR;
	InsertMenuItem (menu, IDM_SEPARATOR_4, FALSE, &item);
	// Help option
	item.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
	item.fType = MFT_STRING;
	item.wID = IDM_HELP;
	item.dwTypeData = StrTable[IDS_HELP];
	item.cch = wcslen (item.dwTypeData);
	InsertMenuItem (menu, IDM_HELP, FALSE, &item);
	// About option
	item.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
	item.fType = MFT_STRING;
	item.wID = IDM_ABOUT;
	item.dwTypeData = StrTable[IDS_ABOUT];
	item.cch = wcslen (item.dwTypeData);
	InsertMenuItem (menu, IDM_ABOUT, FALSE, &item);
	// Exit option
	item.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
	item.fType = MFT_STRING;
	item.wID = IDM_EXIT;
	item.dwTypeData = StrTable[IDS_EXIT];
	item.cch = wcslen (item.dwTypeData);
	InsertMenuItem (menu, IDM_EXIT, FALSE, &item);
}

// void AboutBox (HWND hwnd)
// Shows program information
void AboutBox (HWND hwnd)
{
	CClipWindow	*cw;
	Bitmap		*bitmap;
	int			x, y, w, h;

	EnumWindows (FindClipWindowProc, (LPARAM)CLIP_ID_ABOUT);

	if (FoundClip == NULL)
	{
		bitmap = new Bitmap (GetModuleHandle (NULL),
					(const wchar_t*)MAKEINTRESOURCE(IDB_BITMAP_ABOUT));

		w = (bitmap->GetWidth()*ScrWidth) / 1024;
		h = w * (bitmap->GetHeight() / bitmap->GetWidth());
		x = (int)((ScrWidth  - w) *  0.966f);
		y = (int)((ScrHeight - h) *  0.05f);

		cw = new CClipWindow (x, y, CLIP_ID_ABOUT, bitmap, Settings.Language, false, hwnd);
		cw->SetSize (w, h);
	}
	else
		cw = FoundClip;

	cw->Show (1);
}

// BOOL CALLBACK EnumWindowsProc (HWND hwnd, LPARAM lParam)
// Clip enumerator callback function
BOOL CALLBACK EnumWindowsProc (HWND hwnd, LPARAM lParam)
{
	wchar_t			classname[32];
	MENUITEMINFO	item;
	HMENU			menu;
	int				id;
	CClipWindow		*cw;

	GetClassName (hwnd, classname, 32);

	if (wcscmp (classname, WIN_CLASS_CLIP) == 0)
	{
		ClipCount++;
		cw = (CClipWindow*)GetWindowLongPtr (hwnd, GWLP_USERDATA);

		if ((cw != NULL) && cw->IsHidden () && cw->m_Name[0])
		{
			menu = (HMENU)lParam;
			id = 1 + GetMenuItemCount (menu) - (IDM_SEPARATOR_4-IDM_SEPARATOR_1);

			item.cbSize = sizeof (item);
			item.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID | MIIM_DATA;
			item.fType = MFT_STRING;
			item.wID = id;
			item.dwItemData = (ULONG_PTR)cw;
			item.dwTypeData = cw->m_Name;
			item.cch = wcslen (cw->m_Name);

			InsertMenuItem (menu, 0, TRUE, &item);
		}
	}

	return TRUE;
}

// BOOL CALLBACK ModifyWindowsProc (HWND hwnd, LPARAM lParam)
// Clip delete or hide/show callback function
BOOL CALLBACK ModifyWindowsProc (HWND hwnd, LPARAM lParam)
{
	wchar_t			classname[32];
	CClipWindow		*cw;

	GetClassName (hwnd, classname, 32);

	if (wcscmp (classname, WIN_CLASS_CLIP) == 0)
	{
		cw = (CClipWindow*)GetWindowLongPtr (hwnd, GWLP_USERDATA);

		if (cw != NULL)
			switch (lParam)
			{
				case 0:
					delete cw;
					break;
				case 1:
					cw->Show (1);
					break;
				case 2:
					cw->Show (0);
					break;
				default:
					break;
			}
	}

	return TRUE;
}

// void ShowHelp (void)
// Shows help
void ShowHelp (void)
{
	wchar_t				procfile[MAX_PATH];
	wchar_t				tempfile[MAX_PATH];
	wchar_t				temppath[MAX_PATH];
    STARTUPINFO         stinfo;
    PROCESS_INFORMATION prinfo;

	GetWindowsDirectory (procfile, MAX_PATH);
	wcscat (procfile, L"\\notepad.exe");
	GetTempPath (MAX_PATH, temppath);
	wsprintf (tempfile, L" %s%s\\%s.txt", temppath, APP_NAME, StrTable[IDS_HELP]);

	FILE *file = _wfopen (tempfile+1, L"w+");
	if (file == NULL)
		return;

	fwprintf (file, HelpText[Settings.Language]);
	fclose (file);

    memset (&stinfo, 0, sizeof(stinfo));
    memset (&prinfo, 0, sizeof(prinfo));
	stinfo.cb = sizeof(stinfo);

	CreateProcess (procfile, tempfile, 0, 0, FALSE, CREATE_DEFAULT_ERROR_MODE,
					0, 0, &stinfo, &prinfo);
}

// BOOL CALLBACK FindClipWindowProc (HWND hwnd, LPARAM lParam)
// Search clip by ID
BOOL CALLBACK FindClipWindowProc (HWND hwnd, LPARAM lParam)
{
	wchar_t			classname[32];
	CClipWindow		*cw;

	FoundClip = NULL;

	GetClassName (hwnd, classname, 32);

	if (wcscmp (classname, WIN_CLASS_CLIP) == 0)
	{
		cw = (CClipWindow*)GetWindowLongPtr (hwnd, GWLP_USERDATA);

		if ((cw != NULL) && cw->GetID () == (int)lParam)
		{
			FoundClip = cw;
			return FALSE;
		}
	}

	return TRUE;
}
