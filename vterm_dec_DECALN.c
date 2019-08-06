
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
    vterm_cell_t    *vcell;
    int             idx;
    int             r, c;

    // set the vterm description buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    for(r = 0;r < v_desc->rows; r++)
    {
        vcell = &v_desc->cells[r][0];

        for(c = 0; c < v_desc->cols; c++)
        {
            VCELL_SET_CHAR((*vcell), 'E');
            VCELL_SET_ATTR((*vcell), A_NORMAL);
            VCELL_SET_DEFAULT_COLORS((*vcell), v_desc);

            vcell++;
        }
    }

    return;
}

