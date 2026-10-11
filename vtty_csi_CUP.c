
#include "vtty.h"
#include "vtty_private.h"
#include "vtty_csi.h"
#include "vtty_misc.h"
#include "vtty_buffer.h"

/*
    interprets a 'move cursor' (CUP) escape sequence.

    also the same as HVP.
*/
void
interpret_csi_CUP(vtty_t *vtty, int param[], int pcount)
{
    vtty_desc_t     *v_desc = NULL;

    // set active vtty description selector
    v_desc = vtty->v_desc_active;

    if (pcount == 0)
    {
        /* special case */
        v_desc->crow = 0;
        v_desc->ccol = 0;
        return;
    }
    else if (pcount < 2) return;  // malformed

    // convert from 1-based to 0-based.
    v_desc->crow = param[0] - 1;

    // convert from 1-based to 0-based.
    v_desc->ccol = param[1] - 1;

    clamp_cursor_to_bounds(vtty);
}

