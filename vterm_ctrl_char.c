#include <string.h>

#include "vterm.h"
#include "vterm_private.h"
#include "vterm_buffer.h"
#include "vterm_ctrl_char.h"
#include "vterm_escape.h"

void
vterm_interpret_ctrl_char(vterm_t *vterm, char c)
{
    vterm_desc_t    *v_desc = NULL;
    int             tab_sz;
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
            vterm_scroll_down(vterm, FALSE);
            break;
        }

        // backspace
        case '\b':
        {
            if(v_desc->ccol > 0) v_desc->ccol--;
            break;
        }

        // tab
        case '\t':
        {
            tab_sz = 8 - (v_desc->ccol % 8);
            v_desc->ccol += tab_sz;
            break;
        }

        // vertical tab (new line, col 0)
        case '\x0B':
        {
            vterm_scroll_down(vterm, FALSE);
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

