
#include "vtty.h"
#include "vtty_private.h"
#include "vtty_csi.h"
#include "vtty_buffer.h"

/* Interpret an 'erase characters' (ECH) sequence */
void
interpret_csi_ECH(vtty_t *vtty, int param[], int pcount)
{
    vtty_desc_t     *v_desc = NULL;
    int             n = 1;
    int             max_col;

    // set vtty descipton buffer selector
    v_desc = vtty->v_desc_active;

    if(pcount && param[0] > 0) n = param[0];

    max_col = v_desc->ccol + n;
    if(max_col > v_desc->cols) max_col = v_desc->cols;

    vtty_fill_span(v_desc, v_desc->crow, v_desc->ccol, max_col - 1,
        L' ', v_desc->curattr, v_desc->colors);

    return;
}
