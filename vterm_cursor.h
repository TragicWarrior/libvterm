#ifndef _VTERM_CURSOR_H_
#define _VTERM_CURSOR_H_

#include "vterm_private.h"
#include "vterm.h"

void    vterm_cursor_move_home(vterm_t *vterm);
void    vterm_cursor_move_backward(vterm_t *vterm);
void    vterm_cursor_move_tab(vterm_t *vterm);

void    vterm_cursor_show(vterm_t *vterm, int idx);
void    vterm_cursor_hide(vterm_t *vterm, int idx);

void    vterm_cursor_save(vterm_t *vterm);
void    vterm_cursor_restore(vterm_t *vterm);

#endif
