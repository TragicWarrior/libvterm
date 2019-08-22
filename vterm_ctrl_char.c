#include <string.h>

#include "vterm.h"
#include "vterm_private.h"
#include "vterm_buffer.h"
#include "vterm_ctrl_char.h"
#include "vterm_cursor.h"
#include "vterm_escape.h"

void
vterm_interpret_ctrl_char(vterm_t *vterm, char c)
{
    vterm_desc_t    *v_desc = NULL;
    int             idx;

    // set vterm desc buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    switch(c)
    {
        // carriage return
        case '\r':
        {
            v_desc->ccol = 0;
            break;
        }

        // line-feed
        case '\n':
        {
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
            break;
        }

        // backspace
        case '\b':
        {
            // endwin(); printf("%d\n\r", v_desc->ccol); fflush(stdout); exit(0);
            // printf("%d %ju\n\r", v_desc->ccol, v_desc);
            vterm_cursor_move_backward(vterm);
            break;
        }

        // tab
        case '\t':
        {
            vterm_cursor_move_tab(vterm);
            break;
        }

        // vertical tab (new line, col 0)
        case '\x0B':
        {
            vterm_scroll_up(vterm, FALSE);
            if(v_desc->buffer_state & STATE_AUTOMATIC_LF)
            {
                v_desc->ccol = 0;
            }
            break;
        }

        // begin escape sequence (aborting previous one if any)
        case '\x1B':
        {
            vterm_escape_start(vterm);
            break;
        }

        // enter graphical character mode
        case '\x0E':
        {
            vterm->internal_state |= STATE_ALT_CHARSET;
            break;
        }

        // exit graphical character mode
        case '\x0F':
        {
            vterm->internal_state &= ~STATE_ALT_CHARSET;
            break;
        }

        // these interrupt escape sequences
        case '\x18':

        // bell
        case '\a':
        {
#ifndef NOCURSES
            beep();
#endif
            break;
        }

#ifdef DEBUG
      default:
         fprintf(stderr, "Unrecognized control char: %d (^%c)\n", c, c + '@');
         break;
#endif
   }
}

