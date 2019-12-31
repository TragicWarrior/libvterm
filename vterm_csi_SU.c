
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
    vterm_cell_t    *vcell;
    int             r;
    int             n = 1;          // number of scroll lines
    int             idx;
    int             bottom_row;
    int             top_row;
    int             stride;
    int             i;

    // set selector for buffer description
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    if(pcount > 0)
    {
        if(param[0] > 0) n = param[0];
    }

    // safety checks
    if(n > v_desc->rows) return;
    if(n < 1) return;

    top_row = v_desc->scroll_min;
    bottom_row = v_desc->scroll_max - (n - 1);
    stride = sizeof(vterm_cell_t) * v_desc->cols;

    for(r = top_row; r < bottom_row; r++)
    {
        memcpy(v_desc->cells[r], v_desc->cells[r + n], stride);

        VCELL_ROW_SET_DIRTY(v_desc->cells[r], v_desc->cols);
    }

    top_row = v_desc->scroll_max - (n - 1);
    bottom_row = v_desc->scroll_max + 1;

    for(r = top_row; r < bottom_row; r++)
    {
        vcell = &v_desc->cells[r][0];

        for(i = 0; i < v_desc->cols; i++)
        {
            VCELL_SET_CHAR((*vcell), ' ');
            VCELL_SET_ATTR((*vcell), v_desc->curattr);
            VCELL_SET_COLORS((*vcell), v_desc);

            vcell++;
        }
    }

    return;
}

