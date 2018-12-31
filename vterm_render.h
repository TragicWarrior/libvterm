
#ifndef _VTERM_RENDER_H_
#define _VTERM_RENDER_H_

#ifndef NOCURSES
#  include <ncursesw/curses.h>
#endif

#include "vterm.h"

void vterm_put_char(vterm_t *vterm, chtype c);
void vterm_render_ctrl_char(vterm_t *vterm, char c);
void try_interpret_escape_seq(vterm_t *vterm);

#endif
