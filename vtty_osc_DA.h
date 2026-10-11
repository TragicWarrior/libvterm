#ifndef _VTTY_OSC_DA_
#define _VTTY_OSC_DA_

#include "vtty.h"

/*
    DA (device attributes) OSC
*/

#define     OSC_DA_REPLY_XTERM  "\033[?62;0;6c"
//#define     OSC_DA_REPLY_XTERM  "\033[?62;1;6c"

#define     OSC_DA_REPLY_LINUX  "\033[?6c"

#define     OSC_DA_REPLY_VT100  "\033[?1;2c"


int interpret_osc_DA(vtty_t *vtty, int param[], int pcount);

#endif
