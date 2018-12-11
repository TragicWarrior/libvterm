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

static void
vterm_render_ctrl_char(vterm_t *vterm,char c);

static void
vterm_put_char(vterm_t *vterm,chtype c);


void
vterm_render(vterm_t *vterm, const char *data, int len)
{
    vterm_desc_t    *v_desc = NULL;
    chtype          utf8_char;
    int             bytes = -1;
    int             idx;
    int             i;

    // set the vterm description buffer selector
    idx = vterm_get_active_buffer(vterm);
    v_desc = &vterm->vterm_desc[idx];

    for(i = 0; i < len; i++, data++)
    {
        // completely ignore NUL
        if(*data == 0) continue;

        if(!IS_MODE_ESCAPED(v_desc))
        {
            if((unsigned int)*data >= 1 && (unsigned int)*data <= 31)
            {
                vterm_render_ctrl_char(vterm,*data);
                continue;
            }
        }

        // the non-ascii code points for utf start at 0x80
        if((unsigned int)*data > 0x7F)
        {
            if(!IS_MODE_UTF8(v_desc))
            {
                vterm_utf8_start(vterm);
            }
        }

        if(IS_MODE_UTF8(v_desc))
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

        if(IS_MODE_ESCAPED(v_desc))
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
vterm_put_char(vterm_t *vterm,chtype c)
{
    vterm_desc_t    *v_desc = NULL;
    static char     vt100_acs[]="`afgjklmnopqrstuvwxyz{|}~,+-.";
    int             idx;

    // set vterm desc buffer selector
    idx = vterm_get_active_buffer(vterm);
    v_desc = &vterm->vterm_desc[idx];

    if(v_desc->ccol >= v_desc->cols)
    {
        v_desc->ccol = 0;
        vterm_scroll_down(vterm);
    }

    if(IS_MODE_ACS(v_desc))
    {
        if(strchr(vt100_acs, (char)c) != NULL)
        {
            v_desc->cells[v_desc->crow][v_desc->ccol].ch = NCURSES_ACS(c);
        }
    }
    else
    {
        v_desc->cells[v_desc->crow][v_desc->ccol].ch = c;
    }

    v_desc->cells[v_desc->crow][v_desc->ccol].attr = v_desc->curattr;
    v_desc->ccol++;

    return;
}

void
vterm_render_ctrl_char(vterm_t *vterm, char c)
{
    vterm_desc_t    *v_desc = NULL;
    int             idx;

    // set vterm desc buffer selector
    idx = vterm_get_active_buffer(vterm);
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
            // vterm->ccol = 0;
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
            v_desc->buffer_state |= STATE_ALT_CHARSET;
            break;
        }

        // exit graphical character mode
        case '\x0F':
        {
            v_desc->buffer_state &= ~STATE_ALT_CHARSET;
            break;
        }

/*
        // CSI character. Equivalent to ESC [
        case '\x9B':
        {
            vterm_escape_start(vterm);

            // inject the bracket [
            vterm->esbuf[vterm->esbuf_len++] = '[';
            break;
        }
*/

        // these interrupt escape sequences
        case '\x18':

/*
        case '\x1A':
        {
            vterm_escape_cancel(vterm);
            break;
        }
*/

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
    idx = vterm_get_active_buffer(vterm);
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
    idx = vterm_get_active_buffer(vterm);
    v_desc = &vterm->vterm_desc[idx];

    return v_desc->cells;
}
