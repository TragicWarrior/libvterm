
#include <string.h>

#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_buffer.h"

/* Interpret a 'delete line' sequence (DL) */
void
interpret_csi_DL(vterm_t *vterm, int param[], int pcount)
{
    vterm_desc_t    *v_desc = NULL;
    int             r;
    int             n = 1;

    // set selector for buffer description
    v_desc = vterm->v_desc_active;

    if(pcount && param[0] > 0) n = param[0];

    for(r = v_desc->crow; r <= v_desc->scroll_max; r++)
    {
        if(r + n <= v_desc->scroll_max)
        {
            memcpy(v_desc->cells[r], v_desc->cells[r + n],
                sizeof(vterm_cell_t) * v_desc->cols);

            VCELL_DIRTY_SET_ROW(v_desc, r);
        }
        else
        {
            vterm_fill_span(v_desc, r, 0, v_desc->cols - 1, L' ',
                v_desc->curattr, v_desc->colors);
        }
    }

    return;
}
