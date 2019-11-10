#ifndef _CLIP_H
#define _CLIP_H

#include <gdiplus.h>
#include "Config.h"
#include "WndShadow.h"

using namespace Gdiplus;

#define CLIP_HANDLE_SIZE			20
#define CLIP_TEXT_HEIGHT			17
#define CLIP_MIN_WIDTH				32
#define CLIP_MIN_HEIGHT				32
#define CLIP_SCREEN_DELTA_X			100
#define CLIP_SCREEN_DELTA_Y			75
#define CLIP_ID_STATIC				0xF000
#define CLIP_ID_HELP				(CLIP_ID_STATIC + 0)
#define CLIP_ID_ABOUT				(CLIP_ID_STATIC + 1)
#define BRIGHTNESS_THRESHOLD		75
#define MAX_CLIP_TEXT				24
#define MAX_CLIPS					10000
#define WIN_NAME_CLIP				L"Clip"
#define WIN_CLASS_CLIP				L"ClipWindowClass"
#define WIN_NAME_INPUT				L"InputWindowClass"
#define IDC_NAME_EDIT				101

extern LRESULT CALLBACK ClipWinProc (HWND, UINT, WPARAM, LPARAM);

class CClipWindow
{
	public:
		wchar_t	m_Name[MAX_CLIP_TEXT];		// TODO: private
		bool	m_TextChanged;

		CClipWindow (int, int, int, Bitmap *, int, bool, HWND);
		virtual ~CClipWindow ();
		float GetRatio (void);
		int GetID (void);
		BOOL IsHidden (void);
		void SetSize (int, int);
		void ResetSize (void);
		void UpdateRatio (void);
		void SetPosition (int, int);
		void SetOnTop (int);
		void Show (int);
		void Draw (void);
		void Menu (void);
		void FadeOut (void);
		int Save (LPCWSTR);
		int DragAndDrop (int);
		void RemoveSavedData (void);
		void Lock (int);
		void SetLanguage (int);
		void Shadow (bool);
		void Quality (bool);
		void ShowNameBox (bool);
		BOOL IsNameBoxVisible (void);
		void Freeze (void);

	private:
		int			m_id;
		int			m_x, m_y;
		int			m_w, m_h;
		float		m_r;
		int			m_OnTop;
		int			m_Hidden;
		int			m_Locked;
		bool		m_HiQuality;
		wchar_t		m_Filename[MAX_PATH];
		Bitmap		*m_GpBitmap;
		Pen			*m_GpPenBorder, *m_GpPenHandle;
		HMENU		m_Menu;
		HWND		m_hWnd, m_texthWnd;
		HKEY		m_Key;
		CWndShadow	*m_Shadow;
};

#endif
