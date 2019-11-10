#ifndef _CAPTURE_H
#define _CAPTURE_H

#include <windows.h>
#include <gdiplus.h>

using namespace Gdiplus;

#define IMAGES_FOLDER				L"images"
#define DEFAULT_CLIP_FILENAME		L"Clip"
#define DEFAULT_DESKTOP_FILENAME	L"Desktop"
#define SAVE_FILE_TYPES				L"PNG\0*.png\0"				\
									L"JPEG\0*.jpg\0"			\
									L"All files (*.*)\0*.*\0"
#define MAX_ENCODERS				2
#define JPEG_QUALITY				75

extern LPCWSTR Extensions[MAX_ENCODERS];
extern LPCWSTR MIMETypes[MAX_ENCODERS];

extern EncoderParameters encParms;

extern int SaveRectangle (int *, RECT *, Bitmap **);
extern int SaveDesktop (BOOL);

#endif
