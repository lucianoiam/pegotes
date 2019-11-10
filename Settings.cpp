#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include "Settings.h"
#include "Config.h"
#include "Clip.h"
#include "Capture.h"
#include "Util.h"
#include "resource.h"
#include "Locale.h"
#include "TrayIcon.h"

bool bSetupWindow;
Settings_t Settings;

INT_PTR CALLBACK SetupProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK LanguageWindowProc (HWND hwnd, LPARAM lParam);

// int LoadSettings (void)
// Loads saved settings
int LoadSettings (void)
{
	HKEY	key;
	DWORD	size;
	int		ret;

	Settings.Version = 0;
	ret = RegOpenKeyEx (HKEY_CURRENT_USER, L"Software\\" VENDOR_NAME L"\\" APP_NAME,
						0, KEY_READ|KEY_WRITE, &key);
	if (ret != ERROR_SUCCESS)
		return -1;

	size = sizeof (DWORD);

	RegQueryValueEx (key, KEY_VERSION, NULL, NULL,
			(unsigned char*)&Settings.Version, &size);

	ret = RegQueryValueEx (key, KEY_ENCODER, NULL, NULL,
			(unsigned char*)&Settings.Encoder, &size);
	if ((ret != ERROR_SUCCESS) || (Settings.Encoder > ENCODER_LAST))
	{
		Settings.Encoder = ENCODER_PNG;
		RegSetValueEx (key, KEY_ENCODER, 0, REG_DWORD,
				(unsigned char*)&Settings.Encoder, sizeof(DWORD));
	}

	ret = RegQueryValueEx (key, KEY_LANGUAGE, NULL, NULL,
			(unsigned char*)&Settings.Language, &size);
	if ((ret != ERROR_SUCCESS) || (Settings.Language >= LANGUAGE_LAST))
		GetOSLanguage ();
	RegSetValueEx (key, KEY_LANGUAGE, 0, REG_DWORD,
		(unsigned char*)&Settings.Language, sizeof(DWORD));

	LoadStringTable (Settings.Language);

	ret = RegCloseKey (key);
	if (ret != ERROR_SUCCESS)
		return -1;

	return 0;
}

// int UpdateVersion (void)
// Creates registry key
int UpdateVersion (void)
{
	HKEY	key;
	int		ret;

	ret = RegCreateKeyEx (HKEY_CURRENT_USER, L"Software\\" VENDOR_NAME L"\\" APP_NAME,
							0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, NULL);
	if (ret != ERROR_SUCCESS)
		return -1;

	Settings.Version = APP_VERSION;
	ret = RegSetValueEx (key, KEY_VERSION, 0, REG_DWORD,
			(unsigned char*)&Settings.Version, sizeof(DWORD));
	if (ret != ERROR_SUCCESS)
		return -1;
	
	ret = RegCloseKey (key);
	if (ret != ERROR_SUCCESS)
		return -1;

	return 0;
}

// void RestoreWindows (HWND hwnd)
// Loads saved windows
void RestoreWindows (HWND hwnd)
{
#ifndef _DEBUG
	HKEY		key, subkey;
	DWORD		data, x, y, w, h, size = 255;
	wchar_t		path[MAX_PATH], file[255], clipfolder[255];
	CClipWindow	*cw;
	int			id, i = 0;

	RegOpenKeyEx (HKEY_CURRENT_USER, L"Software\\" VENDOR_NAME L"\\" APP_NAME,
					0, KEY_READ, &key);

	while (RegEnumKeyEx (key, i++, file, &size, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
	{
		// Load bitmap from application data folder

		SHGetSpecialFolderPath (NULL, path, CSIDL_LOCAL_APPDATA, FALSE);

		wcscat (path, L"\\" VENDOR_NAME L"\\" APP_NAME L"\\" IMAGES_FOLDER L"\\");
		wcscat (path, DEFAULT_CLIP_FILENAME);
		wcscat (path, file);
		wcscat (path, Extensions[0]);
		wsprintf (clipfolder, L"Software\\%s\\%s\\%s", VENDOR_NAME, APP_NAME, file);
		
		RegOpenKeyEx (HKEY_CURRENT_USER, clipfolder, 0, KEY_READ, &subkey);

		swscanf (file, L"%d", &id);

		size = sizeof (DWORD);
		RegQueryValueEx (subkey, KEY_X, NULL, NULL, (unsigned char*)&x, &size);

		size = sizeof (DWORD);
		RegQueryValueEx (subkey, KEY_Y, NULL, NULL, (unsigned char*)&y, &size);

		size = sizeof (DWORD);
		RegQueryValueEx (subkey, KEY_WIDTH, NULL, NULL, (unsigned char*)&w, &size);
		
		size = sizeof (DWORD);		
		RegQueryValueEx (subkey, KEY_HEIGHT, NULL, NULL, (unsigned char*)&h, &size);

		Bitmap *diskbmp = new Bitmap (path, FALSE);
		if (diskbmp->GetType () != ImageTypeUnknown)
		{
			// Stupid GDI+ locks files so have to make a copy
			Bitmap *bitmap = new Bitmap (diskbmp->GetWidth (), diskbmp->GetHeight ());
			Graphics graphics (bitmap);
			graphics.DrawImage (diskbmp, 0, 0, diskbmp->GetWidth (), diskbmp->GetHeight ());

			cw = new CClipWindow (x, y, id, bitmap, Settings.Language, false, hwnd);

			cw->Lock (1);
			cw->SetSize (w, h);

			size = sizeof(wchar_t)*MAX_CLIP_TEXT;
			RegQueryValueEx (subkey, KEY_NAME, NULL, NULL, (unsigned char*)cw->m_Name, &size);

			size = 4;
			data = 1;
			RegQueryValueEx (subkey, KEY_ON_TOP, NULL, NULL, (unsigned char*)&data, &size);
			cw->SetOnTop (data);

			size = 4;
			data = 0;
			RegQueryValueEx (subkey, KEY_HIDDEN, NULL, NULL, (unsigned char*)&data, &size);
			cw->Show (!data);

			cw->Lock (0);
		}
		delete diskbmp;
		size = 255;
	}

	RegCloseKey (key);
#endif
}

// int Setup (HWND hwnd)
// Shows setup window
int Setup (HWND hwnd)
{
	HKEY	key;
	int		ret;

	if (bSetupWindow)
		return 0;

	bSetupWindow = true;

	Settings.Save = FALSE;
	Settings.Install = FALSE;
	Settings.LangChanged = FALSE;

	/* TODO: Hacer esto bien

	HRSRC	hResource;
	HGLOBAL	hResData;
	LPCWSTR	pwchMem;

	hResource = FindResourceEx (NULL, RT_DIALOG, MAKEINTRESOURCE(IDD_DIALOG_SETUP),
				MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US));
	hResData = LoadResource (NULL, hResource);
	pwchMem = (LPCWSTR)LockResource(hResData);
	ret = DialogBoxParam (GetModuleHandle (NULL), , hwnd, SetupProc, 0);
	*/

	if (Settings.Language == LANGUAGE_ENGLISH)
		DialogBoxParam (GetModuleHandle (NULL),
			MAKEINTRESOURCE(IDD_DIALOG_SETUP_EN), hwnd, SetupProc, 0);
	else if (Settings.Language == LANGUAGE_SPANISH)
		DialogBoxParam (GetModuleHandle (NULL),
			MAKEINTRESOURCE(IDD_DIALOG_SETUP_ES), hwnd, SetupProc, 0);
	else if (Settings.Language == LANGUAGE_GERMAN)
		DialogBoxParam (GetModuleHandle (NULL),
			MAKEINTRESOURCE(IDD_DIALOG_SETUP_DE), hwnd, SetupProc, 0);

	if (Settings.Save)
	{
		ret = RegOpenKeyEx (HKEY_CURRENT_USER, L"Software\\" VENDOR_NAME L"\\" APP_NAME,
							0, KEY_WRITE, &key);
		if (ret != ERROR_SUCCESS)
			return -1;

		ret = RegSetValueEx (key, KEY_ENCODER, 0, REG_DWORD,
					(unsigned char*)&Settings.Encoder, sizeof(DWORD));
		if (ret != ERROR_SUCCESS)
			return -1;

		ret = RegSetValueEx (key, KEY_LANGUAGE, 0, REG_DWORD,
					(unsigned char*)&Settings.Language, sizeof(DWORD));
		if (ret != ERROR_SUCCESS)
			return -1;

		ret = RegCloseKey (key);
		if (ret != ERROR_SUCCESS)
			return -1;

		if (Settings.Install)
		{					
			ret = Install (TRUE);
			if (ret < 0)
				Error (IDS_ERROR_INSTALL);
			else
				PostQuitMessage (-2);
		}

		if (Settings.LangChanged)
		{
			LoadStringTable (Settings.Language);
			CreateTrayMenu (hwnd);
			EnumWindows (LanguageWindowProc, 0);
		}
	}

	bSetupWindow = false;

	return 0;
}

// INT_PTR CALLBACK SetupProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
// Setup window callback
INT_PTR CALLBACK SetupProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HWND	hwndOwner; 
	RECT	rc, rcDlg, rcOwner; 
	int		i;
	wchar_t	exe[MAX_PATH];
	wchar_t	path[MAX_PATH];
	
	switch (uMsg)
	{
		case WM_INITDIALOG:
			hwndOwner = GetDesktopWindow(); 

			GetWindowRect(hwndOwner, &rcOwner); 
			GetWindowRect(hwndDlg, &rcDlg); 
			CopyRect(&rc, &rcOwner); 

			// Offset the owner and dialog box rectangles so that right and bottom 
			// values represent the width and height, and then offset the owner again 
			// to discard space taken up by the dialog box. 
			OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top); 
			OffsetRect(&rc, -rc.left, -rc.top); 
			OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom); 

			// The new position is the sum of half the remaining space and the owner's 
			// original position. 
			SetWindowPos(hwndDlg, HWND_TOP, rcOwner.left + (rc.right / 2), 
							rcOwner.top + (rc.bottom / 2), 0, 0, SWP_NOSIZE); 

			// Select current encoder
			CheckRadioButton (hwndDlg, IDC_RADIO_PNG, IDC_RADIO_PNG+ENCODER_LAST,
								IDC_RADIO_PNG+Settings.Encoder);

#ifndef _DEBUG
			// Check if already running from startup
			GetModuleFileName (NULL, exe, MAX_PATH);
			SHGetSpecialFolderPath (NULL, path, CSIDL_STARTUP, FALSE);

			if (wcswcs (exe, path) != NULL)
			{
				EnableWindow (GetDlgItem (hwndDlg, IDC_CHECK_INSTALL), FALSE);
				CheckDlgButton (hwndDlg, IDC_CHECK_INSTALL, BST_CHECKED);
				SetWindowText (GetDlgItem (hwndDlg, IDC_CHECK_INSTALL),
						StrTable[IDS_SETUP_INSTALLED]);
			}
#endif

			// Initialise language combo box
			i = 0;
			while (Languages[i].ID != LANGUAGE_LAST)
				SendMessage (GetDlgItem (hwndDlg, IDC_COMBO_LANG), CB_ADDSTRING,
							(WPARAM)i, (LPARAM)Languages[i++].Name);
			i = 0;
			while (Languages[i].ID != Settings.Language)
				i++;
			SendMessage (GetDlgItem (hwndDlg, IDC_COMBO_LANG), CB_SETCURSEL, i, 0);

			SetFocus (GetDlgItem (hwndDlg, IDOK));
			break;
		case WM_CLOSE:
			EndDialog (hwndDlg, -1);
			break;
        case WM_COMMAND: 
            switch (LOWORD(wParam)) 
            {
                case IDOK:
					Settings.Save = TRUE;
					for (i = ENCODER_PNG; i <= ENCODER_LAST; i++)
						if (IsDlgButtonChecked (hwndDlg, IDC_RADIO_PNG+i) == BST_CHECKED)
						{
							Settings.Encoder = i;
							break;
						}
					Settings.Install = IsWindowEnabled (GetDlgItem (hwndDlg, IDC_CHECK_INSTALL)) &&
							(BOOL)(IsDlgButtonChecked (hwndDlg, IDC_CHECK_INSTALL) == BST_CHECKED);

					i = SendMessage (GetDlgItem (hwndDlg, IDC_COMBO_LANG), CB_GETCURSEL, 0, 0);
					if (Settings.Language != Languages[i].ID)
					{
						Settings.Language = Languages[i].ID;
						Settings.LangChanged = TRUE;
					}
					EndDialog (hwndDlg, 0);
					break;
				case IDCANCEL:
					EndDialog (hwndDlg, -1);
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}

	return FALSE;
}

// int Install (BOOL fromsetup)
// Install program in startup folder
int Install (BOOL fromsetup)
{
	wchar_t				src[MAX_PATH];
	wchar_t				dst[MAX_PATH];
	wchar_t				path[MAX_PATH];
	STARTUPINFO			startupinfo;
	PROCESS_INFORMATION	procinfo;

#ifndef _DEBUG
	GetModuleFileName (NULL, src, MAX_PATH);
	SHGetSpecialFolderPath (NULL, path, CSIDL_STARTUP, FALSE);
	wsprintf (dst, L"%s\\%s.exe", path, APP_NAME);

	if (CopyFile (src, dst, FALSE) == 0)
#endif
		return -1;

	startupinfo.cb = sizeof (STARTUPINFO);
	startupinfo.lpReserved = NULL;
	startupinfo.lpDesktop = L"";
	startupinfo.lpTitle = NULL;
	startupinfo.cbReserved2 = 0;
	startupinfo.lpReserved2 = NULL;

	wcscat (dst, L" /p");

	if (!fromsetup)
	{
		wchar_t path[MAX_PATH];
		GetTempPath (MAX_PATH, path);
		wcscat (path, APP_NAME);

		if (GetFileAttributes (path) == INVALID_FILE_ATTRIBUTES)
			CreateDirectory (path, NULL);

		wcscat (path,  L"\\" OLD_EXE_FILE);
		FILE *file = _wfopen (path, L"w+");

		if (file != NULL)
		{
			fwprintf (file, src);
			fclose (file);
		}

		wcscat (dst, L" /h /d");
	}

	CreateProcess (NULL, dst, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS,
					NULL, path, &startupinfo, &procinfo);

	return 0;
}

// BOOL CALLBACK LanguageWindowProc (HWND hwnd, LPARAM lParam)
// Clip change language callback function
BOOL CALLBACK LanguageWindowProc (HWND hwnd, LPARAM lParam)
{
	wchar_t			classname[32];
	CClipWindow		*cw;

	GetClassName (hwnd, classname, 32);

	if (wcscmp (classname, WIN_CLASS_CLIP) == 0)
	{
		cw = (CClipWindow*)GetWindowLongPtr (hwnd, GWLP_USERDATA);

		if (cw != NULL)
			cw->SetLanguage (Settings.Language);
	}

	return TRUE;
}

// void GetOSLanguage (void)
// Get current language and load matching string table
void GetOSLanguage (void)
{
	switch (PRIMARYLANGID(GetUserDefaultLangID ()))
	{
		case LANG_ENGLISH:
			Settings.Language = LANGUAGE_ENGLISH;
			break;
		case LANG_SPANISH:
			Settings.Language = LANGUAGE_SPANISH;
			break;
		case LANG_GERMAN:
			Settings.Language = LANGUAGE_GERMAN;
			break;
		default:
			Settings.Language = LANGUAGE_ENGLISH;
			break;
	}

	LoadStringTable (Settings.Language);
}
