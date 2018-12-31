#ifndef _VTERM_UTF8_H_
#define _VTERM_UTF8_H_

#ifndef NOCURSES
#  include <ncursesw/curses.h>
#endif

#include "vterm.h"
#include "wchar.h"

void    vterm_utf8_start(vterm_t *vterm);
void    vterm_utf8_cancel(vterm_t *vterm);

int     vterm_utf8_decode(vterm_t *vterm, chtype *utf8_char, wchar_t *wch);

#endif
