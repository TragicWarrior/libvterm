
#ifndef _VTERM_WRITE_H_
#define _VTERM_WRITE_H_

#include <inttypes.h>

#include "vterm.h"

int vterm_write_rxvt(vterm_t *vterm,uint32_t keycode);
int vterm_write_vt100(vterm_t *vterm,uint32_t keycode);

#endif
