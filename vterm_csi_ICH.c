
#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_buffer.h"

/* Interpret the 'insert blanks' sequence (ICH) */
void
interpret_csi_ICH(vterm_t *vterm, int param[], int pcount)
{
    vterm_cell_t    *vcell_new;
    vterm_cell_t    *vcell_old;
    vterm_desc_t    *v_desc = NULL;
    int             c;
    int             n = 1;
    int             max_stride;
    int             max_col;
    int             scr_end;

    // set the vterm desciption selector
    v_desc = vterm->v_desc_active;

    if(pcount && param[0] > 0) n = param[0];

    /*
        ICH has no effect beyond the edge so calculate the maximum stride
        possible and clamp 'n' to that value if ncessary.
    */
    max_stride = v_desc->cols - v_desc->ccol;
    if(n > max_stride) n = max_stride;

    /*
        calculate where the maximum column would be which is current
        column + n (the stride).
    */
    max_col = v_desc->ccol + n;

    // zero-based index of last column of the screen
    scr_end = v_desc->cols - 1;

    vcell_new = &v_desc->cells[v_desc->crow][scr_end];
    vcell_old = &v_desc->cells[v_desc->crow][scr_end - n];

    for(c = scr_end; c >= max_col; c--)
    {
        memcpy(vcell_new, vcell_old, sizeof(vterm_cell_t));
        VCELL_DIRTY_SET(v_desc, v_desc->crow, c);

        vcell_new--;
        vcell_old--;
    }

    vterm_fill_span(v_desc, v_desc->crow, v_desc->ccol, max_col - 1,
        L' ', v_desc->curattr, v_desc->colors);

    return;
}


