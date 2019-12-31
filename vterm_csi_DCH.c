#include <stdlib.h>

#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_buffer.h"

/* Interpret the 'delete chars' sequence (DCH) */
void
interpret_csi_DCH(vterm_t *vterm, int param[], int pcount)
{
    vterm_desc_t    *v_desc = NULL;
    vterm_cell_t    *vcell_src;
    vterm_cell_t    *vcell_dst;
    int             stride;
    int             idx;
    int             n = 1;
    int             c;

    if(pcount && param[0] > 0) n = param[0];

    // select the correct desc
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    stride = v_desc->cols - v_desc->ccol;
    stride -= n;

    // copy into temporary buffer
    vcell_src = &v_desc->cells[v_desc->crow][v_desc->ccol];
    vcell_src += n;
    vcell_dst = &v_desc->cells[v_desc->crow][v_desc->ccol];

    /*
        by experintation and review of other emulators it seems that DCH
        only copies the character from its neighbor and not the attributes
        or colors.  IMO that's bizarre but reality.
    */
    for(c = 0; c < stride; c++)
    {
        memcpy(&vcell_dst->wch, &vcell_src->wch, sizeof(vcell_dst->wch));

        VCELL_ROW_SET_DIRTY(vcell_dst, 1);

        vcell_src++;
        vcell_dst++;
    }

    // zero out cells created by the void
    vcell_dst = &v_desc->cells[v_desc->crow][v_desc->ccol];
    vcell_dst += stride;

    // same logic as above change the character of the cell only
    for(c = 0; c < n; c++)
    {
        VCELL_SET_CHAR((*vcell_dst), ' ');
        VCELL_SET_COLORS((*vcell_dst), v_desc);

        vcell_dst++;
    }

    return;
}

