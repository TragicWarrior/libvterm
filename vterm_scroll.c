
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

    // set vterm desc buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    /*
        on scroll up, copy a row to the history buffer, but only if the
        active buffer is the standard buffer.
    */
    if(idx == VTERM_BUF_STANDARD)
    {
        vterm_buffer_shift_up(vterm, VTERM_BUF_HISTORY, -1, -1, 1);

        vterm_buffer_clone(vterm, VTERM_BUF_STANDARD, VTERM_BUF_HISTORY,
            vterm->vterm_desc[VTERM_BUF_STANDARD].crow,
            vterm->vterm_desc[VTERM_BUF_HISTORY].rows - 1, 1);
    }

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

    vterm_buffer_shift_up(vterm, idx,
        v_desc->scroll_min, v_desc->scroll_max, 1);

    /* clear last row of the scrolling region */
    vterm_erase_row(vterm, v_desc->scroll_max, reset_colors, 0);

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

    vterm_buffer_shift_down(vterm, idx,
        v_desc->scroll_min, v_desc->scroll_max, 1);

    // clear first row of the scrolling region
    vterm_erase_row(vterm, v_desc->scroll_min, reset_colors, 0);

    // a scroll makes all rows dirty
    VCELL_ALL_SET_DIRTY(v_desc);

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
