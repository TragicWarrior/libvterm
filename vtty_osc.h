#ifndef _VTTY_OSC_
#define _VTTY_OSC_

#include "vtty.h"
#include "vtty_private.h"

#define IS_OSC_MODE(_obj)   (_obj->internal_state & STATE_OSC_MODE) 

int     vtty_interpret_xterm_osc(vtty_t *vtty);

#endif
