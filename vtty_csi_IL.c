
#include <string.h>

#include "vtty.h"
#include "vtty_private.h"
#include "vtty_csi.h"
#include "vtty_buffer.h"

/* Interpret an 'insert line' sequence (IL) */
void
interpret_csi_IL(vtty_t *vtty,int param[],int pcount)
{
    vtty_desc_t     *v_desc = NULL;
    int             r;
    int             n = 1;

    // set vtty description buffer selector
    v_desc = vtty->v_desc_active;

    if(pcount && param[0] > 0) n = param[0];

    for(r = v_desc->scroll_max; r >= v_desc->crow + n; r--)
    {
        memcpy(v_desc->cells[r], v_desc->cells[r - n],
            sizeof(vtty_cell_t) * v_desc->cols);

        VCELL_DIRTY_SET_ROW(v_desc, r);
    }

    for(r = v_desc->crow; r < v_desc->crow + n; r++)
    {
        if(r > v_desc->scroll_max) break;

        vtty_fill_span(v_desc, r, 0, v_desc->cols - 1, L' ',
            v_desc->curattr, v_desc->colors);
    }

    return;
}
