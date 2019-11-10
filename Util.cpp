#include <windows.h>
#include <shlobj.h>
#include <gdiplus.h>
#include "Resource.h"
#include "Config.h"
#include "Util.h"
#include "Clip.h"
#include "Settings.h"
#include "Locale.h"

using namespace Gdiplus;

// int GetEncoderClsid (const WCHAR* format, CLSID* pClsid)
// Get an image encoder CLSID
int GetEncoderClsid (const WCHAR* format, CLSID* pClsid)
{
   UINT  num = 0;          // number of image encoders
   UINT  size = 0;         // size of the image encoder array in bytes

   ImageCodecInfo* pImageCodecInfo = NULL;

   GetImageEncodersSize(&num, &size);
   if(size == 0)
      return -1;  // Failure

   pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
   if(pImageCodecInfo == NULL)
      return -1;  // Failure

   GetImageEncoders(num, size, pImageCodecInfo);

   for(UINT j = 0; j < num; ++j)
   {
      if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
      {
         *pClsid = pImageCodecInfo[j].Clsid;
         free(pImageCodecInfo);
         return j;  // Success
      }    
   }

   free(pImageCodecInfo);
   return -1;  // Failure
}

// HRESULT GetUIObjectOfFile (HWND hwnd, LPWSTR pszPath, REFIID riid, void **ppv)
// Get a file OLE object
HRESULT GetUIObjectOfFile (HWND hwnd, LPWSTR pszPath, REFIID riid, void **ppv)
{
#ifndef _DEBUG
	HRESULT			hr;
	LPITEMIDLIST	pidl;
	IShellFolder	*shf;

	SHGetDesktopFolder (&shf);
	hr = shf->ParseDisplayName (hwnd, NULL, pszPath, NULL, &pidl, NULL);

	if (SUCCEEDED(hr))
	{
		LPCITEMIDLIST	pidlChild;
		IShellFolder	*psf;

		hr = SHBindToParent (pidl, IID_IShellFolder, (void**)&psf, &pidlChild);

		if (SUCCEEDED(hr))
		{
			*ppv = NULL;
			hr = psf->GetUIObjectOf (hwnd, 1, &pidlChild, riid, NULL, ppv);
			psf->Release();
		}

		CoTaskMemFree(pidl);
	}

	shf->Release ();

	return hr;
#else
	return 0;
#endif
}

/*
 * UnicodeToAnsi converts the Unicode string pszW to an ANSI string
 * and returns the ANSI string through ppszA. Space for the
 * the converted string is allocated by UnicodeToAnsi.
 */ 
HRESULT __fastcall UnicodeToAnsi(LPWSTR pszW, LPSTR ppszA)
{

    ULONG cbAnsi, cCharacters;
    DWORD dwError;

    // If input is null then just return the same.
    if (pszW == NULL)
        return NOERROR;

    cCharacters = wcslen(pszW)+1;
    // Determine number of bytes to be allocated for ANSI string. An
    // ANSI string can have at most 2 bytes per character (for Double
    // Byte Character Strings.)
    cbAnsi = cCharacters*2;

    if (NULL == ppszA)
        return E_OUTOFMEMORY;

    // Convert to ANSI.
    if (0 == WideCharToMultiByte(CP_ACP, 0, pszW, cCharacters, ppszA,
                  cbAnsi, NULL, NULL))
    {
        dwError = GetLastError();
        return HRESULT_FROM_WIN32(dwError);
    }
    return NOERROR;

}

// void Message (const wchar_t *s, int i)
// Displays a message box with a string and a number
void Message (const wchar_t *s, int i)
{
	wchar_t msg[128];

	wsprintf (msg, L"%s [0x%08X]", s, i);

	MessageBox (NULL, msg, L"Message", MB_OK);
}

// void Error (int errno)
// Displays an error message box
void Error (int errno)
{
	MessageBox (NULL, StrTable[errno], APP_NAME, MB_OK|MB_ICONERROR);
}













































// INT_PTR CALLBACK DialogProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
// Text input window callback
INT_PTR CALLBACK DialogProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RECT				rcOwner, rcSelf; 
	TextInputDialog_t	*data;

	return FALSE;

	data = (TextInputDialog_t*)GetWindowLongPtr (hwndDlg, GWLP_USERDATA);

	switch (uMsg)
	{
		case WM_INITDIALOG:
			GetWindowRect (hwndDlg, &rcSelf);
			GetWindowRect (GetParent(hwndDlg), &rcOwner);

			SetWindowPos(hwndDlg, HWND_TOP, rcOwner.left, rcOwner.top,
					rcOwner.right-rcOwner.left, rcSelf.bottom-rcSelf.top, SWP_NOMOVE);
			SetWindowPos(GetDlgItem (hwndDlg, IDC_EDIT), HWND_TOP, rcOwner.left, rcOwner.top,
					rcOwner.right-rcOwner.left, rcSelf.bottom-rcSelf.top, SWP_NOMOVE);

			data = (TextInputDialog_t*)lParam;
			SetWindowLongPtr (hwndDlg, GWLP_USERDATA, (long)data);

			// Window title
			SetWindowText (hwndDlg, WIN_NAME_INPUT);		// Window title

			// Edit box
			if (wcslen (data->Text) == 0)
				wcscpy (data->Text, data->Prompt);
			SetWindowText (GetDlgItem (hwndDlg, IDC_EDIT), data->Text);
			SendMessage (GetDlgItem (hwndDlg, IDC_EDIT), EM_SETSEL, 0, -1);
			SendMessage (GetDlgItem (hwndDlg, IDC_EDIT), EM_LIMITTEXT, data->Max-1, 0);
			
			SetFocus (GetDlgItem (hwndDlg, IDC_EDIT)); 
			break;
		case WM_CLOSE:
			EndDialog (hwndDlg, -1);
			break;
        case WM_COMMAND: 
            switch (LOWORD(wParam)) 
            { 
                case IDOK:
					GetDlgItemText (hwndDlg, IDC_EDIT, data->Text, MAX_CLIP_TEXT);
					if (wcsstr (data->Prompt, data->Text) != NULL)
						data->Text[0] = 0;
					EndDialog (hwndDlg, 0);
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

