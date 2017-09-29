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

/* interprets a 'set attribute' (SGR) CSI escape sequence */
void
interpret_csi_SGR(vterm_t *vterm, int param[], int pcount)
{
    int         nested_params[MAX_CSI_ES_PARAMS];
    int         i;
    short       colors;
    static int  depth = 0;

    // this depth counter prevents a recursion bomb.  depth limit is arbitary.
    if (depth > 6) return;

    if(pcount == 0)
    {
        vterm->curattr = A_NORMAL;                      // reset attributes
        return;
    }

    for(i = 0;i < pcount;i++)
    {
        if(param[i] == 0)                               // reset attributes
        {
            vterm->curattr = A_NORMAL;

            // attribute reset is an implicit color reset too so we'll
            // do a nested call to handle it.
            nested_params[0] = 39;
            nested_params[1] = 49;

            depth++;

            interpret_csi_SGR(vterm, nested_params, 2);
            depth--;

            continue;
        }

        if(param[i]==1 || param[i]==2 || param[i]==4)       // bold on
        {
            vterm->curattr |= A_BOLD;
            continue;
        }

        if(param[i] == 5)                                   // blink on
        {
            vterm->curattr |= A_BLINK;
            continue;
        }

        if(param[i] == 7)                                   // reverse on
        {
            vterm->curattr |= A_REVERSE;
            continue;
        }

        if(param[i] == 27)
        {
            vterm->curattr &= ~(A_REVERSE);
            continue;
        }

        if(param[i] == 8)                                 // invisible on
        {
            vterm->curattr |= A_INVIS;
            continue;
        }

        if(param[i] == 10)                                // rmacs
        {
            vterm->state &= ~STATE_ALT_CHARSET;
            continue;
        }

		if(param[i] == 11)                                // smacs
        {
            vterm->state |= STATE_ALT_CHARSET;
            continue;
        }

        if(param[i] == 22 || param[i] == 24)                // bold off
        {
            vterm->curattr &= ~A_BOLD;
            continue;
        }

        if(param[i] == 25)                                // blink off
        {
            vterm->curattr &= ~A_BLINK;
            continue;
        }

        if(param[i] == 28)                                // invisible off
        {
            vterm->curattr &= ~A_INVIS;
            continue;
        }

        if(param[i] >= 30 && param[i] <= 37)            // set fg color
        {
            int  attr_saved = 0;

            vterm->fg = param[i] - 30;

            if( vterm->flags & VTERM_FLAG_NOCURSES ) // no ncurses
                colors = FindColorPair( vterm->fg, vterm->bg );
            else
                colors = find_color_pair(vterm, vterm->fg, vterm->bg);

            if(colors == -1)
                colors = 0;

            /*
                changing color turns off A_BOLD but not A_REVERSE so we need
                to preserve that attr.
            */
            if(vterm->curattr & A_REVERSE) attr_saved |= A_REVERSE;

            vterm->curattr = 0;
            vterm->curattr |= COLOR_PAIR(colors);
            vterm->curattr |= attr_saved;

            continue;
        }

        if(param[i] >= 40 && param[i] <= 47)            // set bg color
        {
            int  attr_saved = 0;

            vterm->bg = param[i]-40;
            if( vterm->flags & VTERM_FLAG_NOCURSES ) // no ncurses
                colors = FindColorPair( vterm->fg, vterm->bg );
            else
                colors = find_color_pair(vterm, vterm->fg, vterm->bg);

            if(colors == -1)
                colors = 0;

            /*
                changing color turns off A_BOLD but not A_REVERSE so we need
                to preserve that attr.
            */
            if(vterm->curattr & A_REVERSE) attr_saved |= A_REVERSE;

            vterm->curattr = 0;
            vterm->curattr |= COLOR_PAIR(colors);
            vterm->curattr |= attr_saved;

            continue;
        }

        if(param[i] == 39)                                // reset fg color
        {
            int  attr_saved = 0;

            if( vterm->flags & VTERM_FLAG_NOCURSES ) // no ncurses
            {
                int fg, bg;
                if( GetFGBGFromColorIndex( vterm->colors, &fg, &bg )==0 )
                {
                    vterm->fg = fg;
                    colors = FindColorPair( vterm->fg, vterm->bg );
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
                short default_fg,default_bg;
                pair_content(vterm->colors, &default_fg, &default_bg);
                vterm->fg = default_fg;
                colors = find_color_pair(vterm, vterm->fg, vterm->bg);
#endif
            }
            if(colors == -1) colors = 0;

            if (vterm->curattr & A_BOLD) attr_saved |= A_BOLD;
            if(vterm->curattr & A_REVERSE) attr_saved |= A_REVERSE;

            vterm->curattr = 0;
            vterm->curattr |= COLOR_PAIR(colors);
            vterm->curattr |= attr_saved;

            continue;
        }

        if(param[i] == 49)                                // reset bg color
        {
            int  attr_saved = 0;

            if(vterm->flags & VTERM_FLAG_NOCURSES) // no ncurses
            {
                int fg, bg;
                if( GetFGBGFromColorIndex( vterm->colors, &fg, &bg )==0 )
                {
                    vterm->bg = bg;
                    colors = FindColorPair( vterm->fg, vterm->bg );
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
                pair_content(vterm->colors, &default_fg, &default_bg);
                vterm->bg = default_bg;
                colors = find_color_pair(vterm, vterm->fg,vterm->bg);
#endif
            }
            if(colors == -1) colors = 0;

            if (vterm->curattr & A_BOLD) attr_saved |= A_BOLD;
            if (vterm->curattr & A_REVERSE) attr_saved |= A_REVERSE;

            vterm->curattr = 0;
            vterm->curattr |= COLOR_PAIR(colors);
            vterm->curattr |= attr_saved;

            continue;
        }
    }
}
