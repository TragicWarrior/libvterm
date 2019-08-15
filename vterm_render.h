
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

void vterm_put_char(vterm_t *vterm, chtype c);
void vterm_render_ctrl_char(vterm_t *vterm, char c);
void try_interpret_escape_seq(vterm_t *vterm);

#endif
