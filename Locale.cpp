#include <windows.h>
#include "Locale.h"
#include "resource.h"

wchar_t StrTable[IDS_LAST][256];
Language_t Languages[LANGUAGE_LAST+1] = {
	{LANGUAGE_ENGLISH, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), L"English"},
	{LANGUAGE_SPANISH, MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH), L"Español"},
	{LANGUAGE_GERMAN,  MAKELANGID(LANG_GERMAN,  SUBLANG_GERMAN), L"Deutsch"},
	{LANGUAGE_LAST, 0, L""}
};

// int LoadStringTable (int lang)
// Loads string array from specified language resource strings
int LoadStringTable (int lang)
{
	wchar_t	*pRes, *p;
	HRSRC	hRes;
	int		i, j, size;

	j = 1;
	while (j < IDS_LAST)
	{
		hRes = FindResourceEx (NULL, RT_STRING, MAKEINTRESOURCE(j/16 + 1),
									Languages[lang].Win32LangID);
		if (hRes == NULL)
			return -1;

		pRes = (wchar_t*)LoadResource (NULL, hRes);
		if (pRes == NULL)
			return -1;

		p = pRes;
		for (i = 0; i < 16; i++)
		{
			if (*p)
			{
				size = *p;			// String size in characters.
				p++;
				if (i == j%16)		// String found in table
				{
					wcsncpy (StrTable[j], p, size);
					StrTable[j][size] = '\0';
				}
				p += size;
			}
			else
				p++;
		}
		j++;
	}

	return 0;
} 

wchar_t *HelpText[LANGUAGE_LAST] = {

	// English

	L"Mouse and key combinations\n" \
	L"\n" \
	L"\n" \
	L"SYSTEM-WIDE\n" \
	L"\n" \
	L"Select screen area              - <PrtScr>\n" \
	L"Save screenshot to desktop      - <Shift> + <PrtScr>\n" \
	L"Show all clips                  - <Ctrl> + <PrtScr>\n" \
	L"Hide all clips                  - <Alt> + <PrtScr>\n" \
	L"\n" \
	L"\n" \
	L"ON SYSTEM TRAY ICON\n" \
	L"\n" \
	L"Menu                            - Mouse R\n" \
	L"Select screen area              - Double click\n" \
	L"\n" \
	L"\n" \
	L"ON SELECTION SCREEN\n" \
	L"\n" \
	L"Create clip                     - Mouse drag\n" \
	L"Save selection to desktop       - <Shift> + Mouse drag\n" \
	L"Cancel                          - Mouse R or <Esc>\n" \
	L"\n" \
	L"\n" \
	L"ON CLIP WINDOW\n" \
	L"\n" \
	L"Move clip                       - Mouse drag\n" \
	L"Drag & drop clip image          - <Ctrl> + Mouse drag\n" \
	L"Drag & drop and close clip      - <Shift> + Mouse drag\n" \
	L"Clip options                    - Mouse R\n" \
	L"Free resize                     - <Shift>\n" \
	L"Restore original size           - Double click\n" \
	L"\n" \
	L"\n" \
	L"ON CLIP MENU\n" \
	L"\n" \
	L"Rename clip                     - <Shift> + \"Hide\" option\n" \
	L"",

	// Spanish

	L"Combinaciones de teclas y mouse\n" \
	L"\n" \
	L"\n" \
	L"GLOBALES\n" \
	L"\n" \
	L"Elegir una porción de pantalla          - <ImprPant>\n" \
	L"Guardar pantalla en el escritorio       - <Shift> + <ImprPant>\n" \
	L"Mostrar todos los recortes              - <Ctrl> + <ImprPant>\n" \
	L"Esconder todos los recortes             - <Alt> + <ImprPant>\n" \
	L"\n" \
	L"\n" \
	L"EN EL ÍCONO DE LA BANDEJA DEL SISTEMA\n" \
	L"\n" \
	L"Menú                                    - Mouse D\n" \
	L"Elegir una porción de pantalla          - Doble clic\n" \
	L"\n" \
	L"\n" \
	L"EN LA PANTALLA DE SELECCIÓN\n" \
	L"\n" \
	L"Crear recorte                           - Arrastrar\n" \
	L"Guardar selección en el escritorio      - <Shift> + Arrastrar\n" \
	L"Cancelar                                - Mouse D o <Esc>\n" \
	L"\n" \
	L"\n" \
	L"EN EL RECORTE\n" \
	L"\n" \
	L"Mover recorte                           - Arrastrar\n" \
	L"Arrastrar y soltar imagen del recorte   - <Ctrl> + Arrastrar\n" \
	L"Arrastrar y soltar y borrar recorte     - <Shift> + Arrastrar\n" \
	L"Opciones del recorte                    - Mouse D\n" \
	L"Cambiar tamaño en forma libre           - <Shift>\n" \
	L"Volver al tamaño original               - Doble clic\n" \
	L"\n" \
	L"\n" \
	L"EN EL MENÚ DEL RECORTE\n" \
	L"\n" \
	L"Renombrar recorte                       - <Shift> + opción \"Esconder\"\n" \
	L"",

	// German

	L"Maus und Tastenfunktionen\n" \
	L"\n" \
	L"\n" \
	L"IMMER VERFÜGBARE FUNKTIONEN:\n" \
	L"\n" \
	L"Auswählmodus starten                                    - <Druck>/<PrtScr>\n" \
	L"Screenshot auf Desktop speichern                        - <Shift> + <Druck>/<PrtScr>\n" \
	L"Alle Clips anzeigen                                     - <Strg>/<Ctrl> + <Druck>/<PrtScr>\n" \
	L"Alle Clips verstecken                                   - <Alt> + <Druck>/<PrtScr>\n" \
	L"\n" \
	L"\n" \
	L"FUNKTIONEN ÜBER SYSTEM-TRAY-ICON:\n" \
	L"\n" \
	L"Menü                                                    - Rechte Maustaste\n" \
	L"Auswählmodus starten                                    - Doppelklick\n" \
	L"\n" \
	L"\n" \
	L"FUNKTIONEN IM AUSWAHLMODUS:\n" \
	L"\n" \
	L"Clip erstellen                                          - Mit der Maus Bereich markieren\n" \
	L"Clip erstellen und direkt auf dem Desktop speichern     - <Shift> + Mit der Maus Bereich markieren\n" \
	L"Abbrechen                                               - Rechte Maustaste oder <Esc>\n" \
	L"\n" \
	L"\n" \
	L"FUNKTIONEN IM CLIP-FENSTER/AUSWAHL:\n" \
	L"\n" \
	L"Clip verschieben                                        - Mit der Maus verschieben\n" \
	L"Clip an Ort verschieben/dort speichern                  - <Strg>/<Ctrl> + verschieben\n" \
	L"Clip an Ort verschieben/dort speichern und schließen    - <Shift> + verschieben\n" \
	L"Clip Optionen                                           - Rechte Maustaste\n" \
	L"Größe des Clips frei ändern                             - <Shift> + rechte untere Ecke ziehen\n" \
	L"Ursprüngliche Größe wiederherstellen                    - Doppelklick\n" \
	L"\n" \
	L"Clip benennen                                           - <Shift>, Rechtsklick auf Clip, dann \"Verstecke\" wählen\n" \
	L""
};
