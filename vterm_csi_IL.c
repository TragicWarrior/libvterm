
#include <string.h>

#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_buffer.h"

/* Interpret an 'insert line' sequence (IL) */
void
interpret_csi_IL(vterm_t *vterm,int param[],int pcount)
{
    vterm_desc_t    *v_desc = NULL;
    int             r;
    int             n = 1;

    // set vterm description buffer selector
    v_desc = vterm->v_desc_active;

    if(pcount && param[0] > 0) n = param[0];

    for(r = v_desc->scroll_max; r >= v_desc->crow + n; r--)
    {
        memcpy(v_desc->cells[r], v_desc->cells[r - n],
            sizeof(vterm_cell_t) * v_desc->cols);

        VCELL_DIRTY_SET_ROW(v_desc, r);
    }

    for(r = v_desc->crow; r < v_desc->crow + n; r++)
    {
        if(r > v_desc->scroll_max) break;

        vterm_fill_span(v_desc, r, 0, v_desc->cols - 1, L' ',
            v_desc->curattr, v_desc->colors);
    }

    return;
}
