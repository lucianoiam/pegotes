#ifndef _SELECTION_H
#define _SELECTION_H

#define WIN_NAME_SEL	L"Selection"
#define WIN_CLASS_SEL	L"SelectionWindowClass"

extern HWND CreateSelWin (HWND);
extern LRESULT CALLBACK SelWinProc (HWND, UINT, WPARAM, LPARAM);

extern HWND shWnd;

#endif
