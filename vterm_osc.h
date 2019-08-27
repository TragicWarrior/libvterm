#ifndef _VTERM_OSC_
#define _VTERM_OSC_

#include "vterm.h"
#include "vterm_private.h"

#define IS_OSC_MODE(_obj)   (_obj->internal_state & STATE_OSC_MODE) 

int     vterm_interpret_xterm_osc(vterm_t *vterm);

#endif
