
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
    int             c;
    int             n = 1;
    int             idx;
    int             max_col;

    // set vterm descipton buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    if(pcount && param[0] > 0) n = param[0];

    max_col = v_desc->ccol + n;

    vcell = &v_desc->cells[v_desc->crow][v_desc->ccol];

    for(c = v_desc->ccol; c < max_col; c++)
    {
        if(c >= v_desc->cols) break;

        VCELL_SET_CHAR((*vcell), ' ');
        VCELL_SET_ATTR((*vcell), v_desc->curattr);
        VCELL_SET_COLORS((*vcell), v_desc);

        vcell++;
    }

    return;
}
