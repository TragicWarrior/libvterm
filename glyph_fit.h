#ifndef _GLYPH_FIT_H_
#define _GLYPH_FIT_H_

#include <stdbool.h>
#include <wchar.h>

/*
    true when the real terminal is the Linux console, the one host whose
    font cannot draw every character a child sends.  Asked once per draw.
*/
bool    vtty_glyph_fit_wanted(void);

/*
    a character the console can draw in place of wch, or wch itself when
    it can draw wch already (or there is nothing better to offer).
*/
wchar_t vtty_fit_glyph(wchar_t wch);

#endif
