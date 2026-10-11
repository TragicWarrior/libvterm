
#include "vtty.h"
#include "vtty_private.h"
#include "vtty_csi.h"
#include "vtty_buffer.h"
#include "vtty_cursor.h"
#include "mouse_driver.h"

/*
    Interpret the DEC RM / DEC PRIVATE RM (reset mode)

    ESC [ ? x l

*/
void
interpret_dec_RM(vtty_t *vtty, int param[], int pcount)
{
    vtty_desc_t     *v_desc = NULL;
    int             i;
    int             idx;

    if(pcount == 0) return;

    // active buffer descriptor (idx kept for VTTY_BUF_* compares below)
    idx = vtty->vtty_desc_idx;
    v_desc = vtty->v_desc_active;

    for(i = 0; i < pcount; i++)
    {
        // DECCKM: normal cursor keys
        if(param[i] == 1)
        {
            vtty->internal_state &= ~STATE_CURSOR_APP;
            continue;
        }

        /*
            DECCOLM

            restore to 80 column mode.  it also erases the screen
            which is all we're going to do.
        */
        if(param[i] == 3)
        {
            vtty_erase(vtty, idx, 0);
            v_desc->ccol = 0;
            v_desc->crow = 0;
            continue;
        }

        /*
            DECOM

            set origin mode.  this means that cursor home position is
            relative to the scrolling region.  it also cause the cursor
            to move to the effective home position.

            we don't support this yet.
        */
        if(param[i] == 6)
        {
            v_desc->buffer_state &= ~STATE_ORIGIN_MODE;
            vtty_cursor_move_home(vtty);
            continue;
        }

        // disable auto-wrap mode
        if(param[i] == 7)
        {
            v_desc->buffer_state |= STATE_NO_WRAP;
            continue;
        }

        if(param[i] == 20)
        {
            v_desc->buffer_state &= ~STATE_AUTOMATIC_LF;
            continue;
        }


        /* civis is actually the "normal" vibility for rxvt   */
        if(param[i] == 25)
        {
            vtty_cursor_hide(vtty, idx);
            continue;
        }

        // restore standard buffer
        // CSI hanlder:  ESC [ ? 47 l
        if(param[i] == 47)
        {
            // check to see if we're already using the ALT buffer
            if(idx == VTTY_BUF_STANDARD) continue;

            vtty_buffer_set_active(vtty, VTTY_BUF_STANDARD);
            continue;
        }

        // shutdown whatever mouse driver is installed
        if(param[i] == 1000 || param[i] == 1002 || param[i] == 1003
            || param[i] == 1005 || param[i] == 1006)
        {
            mouse_driver_stop(vtty);
            continue;
        }

        /*
            turn off alternate scroll mode.  in alternate scroll mode
            the wheel mouse sends cursor up and cursor down movements
            instead of button events.
        */
        if(param[i] == 1007)
        {
            vtty->mouse_mode &= ~VTTY_MOUSE_ALTSCROLL;
            continue;
        }

        /*
            not reall well documented but this code turns on the
            meta key.  see terminfo(5).  we just pass this along
            to ncurses via the meta() function.
        */
        if(param[i] == 1034)
        {
            /*
                according to man pages for meta() "The window
                argument, win, is always ignored."
            */
            meta(NULL, FALSE);

            return;
        }

        /*
            similar to ESC [ ? 47 l except it restores the cursor after
            switching back to standard buffer.
        */
        if(param[i] == 1049)
        {
            if(idx != VTTY_BUF_STANDARD)
            {
                vtty_buffer_set_active(vtty, VTTY_BUF_STANDARD);
            }

            vtty_cursor_restore(vtty);

            continue;
        }

        // DECRST 2004 -- bracketed paste off
        if(param[i] == 2004)
        {
            vtty->internal_state &= ~STATE_BRACKETED_PASTE;
            continue;
        }
    }
}
