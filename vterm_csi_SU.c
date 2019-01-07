
#include <string.h>

#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_buffer.h"

/* Interpret a 'scroll up' sequence (SU) */
void
interpret_csi_SU(vterm_t *vterm, int param[], int pcount)
{
    // vterm_cell_t    *vcell;
    vterm_desc_t    *v_desc = NULL;
    int             r;
    int             n = 1;          // number of scroll lines
    int             idx;
    int             top_row;
    int             bottom_row;

    // set selector for buffer description
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    if(pcount > 0)
    {
        if(param[0] > 0) n = param[0];
    }

    bottom_row = v_desc->scroll_max - n;
    // vcell = &v_desc->cells[0][0];

    for(r = v_desc->scroll_min; r < bottom_row; r++)
    {
        memcpy(v_desc->cells[r], v_desc->cells[r + n],
                sizeof(vterm_cell_t) * v_desc->cols);
    }

    // for(c = 0; c < v_desc->cols; c++)
    // {
    //     CELL_SET_CHAR((*vcell), ' ');
    //    VCELL_SET_ATTR((*vcell), v_desc->curattr);

    //    vcell++;
    //}

    return;
}
