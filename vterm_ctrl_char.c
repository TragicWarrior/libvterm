#include <string.h>

#include "macros.h"
#include "vterm.h"
#include "vterm_private.h"
#include "vterm_buffer.h"
#include "vterm_ctrl_char.h"
#include "vterm_cursor.h"
#include "vterm_escape.h"
#include "vterm_error.h"

void
vterm_interpret_ctrl_char(vterm_t *vterm, char *data)
{
    vterm_desc_t    *v_desc = NULL;
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

    // set vterm desc buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    verb = data[0];

    SWITCH(ctrl_table, (unsigned int)verb, 0);

    ctrl_char_CR:
        // carriage return 
        v_desc->ccol = 0;
        return;

    ctrl_char_NL:
        // line-feed
        if(idx == VTERM_BUF_ALTERNATE)
        {
            // this behavior ssems to work better on ALT buffer
            vterm_scroll_up(vterm, FALSE);
        }
        else
        {
            // this behavoir seems to work better on STD buffer
            vterm_scroll_up(vterm, TRUE);
        }
        return;

    ctrl_char_BACKSPACE:
        // backspace
        vterm_cursor_move_backward(vterm);
        return;

    ctrl_char_TAB:
        // tab
        vterm_cursor_move_tab(vterm);
        return;

    ctrl_char_VTAB:
        // vertical tab (new line, col 0)
        vterm_scroll_up(vterm, FALSE);
        if(v_desc->buffer_state & STATE_AUTOMATIC_LF)
        {
            v_desc->ccol = 0;
        }
        return;

    ctrl_char_ESC_ON:
        // begin escape sequence (aborting previous one if any)
        vterm_escape_start(vterm);
        return;

    ctrl_char_ACS_ON:
        // enter graphical character mode
        vterm->internal_state |= STATE_ALT_CHARSET;
        return;

    ctrl_char_ACS_OFF:
        // exit graphical character mode
        vterm->internal_state &= ~STATE_ALT_CHARSET;
        return;

    ctrl_char_BELL:
        beep();
        return;

    ctrl_char_DEFAULT:
#ifdef DEBUG
        vterm_error(vterm, VTERM_ECODE_UNHANDLED_CTRL_CHAR, (void *)&c)
#endif

    return;
}

