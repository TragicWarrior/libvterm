
#ifndef _VTERM_ESCAPE_H_
#define _VTERM_ESCAPE_H_

#include "vterm.h"

void    vterm_escape_start(vterm_t *vterm);
void    vterm_escape_cancel(vterm_t *vterm);

void    vterm_interpret_escapes(vterm_t *vterm);
int     vterm_interpret_escapes_simple(vterm_t *vterm, char verb);

#endif

