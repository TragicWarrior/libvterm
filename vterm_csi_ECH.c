
#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_buffer.h"

/* Interpret an 'erase characters' (ECH) sequence */
void
interpret_csi_ECH(vterm_t *vterm, int param[], int pcount)
{
    vterm_desc_t    *v_desc = NULL;
    int             c;
    int             n = 1;
    int             max_col;

    // set vterm descipton buffer selector
    v_desc = vterm->v_desc_active;

    if(pcount && param[0] > 0) n = param[0];

    max_col = v_desc->ccol + n;

    for(c = v_desc->ccol; c < max_col; c++)
    {
        if(c >= v_desc->cols) break;

        VCELL_SET_CHAR(v_desc, v_desc->crow, c, ' ');
        VCELL_SET_ATTR(v_desc, v_desc->crow, c, v_desc->curattr);
        VCELL_SET_COLORS(v_desc, v_desc->crow, c);
    }

    return;
}
