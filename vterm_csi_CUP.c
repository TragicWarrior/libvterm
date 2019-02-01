
#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_misc.h"
#include "vterm_buffer.h"

/*
    interprets a 'move cursor' (CUP) escape sequence.

    also the same as HVP.
*/
void
interpret_csi_CUP(vterm_t *vterm, int param[], int pcount)
{
    vterm_desc_t    *v_desc = NULL;
    int             idx;

    // set active vterm description selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

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

    clamp_cursor_to_bounds(vterm);
}

