
#include <string.h>
#include <fcntl.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_buffer.h"

void
vterm_scroll_down(vterm_t *vterm)
{
    vterm_desc_t    *v_desc = NULL;
    int             idx;
    int             i;

    // set vterm desc buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    v_desc->crow++;

    if(v_desc->crow <= v_desc->scroll_max) return;

    /*
        must scroll the scrolling region up by 1 line, and put cursor on
        last line of it
    */
    v_desc->crow = v_desc->scroll_max;

    for(i = v_desc->scroll_min; i < v_desc->scroll_max; i++)
    {
        memcpy(v_desc->cells[i], v_desc->cells[i + 1],
            sizeof(vterm_cell_t) * v_desc->cols);
    }

    /* clear last row of the scrolling region */
    vterm_erase_row(vterm, v_desc->scroll_max);

    return;
}

void
vterm_scroll_up(vterm_t *vterm)
{
    vterm_desc_t    *v_desc = NULL;
    int             idx;
    int             i;

    // set vterm desc buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    v_desc->crow--;

    if(v_desc->crow >= v_desc->scroll_min) return;

    /*
        must scroll the scrolling region up by 1 line, and put cursor on
        first line of it
    */
    v_desc->crow = v_desc->scroll_min;

    for(i = v_desc->scroll_max; i > v_desc->scroll_min; i--)
    {
        memcpy(v_desc->cells[i], v_desc->cells[i - 1],
            sizeof(vterm_cell_t) * v_desc->cols);
    }

    /* clear first row of the scrolling region */
    vterm_erase_row(vterm, v_desc->scroll_min);

    return;
}
