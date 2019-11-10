#ifndef _SETTINGS_H
#define _SETTINGS_H

#include "Clip.h"

#define KEY_VERSION		L"Version"
#define KEY_ENCODER		L"Encoder"
#define KEY_X			L"X"
#define KEY_Y			L"Y"
#define KEY_WIDTH		L"Width"
#define KEY_HEIGHT		L"Height"
#define KEY_ON_TOP		L"OnTop"
#define KEY_HIDDEN		L"Hidden"
#define KEY_NAME		L"Name"
#define KEY_LANGUAGE	L"Language"

#define ENCODER_PNG		0
#define ENCODER_JPEG	1
#define ENCODER_LAST	ENCODER_JPEG

struct settings
{
	DWORD			Version;
	DWORD			Encoder;
	DWORD			Language;
	ULONG			Quality;
	BOOL			Save;
	BOOL			Install;
	BOOL			LangChanged;
};
typedef struct settings Settings_t;

extern bool bSetupWindow;
extern Settings_t Settings;

extern int LoadSettings (void);
extern int UpdateVersion (void);
extern void RestoreWindows (HWND);
extern int Setup (HWND);
extern int Install (BOOL);
extern void GetOSLanguage (void);

#endif
