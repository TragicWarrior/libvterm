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
    int             n = 1;
    int             c;

    if(pcount && param[0] > 0) n = param[0];

    // select the correct desc
    v_desc = vterm->v_desc_active;

    stride = v_desc->cols - v_desc->ccol;
    stride -= n;

    // copy into temporary buffer
    vcell_src = &v_desc->cells[v_desc->crow][v_desc->ccol + n];
    vcell_dst = &v_desc->cells[v_desc->crow][v_desc->ccol];

    /*
        by experintation and review of other emulators it seems that DCH
        only copies the character from its neighbor and not the attributes
        or colors.  IMO that's bizarre but reality.
    */
    for(c = 0; c < stride; c++)
    {
        memcpy(&vcell_dst->wch, &vcell_src->wch, sizeof(vcell_dst->wch));

        VCELL_DIRTY_SET(v_desc, v_desc->crow, v_desc->ccol + c);

        vcell_src++;
        vcell_dst++;
    }

    /*
        zero out cells created by the void.  deliberately NOT
        vterm_fill_span: DCH's researched behavior (see the comment
        above) blanks the glyph and colors but leaves attr untouched
        -- a fourth flavor with exactly one user.
    */
    for(c = 0; c < n; c++)
    {
        VCELL_SET_CHAR(v_desc, v_desc->crow, v_desc->ccol + stride + c, ' ');
        VCELL_SET_COLORS(v_desc, v_desc->crow, v_desc->ccol + stride + c);
    }

    return;
}

