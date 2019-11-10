#include <windows.h>
#include <ole2.h>
#include <gdiplus.h>
#include <shlobj.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include "Resource.h"
#include "Config.h"
#include "Util.h"
#include "Settings.h"
#include "Capture.h"
#include "Selection.h"
#include "TrayIcon.h"
#include "Locale.h"
#include "WndShadow.h"

using namespace Gdiplus;

static HWND Init (HINSTANCE);
static void CleanUp (HWND);
static int CheckInstall (BOOL post);
static void DeleteOldFiles (void);

int			ScrWidth, ScrHeight;
ULONG_PTR	m_gdiplusToken;
HMODULE		hUser32 = GetModuleHandle(L"USER32.DLL");
lpfnSetLayeredWindowAttributes m_pSetLayeredWindowAttributes =
	(lpfnSetLayeredWindowAttributes)GetProcAddress(hUser32, 
	"SetLayeredWindowAttributes");

extern Settings_t Settings;
extern EncoderParameters encParms;

// WinMain
// Program entry point
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
					LPSTR lpCmdLine, int nCmdShow)
{
	HWND	hwnd;
	BOOL	post, help, del;
	MSG		msg;
	int		ret;

	//MessageBox (NULL, L"TEST VERSION - DO NOT DISTRIBUTE", APP_NAME, MB_OK|MB_ICONEXCLAMATION);

	GetOSLanguage ();						// Load string table

	shWnd = NULL;
	post = (BOOL) (strstr (lpCmdLine, CMD_POST_INSTALL) != NULL);
	help = (BOOL) (strstr (lpCmdLine, CMD_SHOW_HELP) != NULL);
	del = (BOOL) (strstr (lpCmdLine, CMD_DELETE_OLD_EXE) != NULL);

	if (del)								// Process /d switch
		DeleteOldFiles ();

	ret = CheckInstall (post);				// Check version and multiple instances
	if (ret < 0)
		return ret;

	hwnd = Init (hInstance);				// Create main window, etc
	if (hwnd == NULL)
		return -1;

	RestoreWindows (hwnd);
	
	if (help || (ret==1))					// Process /h switch
	{
		ShowHelp ();
		AboutBox (hwnd);
	}

	// Main loop
	while (GetMessage (&msg, NULL, 0, 0) > 0)
	{
		// Workaround for Windows API...
		// Redirect all keys to parent window for non-dialog controls
		if (msg.message == WM_CHAR)
			SendMessage (GetParent (msg.hwnd), WM_CHAR,
							msg.wParam, (LPARAM)msg.hwnd);

		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}
	
	CleanUp (hwnd);

	return 0;
}

// static HWND Init (HINSTANCE hInstance)
// Initialisation
static HWND Init (HINSTANCE hInstance)
{
	HWND				hwnd;
	WNDCLASSEX			wc;
	GdiplusStartupInput gdiplusStartupInput;
	wchar_t				path[MAX_PATH];
	wchar_t				file[MAX_PATH];
	HMENU				menu;

	// global variables
	bSetupWindow = false;

	// init random number generator
	srand ((unsigned int) time (NULL));

	// screen dimensions
	ScrWidth = GetSystemMetrics (SM_CXSCREEN);
	ScrHeight = GetSystemMetrics (SM_CYSCREEN);

	// GDI+ init
	if (GdiplusStartup (&m_gdiplusToken, &gdiplusStartupInput, NULL) != Ok)
		return NULL;
	// OLE init
	if (OleInitialize (NULL) != S_OK)
		return NULL;

	// Clean old temp files
	GetTempPath (MAX_PATH, path);
	wcscat (path, APP_NAME L"\\");
	if (GetFileAttributes (path) != INVALID_FILE_ATTRIBUTES)
	{
		WIN32_FIND_DATA	info;
		HANDLE			hp; 

		wcscpy (file, path);		
		wcscat (file, L"*.*");		
		
		hp = FindFirstFile (file, &info);

		if (hp != INVALID_HANDLE_VALUE)
		{
			do {
				wcscpy (file, path);
				wcscat (file, info.cFileName);
				DeleteFile (file);
			} while (FindNextFile (hp, &info));

			FindClose (hp);
		}
	}

	// Create Application Data subfolders
#ifndef _DEBUG
	SHGetSpecialFolderPath (NULL, path, CSIDL_LOCAL_APPDATA, TRUE);
#endif
	wcscat (path, L"\\" VENDOR_NAME);
	if (GetFileAttributes (path) == INVALID_FILE_ATTRIBUTES)
		CreateDirectory (path, NULL);
	wcscat (path, L"\\" APP_NAME);
	if (GetFileAttributes (path) == INVALID_FILE_ATTRIBUTES)
		CreateDirectory (path, NULL);
	wcscat (path, L"\\" IMAGES_FOLDER);
	if (GetFileAttributes (path) == INVALID_FILE_ATTRIBUTES)
		CreateDirectory (path, NULL);

	// Register main window class
	wc.cbSize        = sizeof (WNDCLASSEX);
	wc.style         = 0;
	wc.lpfnWndProc   = MainWinProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon (hInstance, MAKEINTRESOURCE(IDI_ICON_LARGE));
	wc.hCursor       = LoadCursor (NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject (BLACK_BRUSH);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = WIN_CLASS_MAIN;
	wc.hIconSm       = LoadIcon (hInstance, MAKEINTRESOURCE(IDI_ICON_LARGE));

	if (!RegisterClassEx (&wc))
		return NULL;

	// Register selection window class
	wc.cbSize        = sizeof (WNDCLASSEX);
	wc.style         = 0;
	wc.lpfnWndProc   = SelWinProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon (NULL, IDI_APPLICATION);
	wc.hCursor       = LoadCursor (NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject (BLACK_BRUSH);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = WIN_CLASS_SEL;
	wc.hIconSm       = LoadIcon (NULL, IDI_APPLICATION);

	if (!RegisterClassEx (&wc))
		return NULL;

	// Register clip window class
	wc.cbSize        = sizeof (WNDCLASSEX);
	wc.style         = 0;
	wc.lpfnWndProc   = ClipWinProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon (NULL, IDI_APPLICATION);
	wc.hCursor       = LoadCursor (NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject (WHITE_BRUSH);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = WIN_CLASS_CLIP;
	wc.hIconSm       = LoadIcon (NULL, IDI_APPLICATION);

	if (!RegisterClassEx (&wc))
		return NULL;

	menu = CreatePopupMenu ();
	hwnd = CreateWindowEx (WS_EX_TOOLWINDOW, WIN_CLASS_MAIN,
							WIN_NAME_MAIN, WS_POPUP, 0, 0, 0, 0,
							NULL, menu, hInstance, NULL);
	if (hwnd == NULL)
		return NULL;

	CreateTrayIcon (hwnd);
	CreateTrayMenu (hwnd);

    Settings.Quality = JPEG_QUALITY;
	encParms.Count = 1;
    encParms.Parameter[0].Guid = EncoderQuality;
    encParms.Parameter[0].NumberOfValues = 1;
    encParms.Parameter[0].Type = EncoderParameterValueTypeLong;
	encParms.Parameter[0].Value = &Settings.Quality;

	// Main window, show icon in task manager
	ShowWindow (hwnd, SW_SHOWNOACTIVATE);

	// Register PrtScr hits
	RegisterHotKey (hwnd, 0x00, 0, HOTKEY);
	RegisterHotKey (hwnd, 0x01, MOD_SHIFT, HOTKEY);
	RegisterHotKey (hwnd, 0x02, MOD_CONTROL, HOTKEY);
	RegisterHotKey (hwnd, 0x03, MOD_ALT, HOTKEY);

	// Window shadow class
	CWndShadow::Initialize(GetModuleHandle(NULL));

	return hwnd;
}

// static void CleanUp (HWND hwnd)
// Clean up before exit
static void CleanUp (HWND hwnd)
{
	NOTIFYICONDATA	icondata;

	// Close clip windows
	EnumWindows (ModifyWindowsProc, 0);
	DestroyMenu (GetMenu (hwnd));

	icondata.hWnd = hwnd;
	icondata.uID = ID_TOOLTIP_MENU;
#ifndef _DEBUG
	Shell_NotifyIcon (NIM_DELETE, &icondata);
#endif

	GdiplusShutdown (m_gdiplusToken);
	UnregisterHotKey (hwnd, 0x00);
	UnregisterHotKey (hwnd, 0x01);
	UnregisterHotKey (hwnd, 0x02);
	UnregisterHotKey (hwnd, 0x03);
}

// static int CheckInstall (BOOL post)
// Check if install needed
static int CheckInstall (BOOL post)
{
	HWND	phWnd;
	wchar_t	path[MAX_PATH];
	int		ret;

	ret = LoadSettings ();

	if (Settings.Version > APP_VERSION)
	{
		Error (IDS_ERROR_VERSION);
		return -1;
	}

	phWnd = FindWindow (WIN_CLASS_MAIN, WIN_NAME_MAIN);

	if (Settings.Version < APP_VERSION)			// Found older version
	{
		ret = UpdateVersion ();
		if (ret < 0)							// Update or create registry key
		{
			Error (IDS_ERROR_SETTINGS);
			return -1;
		}

		ret = MessageBox (NULL, StrTable[IDS_INSTALL_QUESTION],
					APP_NAME, MB_YESNO|MB_ICONQUESTION);
		if (phWnd != NULL)
		{	
			SendMessage (phWnd, WM_CLOSE, 0, 0);
			SendMessage (phWnd, WM_QUIT, 0 ,0);
			// Sleep (1000);
		}
		
		if (ret == IDYES)
		{
			ret = Install (FALSE);
			if (ret < 0)
				Error (IDS_ERROR_INSTALL);
			else
				return -2;
		}
#ifndef _DEBUG
		if (ret == IDNO)
		{
			SHGetSpecialFolderPath (NULL, path, CSIDL_STARTUP, FALSE);
			wcscat (path, L"\\" APP_NAME L".exe");
			DeleteFile (path);
		}
#endif

		return 1;
	}

	if ((phWnd != NULL) && !post && (Settings.Version == APP_VERSION))
	{
		PostMessage (phWnd, WM_HOTKEY, 0, 0);	// Don't allow multiple instances
		return -2;
	}

	return 0;
}

// static void DeleteOldFiles (void)
// Delete old version files
static void DeleteOldFiles (void)
{
	wchar_t	path[MAX_PATH];
	FILE	*file;
	
	GetTempPath (MAX_PATH, path);			// Create temp path
	wcscat (path, APP_NAME L"\\");
	if (GetFileAttributes (path) == INVALID_FILE_ATTRIBUTES)
	{
		CreateDirectory (path, NULL);
		return;
	}

	wcscat (path, OLD_EXE_FILE);
	file = _wfopen (path, L"r");

	if (file != NULL)
	{
		if (fgetws (path, MAX_PATH, file) != NULL)
			DeleteFile (path);				// Delete old exe
		fclose (file);
	}
}
