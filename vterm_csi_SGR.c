/*
Copyright (C) 2009 Bryan Christ

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/*
This library is based on ROTE written by Bruno Takahashi C. de Oliveira
*/

/*
   VT100 SGR documentation
   From http://vt100.net/docs/vt510-rm/SGR table 5-16
   0  All attributes off
   1  Bold
   4  Underline
   5  Blinking
   7  Negative image
   8  Invisible image
   10    The ASCII character set is the current 7-bit
         display character set (default) - SCO Console only.
   11    Map Hex 00-7F of the PC character set codes
         to the current 7-bit display character set
         - SCO Console only.
   12    Map Hex 80-FF of the current character set to
         the current 7-bit display character set - SCO
         Console only.
   22    Bold off
   24    Underline off
   25    Blinking off
   27    Negative image off
   28    Invisible image off
*/

#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_colors.h"
#include "vterm_buffer.h"

/* interprets a 'set attribute' (SGR) CSI escape sequence */
void
interpret_csi_SGR(vterm_t *vterm, int param[], int pcount)
{
    vterm_desc_t    *v_desc = NULL;
    int             nested_params[MAX_CSI_ES_PARAMS];
    int             i;
    short           colors;
    static int      depth = 0;
    int             idx;

    // this depth counter prevents a recursion bomb.  depth limit is arbitary.
    if (depth > 6) return;

    // set vterm_desc buffer selector
    idx = vterm_get_active_buffer(vterm);
    v_desc = &vterm->vterm_desc[idx];

    if(pcount == 0)
    {
        v_desc->curattr = A_NORMAL;                      // reset attributes
        return;
    }

    for(i = 0;i < pcount;i++)
    {
        if(param[i] == 0)                               // reset attributes
        {
            v_desc->curattr = A_NORMAL;

            // attribute reset is an implicit color reset too so we'll
            // do a nested call to handle it.
            nested_params[0] = 39;
            nested_params[1] = 49;

            depth++;

            interpret_csi_SGR(vterm, nested_params, 2);
            depth--;

            continue;
        }

        if(param[i] == 1 || param[i] == 2 || param[i] == 4)       // bold on
        {
            v_desc->curattr |= A_BOLD;
            continue;
        }

        if(param[i] == 5)                                   // blink on
        {
            v_desc->curattr |= A_BLINK;
            continue;
        }

        if(param[i] == 7)                                   // reverse on
        {
            v_desc->curattr |= A_REVERSE;
            continue;
        }

        if(param[i] == 27)
        {
            v_desc->curattr &= ~(A_REVERSE);
            continue;
        }

        if(param[i] == 8)                                 // invisible on
        {
            v_desc->curattr |= A_INVIS;
            continue;
        }

        if(param[i] == 10)                                // rmacs
        {
            // v_desc->buffer_state &= ~STATE_ALT_CHARSET;
            vterm->internal_state &= ~STATE_ALT_CHARSET;
            continue;
        }

		if(param[i] == 11)                                // smacs
        {
            // v_desc->buffer_state |= STATE_ALT_CHARSET;
            vterm->internal_state |= STATE_ALT_CHARSET;
            continue;
        }

        if(param[i] == 22 || param[i] == 24)                // bold off
        {
            v_desc->curattr &= ~A_BOLD;
            continue;
        }

        if(param[i] == 25)                                // blink off
        {
            v_desc->curattr &= ~A_BLINK;
            continue;
        }

        if(param[i] == 28)                                // invisible off
        {
            v_desc->curattr &= ~A_INVIS;
            continue;
        }

        if(param[i] >= 30 && param[i] <= 37)            // set fg color
        {
            int  attr_saved = 0;

            v_desc->fg = param[i] - 30;

            if( vterm->flags & VTERM_FLAG_NOCURSES ) // no ncurses
                colors = FindColorPair(v_desc->fg, v_desc->bg );
            else
                colors = find_color_pair(vterm, v_desc->fg, v_desc->bg);

            if(colors == -1)
                colors = 0;

            /*
                the COLOR_PAIR macros seems to trample attributes.
                save them before making changes and OR them back in.
            */
            if(v_desc->curattr & A_REVERSE) attr_saved |= A_REVERSE;
            if(v_desc->curattr & A_BOLD) attr_saved |= A_BOLD;

            v_desc->curattr = 0;
            v_desc->curattr |= COLOR_PAIR(colors);
            v_desc->curattr |= attr_saved;

            continue;
        }

        if(param[i] >= 40 && param[i] <= 47)            // set bg color
        {
            int  attr_saved = 0;

            v_desc->bg = param[i]-40;
            if(vterm->flags & VTERM_FLAG_NOCURSES) // no ncurses
                colors = FindColorPair(v_desc->fg, v_desc->bg);
            else
                colors = find_color_pair(vterm, v_desc->fg, v_desc->bg);

            if(colors == -1)
                colors = 0;

            /*
                the COLOR_PAIR macros seems to trample attributes.
                save them before making changes and OR them back in.
            */
            if(v_desc->curattr & A_REVERSE) attr_saved |= A_REVERSE;
            if(v_desc->curattr & A_BOLD) attr_saved |= A_BOLD;

            v_desc->curattr = 0;
            v_desc->curattr |= COLOR_PAIR(colors);
            v_desc->curattr |= attr_saved;

            continue;
        }

        if(param[i] == 39)                                // reset fg color
        {
            int  attr_saved = 0;

            if(vterm->flags & VTERM_FLAG_NOCURSES) // no ncurses
            {
                int fg, bg;
                if(GetFGBGFromColorIndex(v_desc->colors, &fg, &bg )==0 )
                {
                    v_desc->fg = fg;
                    colors = FindColorPair(v_desc->fg, v_desc->bg );
                }
                else
                {
                    colors = -1;
                }
            }
            else
            {
#ifdef NOCURSES
                // should not ever execute - bad combination of flags and
                // #define's.
                colors = -1;
#else
                short default_fg, default_bg;
                pair_content(v_desc->colors, &default_fg, &default_bg);
                v_desc->fg = default_fg;
                colors = find_color_pair(vterm, v_desc->fg, v_desc->bg);
#endif
            }
            if(colors == -1) colors = 0;

            if(v_desc->curattr & A_BOLD) attr_saved |= A_BOLD;
            if(v_desc->curattr & A_REVERSE) attr_saved |= A_REVERSE;

            v_desc->curattr = 0;
            v_desc->curattr |= COLOR_PAIR(colors);
            v_desc->curattr |= attr_saved;

            continue;
        }

        if(param[i] == 49)                                // reset bg color
        {
            int  attr_saved = 0;

            if(vterm->flags & VTERM_FLAG_NOCURSES) // no ncurses
            {
                int fg, bg;
                if(GetFGBGFromColorIndex(v_desc->colors, &fg, &bg) == 0)
                {
                    v_desc->bg = bg;
                    colors = FindColorPair(v_desc->fg, v_desc->bg );
                }
                else
                {
                    colors = -1;
                }
            }
            else
            {
#ifdef NOCURSES
                // should not ever execute - bad combination of flags and
                // #define's.
                colors = -1;
#else
                short default_fg, default_bg;
                pair_content(v_desc->colors, &default_fg, &default_bg);
                v_desc->bg = default_bg;
                colors = find_color_pair(vterm, v_desc->fg, v_desc->bg);
#endif
            }
            if(colors == -1) colors = 0;

            if (v_desc->curattr & A_BOLD) attr_saved |= A_BOLD;
            if (v_desc->curattr & A_REVERSE) attr_saved |= A_REVERSE;

            v_desc->curattr = 0;
            v_desc->curattr |= COLOR_PAIR(colors);
            v_desc->curattr |= attr_saved;

            continue;
        }
    }
}
