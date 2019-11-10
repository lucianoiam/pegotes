#ifndef _LOCALE_H
#define _LOCALE_H

#include "resource.h"

enum langlist {
					LANGUAGE_ENGLISH = 0,
					LANGUAGE_SPANISH,
					LANGUAGE_GERMAN,
					LANGUAGE_LAST
				};

struct langtype
{
	DWORD	ID;
	WORD	Win32LangID;
	LPCWSTR	Name;
};
typedef struct langtype Language_t;

extern Language_t Languages[LANGUAGE_LAST+1];
extern wchar_t *HelpText[LANGUAGE_LAST];

extern wchar_t StrTable[IDS_LAST][256];
extern int LoadStringTable (int);

#endif
