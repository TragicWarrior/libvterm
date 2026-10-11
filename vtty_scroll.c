
#include <string.h>
#include <fcntl.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>

#ifdef __FreeBSD__
#include <ncurses/ncurses.h>
#endif

#include "vtty.h"
#include "vtty_private.h"
#include "vtty_csi.h"
#include "vtty_buffer.h"

void
vtty_scroll_up(vtty_t *vtty, bool reset_colors)
{
    vtty_desc_t     *v_desc = NULL;
    int             idx;

    // active buffer descriptor (idx kept for compares / shift_up,down calls)
    idx = vtty->vtty_desc_idx;
    v_desc = vtty->v_desc_active;

    v_desc->crow++;

    /*
        we just moved the cursor down a row.  if we haven't hit the bottom
        edge yet, then we're done -- no row was lost, so nothing belongs
        in the scrollback history.  (the prior implementation cloned the
        cursor's row to history unconditionally here, which meant every
        \n added a duplicate of still-visible content -- visible as a
        stack of identical UI redraws when scrolling back through an app
        like Claude Code that repaints its prompt in place several times
        per second.)
    */
    if(v_desc->crow <= v_desc->scroll_max) return;

    /*
        scroll-region overflow: the cursor wanted to advance past
        scroll_max, so we have to shift the region up and surrender the
        top row.  pin the cursor to the last row of the region.
    */
    v_desc->crow = v_desc->scroll_max;

    /*
        push the row that's about to disappear into scrollback, but only
        for the standard buffer (alt screen is intentionally not
        captured) and only when scroll_min == 0 (a DECSTBM region with a
        top margin is scrolling internally, not off the actual top of
        the screen, so its evictions don't belong in history -- matches
        xterm convention).  The append runs BEFORE the shift so we
        capture the row at its true source position.
    */
    if(idx == VTTY_BUF_STANDARD && v_desc->scroll_min == 0)
    {
        vtty_buffer_history_append(vtty, VTTY_BUF_STANDARD,
            v_desc->scroll_min);
    }

    vtty_buffer_shift_up(vtty, idx,
        v_desc->scroll_min, v_desc->scroll_max, 1);

    /* clear last row of the scrolling region */
    vtty_erase_row(vtty, v_desc->scroll_max, reset_colors, 0);

    if(vtty->event_mask & VTTY_MASK_TERM_SCROLLED)
    {
        if(vtty->event_hook != NULL)
        {
            vtty->event_hook(vtty, VTTY_EVENT_TERM_SCROLLED,
                &(int){-1});
        }
    }

    return;
}

void
vtty_scroll_down(vtty_t *vtty, bool reset_colors)
{
    vtty_desc_t     *v_desc = NULL;
    int             idx;

    // active buffer descriptor (idx kept for compares / shift_up,down calls)
    idx = vtty->vtty_desc_idx;
    v_desc = vtty->v_desc_active;

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

    vtty_buffer_shift_down(vtty, idx,
        v_desc->scroll_min, v_desc->scroll_max, 1);

    // clear first row of the scrolling region
    vtty_erase_row(vtty, v_desc->scroll_min, reset_colors, 0);

    // a scroll makes all rows dirty
    VCELL_ALL_SET_DIRTY(v_desc);

    if(vtty->event_mask & VTTY_MASK_TERM_SCROLLED)
    {
        if(vtty->event_hook != NULL)
        {
            vtty->event_hook(vtty, VTTY_EVENT_TERM_SCROLLED,
                &(int){1});
        }
    }

    return;
}
