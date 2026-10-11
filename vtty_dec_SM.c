
#include "vtty.h"
#include "vtty_private.h"
#include "vtty_csi.h"
#include "vtty_buffer.h"
#include "vtty_cursor.h"
#include "mouse_driver.h"

/*
    Info on bracketed paste can be found here:

    ESC [ ? x h

    https://cirw.in/blog/bracketed-paste
*/

/* Interpret DEC SM (set mode) */
void
interpret_dec_SM(vtty_t *vtty, int param[], int pcount)
{
    vtty_desc_t     *v_desc = NULL;
    int             i;
    int             idx;

    if(pcount == 0) return;

    // active buffer descriptor (idx kept for cursor_show / VTTY_BUF_ALTERNATE compares)
    idx = vtty->vtty_desc_idx;
    v_desc = vtty->v_desc_active;

    for(i = 0; i < pcount; i++)
    {
        // DECCKM: application cursor keys
        if(param[i] == 1)
        {
            vtty->internal_state |= STATE_CURSOR_APP;
            continue;
        }

        /*
            DECCOLM

            set 132 column mode.  it also erases the screen which is
            all we're going to do.
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
            v_desc->buffer_state |= STATE_ORIGIN_MODE;
            vtty_cursor_move_home(vtty);
            continue;
        }

        // enable auto-wrap mode (default is auto-wrap)
        if(param[i] == 7)
        {
            v_desc->buffer_state &= ~STATE_NO_WRAP;
            continue;
        }

        if(param[i] == 20)
        {
            v_desc->buffer_state &= STATE_AUTOMATIC_LF;
            continue;
        }

        /* civis is actually "normal" for rxvt */
        if(param[i] == 25)
        {
            vtty_cursor_show(vtty, idx);
            continue;
        }

        // start alt buffer
        // CSI handler:  ESC [ ? 47 h
        if(param[i] == 47)
        {
            // check to see if we're already using the ALT buffer
            if(idx == VTTY_BUF_ALTERNATE) continue;

            vtty_buffer_set_active(vtty, VTTY_BUF_ALTERNATE);
            continue;
        }

        /*
            set mouse mode VT200 and installs the generic mouse driver
            this is a varation of the X10 protocol with tracking
        */
        if(param[i] == 1000)
        {
            mouse_driver_init(vtty);
            mouse_driver_start(vtty);

            vtty->mouse_mode |= VTTY_MOUSE_VT200;
            vtty->mouse_driver = mouse_driver_default;
            continue;
        }

        if(param[i] == 1001)
        {
            vtty->mouse_mode |= VTTY_MOUSE_HIGHLIGHT;
            continue;
        }

        /*
            button-event (1002) and any-event (1003) tracking.  both
            report presses and releases as mode 1000 does;  1002 adds
            motion while a button is held (drags) and 1003 adds all
            motion.
        */
        if(param[i] == 1002 || param[i] == 1003)
        {
            mouse_driver_init(vtty);
            mouse_driver_start(vtty);

            vtty->mouse_mode |= VTTY_MOUSE_VT200;
            vtty->mouse_driver = mouse_driver_default;

            mouse_driver_set_motion(vtty, (param[i] == 1002) ?
                VTTY_MOUSE_BTN_EVENT : VTTY_MOUSE_ANY_EVENT);
            continue;
        }

        /*
            set mouse mode SGR and installs the generic mouse driver
            the most ubiquitous mouse mode handling > 223 x 223 coord
        */
        if(param[i] == 1006)
        {
            mouse_driver_init(vtty);
            mouse_driver_start(vtty);

            vtty->mouse_mode |= VTTY_MOUSE_SGR;
            vtty->mouse_driver = mouse_driver_default;
            continue;
        }

        /*
            turn on alternate scroll mode.  this causes cursor up and
            cursor down to be emitted instead of the actual button
            values when the wheel mouse is used.
        */
        if(param[i] == 1007)
        {
            vtty->mouse_mode |= VTTY_MOUSE_ALTSCROLL;
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
            meta(NULL, TRUE);

            return;
        }

        /*
            nearly identical to ESC [ ? 47 h except it calls for
            saving the cursor first then switching to alt buffer.
        */
        if(param[i] == 1049)
        {
            vtty_cursor_save(vtty);

            // check to see if we're already using the ALT buffer
            if(idx != VTTY_BUF_ALTERNATE)
                vtty_buffer_set_active(vtty, VTTY_BUF_ALTERNATE);

            continue;
        }

        // DECSET 2004 -- bracketed paste (wrap bulk pastes in ESC[200~..201~)
        if(param[i] == 2004)
        {
            vtty->internal_state |= STATE_BRACKETED_PASTE;
            continue;
        }
    }
}
