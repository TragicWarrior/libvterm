
#include <string.h>

#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_buffer.h"


/*
    Interpret a 'scroll ' sequence (SU)

    ESC [ n S

    n = number of lines to scroll
*/

void
interpret_csi_SU(vterm_t *vterm, int param[], int pcount)
{
    vterm_desc_t    *v_desc = NULL;
    int             r;
    int             n = 1;          // number of scroll lines
    int             bottom_row;
    int             top_row;
    int             stride;

    // set selector for buffer description
    v_desc = vterm->v_desc_active;

    if(pcount > 0)
    {
        if(param[0] > 0) n = param[0];
    }

    // safety checks
    if(n < 1) return;

    /*
        clamp n to the scroll region height (see interpret_csi_SD).  without
        this, the clear loop's top_row (scroll_max - n + 1) can run below
        scroll_min -- or negative -- and index cells[] out of bounds.
    */
    if(n > v_desc->scroll_max - v_desc->scroll_min + 1)
        n = v_desc->scroll_max - v_desc->scroll_min + 1;

    top_row = v_desc->scroll_min;
    bottom_row = v_desc->scroll_max - (n - 1);
    stride = sizeof(vterm_cell_t) * v_desc->cols;

    for(r = top_row; r < bottom_row; r++)
    {
        memcpy(v_desc->cells[r], v_desc->cells[r + n], stride);

        VCELL_DIRTY_SET_ROW(v_desc, r);
    }

    top_row = v_desc->scroll_max - (n - 1);
    bottom_row = v_desc->scroll_max + 1;

    for(r = top_row; r < bottom_row; r++)
    {
        vterm_fill_span(v_desc, r, 0, v_desc->cols - 1, L' ',
            v_desc->curattr, v_desc->colors);
    }

    return;
}

