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

#include <string.h>
#include <fcntl.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_escape.h"
#include "vterm_utf8.h"
#include "vterm_buffer.h"
#include "vterm_colors.h"
#include "macros.h"

static void
vterm_render_ctrl_char(vterm_t *vterm,char c);

static void
vterm_put_char(vterm_t *vterm,chtype c);


void
vterm_render(vterm_t *vterm, const char *data, int len)
{
    chtype          utf8_char;
    int             bytes = -1;
    int             i;

    for(i = 0; i < len; i++, data++)
    {
        // completely ignore NUL
        if(*data == 0) continue;

        if(!IS_MODE_ESCAPED(vterm))
        {
            if((unsigned int)*data >= 1 && (unsigned int)*data <= 31)
            {
                vterm_render_ctrl_char(vterm,*data);
                continue;
            }
        }

        // the code points for utf start at 0x80
        if((unsigned int)*data > 0x7F)
        {
            if(!IS_MODE_UTF8(vterm))
            {
                vterm_utf8_start(vterm);
            }
        }

        if(IS_MODE_UTF8(vterm))
        {
            if(vterm->utf8_buf_len >= UTF8_BUF_SIZE)
            {
                vterm_utf8_cancel(vterm);
                continue;
            }

            // append byte to utf-8 buffer
            vterm->utf8_buf[vterm->utf8_buf_len] = *data;

            // increment the buffer length and push out the NULL terminator
            vterm->utf8_buf_len++;
            vterm->utf8_buf[vterm->utf8_buf_len] = 0;

            // we're in UTF-8 mode... do this
            bytes = vterm_utf8_decode(vterm, &utf8_char);

            // we're done
            if (bytes > 0)
            {
                vterm_put_char(vterm, utf8_char);
                vterm_utf8_cancel(vterm);
            }

            continue;
        }

        if(IS_MODE_ESCAPED(vterm))
        {
            if(vterm->esbuf_len >= ESEQ_BUF_SIZE)
            {
                vterm_escape_cancel(vterm);
                return;
            }

            // append character to ongoing escape sequence
            vterm->esbuf[vterm->esbuf_len] = *data;

            // increment the buffer length and push out the NULL terminator
            vterm->esbuf_len++;
            vterm->esbuf[vterm->esbuf_len] = 0;

            // if we are in escape mode (initiated by 0x1B) go here...
            vterm_interpret_escapes(vterm);
        }
        else
        {
            vterm_put_char(vterm, *data);
        }
    }

    return;
}

void
vterm_put_char(vterm_t *vterm, chtype c)
{
    vterm_desc_t    *v_desc = NULL;
    vterm_cell_t    *vcell = NULL;
    static char     vt100_acs[]="`afgjklmnopqrstuvwxyz{|}~,+-.";
    static char     *end = vt100_acs + ARRAY_SZ(vt100_acs);
    char            *pos = NULL;
    int             idx;

    // set vterm desc buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    if(v_desc->ccol >= v_desc->cols)
    {
        v_desc->ccol = 0;
        vterm_scroll_down(vterm);
    }

    /*
        store the location of the cell so we don't have to do
        multiple scalar look-ups later.
    */
    vcell = &v_desc->cells[v_desc->crow][v_desc->ccol];

    if(IS_MODE_ACS(vterm))
    {
        pos = vt100_acs;

        // iternate through ACS looking for matches
        while(pos != end)
        {
            if((char)c == *pos)
            {
                VCELL_SET_CHAR((*vcell), NCURSES_ACS(c));
                memcpy(&vcell->uch, NCURSES_WACS(c), sizeof(cchar_t));
            }
            pos++;
        }
    }
    else
    {
        VCELL_SET_CHAR((*vcell), c);
    }

    VCELL_SET_ATTR((*vcell), v_desc->curattr);

    v_desc->ccol++;

    return;
}

void
vterm_render_ctrl_char(vterm_t *vterm, char c)
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
            vterm_scroll_down(vterm);
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
            while(v_desc->ccol % 8) vterm_put_char(vterm,' ');
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

void
vterm_get_size( vterm_t *vterm, int *width, int *height )
{
    vterm_desc_t    *v_desc = NULL;
    int             idx;

    if(vterm == NULL || width == NULL || height == NULL )
        return;

    // set vterm desc buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    *width = v_desc->cols;
    *height = v_desc->rows;

    return;
}

vterm_cell_t**
vterm_get_buffer( vterm_t *vterm )
{
    vterm_desc_t    *v_desc = NULL;
    int             idx;

    if(vterm == NULL) return NULL;

    // set vterm desc buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    return v_desc->cells;
}
