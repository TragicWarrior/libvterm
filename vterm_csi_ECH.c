
#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_buffer.h"

/* Interpret an 'erase characters' (ECH) sequence */
void
interpret_csi_ECH(vterm_t *vterm, int param[], int pcount)
{
    vterm_desc_t    *v_desc = NULL;
    int             n = 1;
    int             max_col;

    // set vterm descipton buffer selector
    v_desc = vterm->v_desc_active;

    if(pcount && param[0] > 0) n = param[0];

    max_col = v_desc->ccol + n;
    if(max_col > v_desc->cols) max_col = v_desc->cols;

    vterm_fill_span(v_desc, v_desc->crow, v_desc->ccol, max_col - 1,
        L' ', v_desc->curattr, v_desc->colors);

    return;
}
