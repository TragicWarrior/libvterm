#include "vterm_csi.h"
#include "vterm_private.h"
#include "vterm_buffer.h"

/*
    This function repeats a character X times.

    On xterm the CSI is: ESC [ n b

    ...where 'n' is the number of times that the previous character should
    be repeated.
*/
void
interpret_csi_REP(vterm_t *vterm, int param[], int pcount)
{
    vterm_cell_t    *vcell_prev;
    vterm_cell_t    *vcell;
    vterm_desc_t    *v_desc = NULL;
    int             idx;
    int             max_col;
    int             c;

    if(vterm == NULL) return;

    // set the vterm desciption selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    // if repeat value isn't supplied, return.  nothing to do.
    if(pcount == 0) return;

    // pcount[0] should contain the stride (rep count)

    // max sure repeat doesn't overrun right edge of screen
    max_col = v_desc->ccol + param[0];
    if(max_col > v_desc->cols) max_col = v_desc->cols;

    vcell = &v_desc->cells[v_desc->crow][v_desc->ccol];

    vcell_prev = &v_desc->cells[v_desc->crow][v_desc->ccol - 1];

    for(c = v_desc->ccol; c < max_col; c++)
    {
        memcpy(vcell, vcell_prev, sizeof(vterm_cell_t));

        VCELL_ROW_SET_DIRTY(vcell, 1);

        vcell++;
    }

    // advance position to end of stride
    v_desc->ccol += param[0];

    return;
}
