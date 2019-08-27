
#include <string.h>
#include <fcntl.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>

#ifdef __FreeBSD__
#include <ncurses/ncurses.h>
#endif

#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_buffer.h"

void
vterm_scroll_up(vterm_t *vterm, bool reset_colors)
{
    vterm_desc_t    *v_desc = NULL;
    int             idx;
    int             i;
    vterm_cell_t    **vcell_src;
    vterm_cell_t    **vcell_dst;

    // set vterm desc buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    v_desc->crow++;

    /*
        we just moved the cursor down a row.  if we haven't hit the bottom
        edge yet, then we're done.
    */
    if(v_desc->crow <= v_desc->scroll_max) return;

    /*
        must scroll the scrolling region up by 1 line, and put cursor on
        last line of it
    */
    v_desc->crow = v_desc->scroll_max;

    vcell_src = &v_desc->cells[v_desc->scroll_min + 1];
    vcell_dst = &v_desc->cells[v_desc->scroll_min];

    for(i = v_desc->scroll_min; i < v_desc->scroll_max; i++)
    {
        memcpy(*vcell_dst, *vcell_src,
             sizeof(vterm_cell_t) * v_desc->cols);

        vcell_src++;
        vcell_dst++;
    }

    /* clear last row of the scrolling region */
    vterm_erase_row(vterm, v_desc->scroll_max, reset_colors);

    if(vterm->event_mask & VTERM_MASK_TERM_SCROLLED)
    {
        if(vterm->event_hook != NULL)
        {
            vterm->event_hook(vterm, VTERM_EVENT_TERM_SCROLLED,
                &(int){-1});
        }
    }

    return;
}

void
vterm_scroll_down(vterm_t *vterm, bool reset_colors)
{
    vterm_desc_t    *v_desc = NULL;
    int             idx;
    int             i;

    // set vterm desc buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    // move the cursor up
    v_desc->crow--;

    /*
        we just moved the cursor up a row.  if we haven't hit the top
        edge yet, then we're done.
    */
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
    vterm_erase_row(vterm, v_desc->scroll_min, reset_colors);

    if(vterm->event_mask & VTERM_MASK_TERM_SCROLLED)
    {
        if(vterm->event_hook != NULL)
        {
            vterm->event_hook(vterm, VTERM_EVENT_TERM_SCROLLED,
                &(int){1});
        }
    }

    return;
}
