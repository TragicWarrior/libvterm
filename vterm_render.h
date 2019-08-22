
#ifndef _VTERM_RENDER_H_
#define _VTERM_RENDER_H_

#ifndef NOCURSES

#ifdef __linux__
#include <ncursesw/curses.h>
#endif

#ifdef __FreeBSD__
#include <ncurses/ncurses.h>
#endif

#endif

#include "vterm.h"


#endif
