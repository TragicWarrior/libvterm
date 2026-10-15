#ifndef NOUTF8
#ifndef _VTTY_UTF8_H_
#define _VTTY_UTF8_H_

#ifndef NOCURSES

#ifdef __linux__
#include <ncursesw/curses.h>
#endif

#ifdef __FreeBSD__
#include <ncurses/ncurses.h>
#endif

#endif

#include "vtty.h"
#include "wchar.h"

void    vtty_utf8_start(vtty_t *vtty);
void    vtty_utf8_cancel(vtty_t *vtty);

int     vtty_utf8_decode(vtty_t *vtty, wchar_t *wch);

#endif
#endif
