
#include <string.h>

#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_buffer.h"

/* Interpret a 'delete line' sequence (DL) */
void
interpret_csi_DL(vterm_t *vterm, int param[], int pcount)
{
    vterm_cell_t    *vcell;
    vterm_desc_t    *v_desc = NULL;
    int             c, r;
    int             n = 1;
    int             idx;

    // set selector for buffer description
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    if(pcount && param[0] > 0) n = param[0];

    for(r = v_desc->crow; r <= v_desc->scroll_max; r++)
    {
        vcell = &v_desc->cells[r][0];

        if(r + n <= v_desc->scroll_max)
        {
            memcpy(v_desc->cells[r], v_desc->cells[r + n],
                sizeof(vterm_cell_t) * v_desc->cols);

            VCELL_ROW_SET_DIRTY(v_desc->cells[r], v_desc->cols);
        }
        else
        {
            for(c = 0; c < v_desc->cols; c++)
            {
                VCELL_SET_CHAR((*vcell), ' ');
                VCELL_SET_ATTR((*vcell), v_desc->curattr);
                VCELL_SET_COLORS((*vcell), v_desc);

                vcell++;
            }
        }
    }

    return;
}
