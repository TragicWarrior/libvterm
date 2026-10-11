#define _DARWIN_C_SOURCE                // for SIGWINCH on OSX

#include <stdlib.h>
#include <termios.h>
#include <string.h>
#include <signal.h>

#include <sys/ioctl.h>

#include "macros.h"
#include "vtty.h"
#include "vtty_private.h"
#include "vtty_misc.h"
#include "vtty_buffer.h"

void
vtty_resize_full(vtty_t *vtty, uint16_t width, uint16_t height,
                    int grip_top, int grip_left,
                    int grip_bottom, int grip_right)
{
    vtty_desc_t     *v_desc = NULL;
    struct winsize  ws = {.ws_xpixel = 0,.ws_ypixel = 0};
    int             delta_y;
    int             start_y;
    int             idx;

    // suppress warnings
    VAR_UNUSED(grip_top);
    VAR_UNUSED(grip_left);
    VAR_UNUSED(grip_bottom);
    VAR_UNUSED(grip_right);

    if(vtty == NULL) return;
    if(width == 0 || height == 0) return;

    // active buffer descriptor (idx kept for the buffer_realloc call below)
    idx = vtty->vtty_desc_idx;
    v_desc = vtty->v_desc_active;

    delta_y = height - v_desc->rows;
    start_y = v_desc->rows;

    // fire off the TERM RESIZED event hook if it's been set
    if(vtty->event_mask & VTTY_MASK_TERM_PRESIZE)
    {
        if(vtty->event_hook != NULL)
        {
            vtty->event_hook(vtty,
                VTTY_EVENT_TERM_PRESIZE, (void *)NULL);
        }
    }

    // realloc to accomodate the new matrix size
    vtty_buffer_realloc(vtty, idx, width, height);

    /*
        realloc the history buffer to the new width and keep the
        height the same.
    */
    vtty_buffer_realloc(vtty, VTTY_BUF_HISTORY, width, -1);

    if(!(v_desc->buffer_state & STATE_SCROLL_SHORT))
    {
        v_desc->scroll_max = height - 1;
    }

    ws.ws_row = height;
    ws.ws_col = width;

    clamp_cursor_to_bounds(vtty);

    // if(delta_x > 0) vtty_erase_cols(vtty, start_x);
    if(delta_y > 0) vtty_erase_rows(vtty, start_y, 0);

    // signal the child process that terminal changed size
    if(!(vtty->flags & VTTY_FLAG_NOPTY))
    {
        ioctl(vtty->pty_fd, TIOCSWINSZ, &ws);
        kill(vtty->child_pid, SIGWINCH);
    }

    // fire off the TERM RESIZED event hook if it's been set
    if(vtty->event_mask & VTTY_MASK_TERM_RESIZED)
    {
        if(vtty->event_hook != NULL)
        {
            vtty->event_hook(vtty,
                VTTY_EVENT_TERM_RESIZED, (void *)&ws);
        }
    }

    return;
}
