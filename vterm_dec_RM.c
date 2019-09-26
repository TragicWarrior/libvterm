
#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_buffer.h"
#include "vterm_cursor.h"
#include "mouse_driver.h"

/*
    Interpret the DEC RM / DEC PRIVATE RM (reset mode)

    ESC [ ? x l

*/
void
interpret_dec_RM(vterm_t *vterm, int param[], int pcount)
{
    vterm_desc_t    *v_desc = NULL;
    int             i;
    int             idx;

    if(pcount == 0) return;

    // set vterm desc buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    for(i = 0; i < pcount; i++)
    {
        /*
            DECCOLM

            restore to 80 column mode.  it also erases the screen
            which is all we're going to do.
        */
        if(param[i] == 3)
        {
            vterm_erase(vterm, idx, 0);
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
            vterm_cursor_move_home(vterm);
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
            vterm_cursor_hide(vterm, idx);
            continue;
        }

        // restore standard buffer
        // CSI hanlder:  ESC [ ? 47 l
        if(param[i] == 47)
        {
            // check to see if we're already using the ALT buffer
            if(idx == VTERM_BUF_STANDARD) continue;

            vterm_buffer_set_active(vterm, VTERM_BUF_STANDARD);
            continue;
        }

        // shutdown whatever mouse driver is installed
        if(param[i] == 1000 || param[i] == 1005 || param[i] == 1006)
        {
            mouse_driver_stop(vterm);
            continue;
        }

        /*
            turn off alternate scroll mode.  in alternate scroll mode
            the wheel mouse sends cursor up and cursor down movements
            instead of button events.
        */
        if(param[i] == 1007)
        {
            vterm->mouse_mode &= ~VTERM_MOUSE_ALTSCROLL;
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
            if(idx != VTERM_BUF_STANDARD)
            {
                vterm_buffer_set_active(vterm, VTERM_BUF_STANDARD);
            }

            vterm_cursor_restore(vterm);

            continue;
        }

        // stub for turning off bracked paste
        if(param[i] == 2004)
        {
            // todo
            continue;
        }
    }
}
