#include <string.h>

#include "macros.h"
#include "vtty.h"
#include "vtty_private.h"
#include "vtty_buffer.h"
#include "vtty_ctrl_char.h"
#include "vtty_cursor.h"
#include "vtty_escape.h"
#include "vtty_error.h"

void
vtty_interpret_ctrl_char(vtty_t *vtty, char *data)
{
    vtty_desc_t     *v_desc = NULL;
    int             idx;
    char            verb;
    static void     *ctrl_table[] =
                        {
                            [0]         = &&ctrl_char_DEFAULT,
                            ['\r']      = &&ctrl_char_CR,
                            ['\n']      = &&ctrl_char_NL,
                            ['\b']      = &&ctrl_char_BACKSPACE,
                            ['\t']      = &&ctrl_char_TAB,
                            ['\x0B']    = &&ctrl_char_VTAB,
                            ['\x1B']    = &&ctrl_char_ESC_ON,
                            ['\x0E']    = &&ctrl_char_ACS_ON,
                            ['\x0F']    = &&ctrl_char_ACS_OFF,
                            ['\x18']    = &&ctrl_char_BELL,
                            ['\a']      = &&ctrl_char_BELL,
                        };

    // active buffer descriptor (idx kept for VTTY_BUF_ALTERNATE compare below)
    idx = vtty->vtty_desc_idx;
    v_desc = vtty->v_desc_active;

    verb = data[0];

    SWITCH(ctrl_table, (unsigned int)verb, 0);

    ctrl_char_CR:
        // carriage return 
        v_desc->ccol = 0;
        return;

    ctrl_char_NL:
        // line-feed
        if(idx == VTTY_BUF_ALTERNATE)
        {
            // this behavior ssems to work better on ALT buffer
            vtty_scroll_up(vtty, FALSE);
        }
        else
        {
            // this behavoir seems to work better on STD buffer
            vtty_scroll_up(vtty, TRUE);
        }
        return;

    ctrl_char_BACKSPACE:
        // backspace
        vtty_cursor_move_backward(vtty);
        return;

    ctrl_char_TAB:
        // tab
        vtty_cursor_move_tab(vtty);
        return;

    ctrl_char_VTAB:
        // vertical tab (new line, col 0)
        vtty_scroll_up(vtty, FALSE);
        if(v_desc->buffer_state & STATE_AUTOMATIC_LF)
        {
            v_desc->ccol = 0;
        }
        return;

    ctrl_char_ESC_ON:
        // begin escape sequence (aborting previous one if any)
        vtty_escape_start(vtty);
        return;

    ctrl_char_ACS_ON:
        // enter graphical character mode
        vtty->internal_state |= STATE_ALT_CHARSET;
        return;

    ctrl_char_ACS_OFF:
        // exit graphical character mode
        vtty->internal_state &= ~STATE_ALT_CHARSET;
        return;

    ctrl_char_BELL:
        beep();
        return;

    ctrl_char_DEFAULT:
#ifdef DEBUG
        vtty_error(vtty, VTTY_ECODE_UNHANDLED_CTRL_CHAR, (void *)&c)
#endif

    return;
}

