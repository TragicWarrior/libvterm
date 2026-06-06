
#include <string.h>

#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_buffer.h"


/*
    Interpret a 'scroll down' sequence (SD)

    ESC [ n T

    n = number of lines to scroll
*/

void
interpret_csi_SD(vterm_t *vterm, int param[], int pcount)
{
    vterm_desc_t    *v_desc = NULL;
    int             r;
    int             n = 1;          // number of scroll lines
    int             bottom_row;
    int             top_row;
    int             stride;
    int             c;

    // set selector for buffer description
    v_desc = vterm->v_desc_active;

    if(pcount > 0)
    {
        if(param[0] > 0) n = param[0];
    }

    // safety check
    if(n < 1) return;

    /*
        n can't exceed the height of the scroll region; scrolling by more
        than that simply clears the whole region.  clamping here keeps the
        clear loop below (which runs from scroll_min upward by n rows) from
        writing past scroll_max -- and therefore past cells[].
    */
    if(n > v_desc->scroll_max - v_desc->scroll_min + 1)
        n = v_desc->scroll_max - v_desc->scroll_min + 1;

    top_row = v_desc->scroll_min + (n - 1);
    bottom_row = v_desc->scroll_max;
    stride = sizeof(vterm_cell_t) * v_desc->cols;

    for(r = bottom_row; r > top_row; r--)
    {
        memcpy(v_desc->cells[r], v_desc->cells[r - n], stride);

        VCELL_DIRTY_SET_ROW(v_desc, r);
    }

    top_row = v_desc->scroll_min;
    bottom_row = top_row + (n - 1);

    for(r = top_row; r < bottom_row + 1; r++)
    {
        for(c = 0; c < v_desc->cols; c++)
        {
            VCELL_SET_CHAR(v_desc, r, c, ' ');
            VCELL_SET_ATTR(v_desc, r, c, v_desc->curattr);
            VCELL_SET_COLORS(v_desc, r, c);
        }
    }

    return;
}

