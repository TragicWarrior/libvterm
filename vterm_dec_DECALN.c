
#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_buffer.h"

/*
    ESC # 8

    This is designed for service technicians.  It fills the screen with
    the character 'E'.
*/

void
interpret_dec_DECALN(vterm_t *vterm)
{
    vterm_desc_t    *v_desc = NULL;
    int             r;

    // set the vterm description buffer selector
    v_desc = vterm->v_desc_active;

    for(r = 0;r < v_desc->rows; r++)
    {
        vterm_fill_span(v_desc, r, 0, v_desc->cols - 1, L'E',
            A_NORMAL, v_desc->default_colors);
    }

    return;
}

