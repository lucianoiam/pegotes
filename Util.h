#ifndef _UTIL_H
#define _UTIL_H

#include <windows.h>

struct txtinputdlg
{
	LPCWSTR	Prompt;
	LPWSTR	Text;
	int		Max;
};
typedef struct txtinputdlg TextInputDialog_t;

extern int GetEncoderClsid (const WCHAR *, CLSID *);
extern HRESULT GetUIObjectOfFile (HWND, LPWSTR, REFIID, void **);
extern HRESULT __fastcall UnicodeToAnsi(LPWSTR, LPSTR);
extern void Message (const wchar_t *, int i);
extern void Error (int);

// This should be in WinUser.h
#define WS_EX_LAYERED		0x00080000
#define LWA_COLORKEY		0x00000001
#define LWA_ALPHA			0x00000002
typedef BOOL (WINAPI *lpfnSetLayeredWindowAttributes)(HWND hWnd, 
				COLORREF crKey, BYTE bAlpha, DWORD dwFlags);
extern lpfnSetLayeredWindowAttributes m_pSetLayeredWindowAttributes;

// Macros
#define KEY_DOWN(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 1 : 0)
#define KEY_UP(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 1 : 0)

#endif
