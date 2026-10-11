
#ifndef _VTTY_ESCAPE_H_
#define _VTTY_ESCAPE_H_

#include "vtty.h"

void    vtty_esbuf_init(vtty_t *vtty);
void    vtty_esbuf_reset(vtty_t *vtty);
int     vtty_esbuf_putc(vtty_t *vtty, char c);

void    vtty_escape_start(vtty_t *vtty);
void    vtty_escape_cancel(vtty_t *vtty);

void    vtty_interpret_escapes(vtty_t *vtty);
int     vtty_interpret_escapes_simple(vtty_t *vtty, char verb);

#endif

