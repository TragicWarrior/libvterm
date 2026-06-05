
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
    int             r, c;
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

        for(c = 0; c < v_desc->cols; c++)
        {
            VCELL_SET_CHAR(v_desc, r, c, ' ');
            VCELL_SET_ATTR(v_desc, r, c, v_desc->curattr);
            VCELL_SET_COLORS(v_desc, r, c);
        }
    }

    return;
}
