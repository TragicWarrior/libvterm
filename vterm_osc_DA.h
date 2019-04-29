#ifndef _VTERM_OSC_DA_
#define _VTERM_OSC_DA_

#include "vterm.h"

/*
    DA (device attributes) OSC

    Inquiry Sequence:   ESC [ 0 c
    Reply Sequence:     ESC [ ? 1 ; # c ]

    Where # is one of the following

    0 Base VT100, no options
    1 Processor options (STP)
    2 Advanced video option (AVO)
    3 AVO and STP
    4 Graphics processor option (GPO)
    5 GPO and STP
    6 GPO and AVO
    7 GPO, STP, and AVO
*/

// libvterm will reply with code 7
#define     OSC_DA_REPLY        "[\e?1;7c]"

int interpret_osc_DA(vterm_t *vterm);

#endif
