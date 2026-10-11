
#include "vtty.h"
#include "vtty_private.h"
#include "vtty_csi.h"
#include "vtty_buffer.h"

/*
    ESC # 8

    This is designed for service technicians.  It fills the screen with
    the character 'E'.
*/

void
interpret_dec_DECALN(vtty_t *vtty)
{
    vtty_desc_t     *v_desc = NULL;
    int             r;

    // set the vtty description buffer selector
    v_desc = vtty->v_desc_active;

    for(r = 0;r < v_desc->rows; r++)
    {
        vtty_fill_span(v_desc, r, 0, v_desc->cols - 1, L'E',
            A_NORMAL, v_desc->default_colors);
    }

    return;
}

