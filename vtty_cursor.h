#ifndef _VTTY_CURSOR_H_
#define _VTTY_CURSOR_H_

#include "vtty_private.h"
#include "vtty.h"

void    vtty_cursor_move_home(vtty_t *vtty);
void    vtty_cursor_move_backward(vtty_t *vtty);
void    vtty_cursor_move_tab(vtty_t *vtty);

void    vtty_cursor_show(vtty_t *vtty, int idx);
void    vtty_cursor_hide(vtty_t *vtty, int idx);

void    vtty_cursor_save(vtty_t *vtty);
void    vtty_cursor_restore(vtty_t *vtty);

#endif
