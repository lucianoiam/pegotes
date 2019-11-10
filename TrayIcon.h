#ifndef _TRAYICON_H
#define _TRAYICON_H

#include <windows.h>

#define SATAN				666
#define WM_TOOLTIP			(WM_USER + SATAN)
#define HOTKEY				VK_SNAPSHOT
#define MAX_MENU_CLIPS		1000
#define ID_TOOLTIP_MENU		0x01
#define WIN_NAME_MAIN		APP_NAME
#define WIN_CLASS_MAIN		L"PegotesWindowClass"

enum clipopt	{
					IDM_HIDE = 0,
					IDM_DELETE,
					IDM_SAVE,
					IDM_SEPARATOR,
					IDM_ON_TOP
				};

enum mainopt	{
					IDM_SEPARATOR_1 = MAX_MENU_CLIPS,
					IDM_SHOW_ALL,
					IDM_HIDE_ALL,
					IDM_NEW,
					IDM_SEPARATOR_2,
					IDM_SAVE_DESKTOP,
					IDM_OPTIONS,
					IDM_SEPARATOR_3,
					IDM_HELP,
					IDM_ABOUT,
					IDM_EXIT,
					IDM_SEPARATOR_4
				};

extern void CreateTrayIcon (HWND);
extern void CreateTrayMenu (HWND);
extern void AboutBox (HWND);
extern void ShowHelp (void);
extern LRESULT CALLBACK MainWinProc (HWND, UINT, WPARAM, LPARAM);
extern BOOL CALLBACK ModifyWindowsProc (HWND, LPARAM);

#endif
