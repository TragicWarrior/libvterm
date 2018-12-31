
#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_buffer.h"

/* Interpret an 'erase characters' (ECH) sequence */
void
interpret_csi_ECH(vterm_t *vterm, int param[], int pcount)
{
    vterm_cell_t    *vcell;
    vterm_desc_t    *v_desc = NULL;
    int             i;
    int             n = 1;
    int             idx;

    // set vterm descipton buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    if(pcount && param[0] > 0) n = param[0];

    for(i = v_desc->ccol; i < v_desc->ccol + n; i++)
    {
        if(i >= v_desc->cols) break;

        // save cell address to reduce scalar look-ups
        vcell = &v_desc->cells[v_desc->crow][i];

        VCELL_SET_CHAR((*vcell), ' ');
        VCELL_SET_ATTR((*vcell), v_desc->curattr);
    }

    return;
}
