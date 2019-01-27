
#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_buffer.h"

/* Interpret the 'delete chars' sequence (DCH) */
void
interpret_csi_DCH(vterm_t *vterm, int param[], int pcount)
{
    vterm_cell_t    *vcell_new;
    vterm_cell_t    *vcell_old;
    vterm_desc_t    *v_desc = NULL;
    int             c;
    int             n = 1;
    int             idx;

    if(pcount && param[0] > 0) n = param[0];

    // select the correct desc
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    vcell_new = &v_desc->cells[v_desc->crow][v_desc->ccol];

    for(c = v_desc->ccol; c < v_desc->cols; c++)
    {
        if(c + n < v_desc->cols)
        {
            vcell_old = vcell_new + n;

            /*
                this is a shallow struct copy.  if a vterm_cell_t ever becomes
                packed with heap data referenced by pointer, it could
                be problematic.
            */
            *vcell_new = *vcell_old;
        }
        else
        {
            VCELL_SET_CHAR((*vcell_new), ' ');
            VCELL_SET_ATTR((*vcell_new), v_desc->curattr);
            VCELL_SET_COLORS((*vcell_new), v_desc->colors);
        }

        vcell_new++;
    }
}

