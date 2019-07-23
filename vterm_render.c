
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_escape.h"
#include "vterm_utf8.h"
#include "vterm_buffer.h"
#include "color_cache.h"
#include "macros.h"

static void
vterm_render_ctrl_char(vterm_t *vterm,char c);

static void
vterm_put_char(vterm_t *vterm, chtype c, wchar_t *wch);


void
vterm_render(vterm_t *vterm, const char *data, int len)
{
    chtype          utf8_char;
    wchar_t         wch[CCHARW_MAX];
    int             bytes = -1;
    int             i;

    for(i = 0; i < len; i++, data++)
    {
        // completely ignore NUL
        if(*data == 0) continue;

        // special processing looking for reset sequence
        if(vterm->rs1_reset != NULL) vterm->rs1_reset(vterm, (char *)data);

        if(!IS_MODE_ESCAPED(vterm))
        {
            if((unsigned int)*data >= 1 && (unsigned int)*data <= 31)
            {
                vterm_render_ctrl_char(vterm, *data);
                continue;
            }
        }

        // UTF-8 encoding is indicated by a bit at 0x80
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
            memset(wch, 0, sizeof(wch));
            bytes = vterm_utf8_decode(vterm, &utf8_char, wch);

            // we're done
            if(bytes > 0)
            {
                vterm_put_char(vterm, *data, wch);
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
            vterm_put_char(vterm, *data, NULL);
        }
    }

    return;
}

/*
    if wch is not NULL, then 'c' should be ignored in this
    function.

    todo:  this function is highly dependent on ncurses.  an alternate
    version needs to be written which doesn't rely on ncurses to
    support the NOCURSES build option.
*/
void
vterm_put_char(vterm_t *vterm, chtype c, wchar_t *wch)
{
    vterm_desc_t    *v_desc = NULL;
    vterm_cell_t    *vcell = NULL;
    int             idx;

    // set vterm desc buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    if(v_desc->ccol >= v_desc->cols)
    {
        v_desc->ccol = 0;
        vterm_scroll_down(vterm, FALSE);
    }

    /*
        store the location of the cell so we don't have to do
        multiple scalar look-ups later.
    */
    vcell = &v_desc->cells[v_desc->crow][v_desc->ccol];

    // handle plain ASCII scenario
    if(wch == NULL)
    {
        VCELL_SET_CHAR((*vcell), c);
        VCELL_SET_COLORS((*vcell), v_desc);

        if(IS_MODE_ACS(vterm))
        {
            VCELL_SET_ATTR((*vcell), v_desc->curattr | A_ALTCHARSET);
        }
        else
        {
            VCELL_SET_ATTR((*vcell), v_desc->curattr);
        }

        // "remember" what was written in case the next call is csi REP.
        memcpy(&v_desc->last_cell, vcell, sizeof(vterm_cell_t));

        v_desc->ccol++;

        return;
    }

    // handle wide character
    memcpy(vcell->wch, wch, sizeof(vcell->wch));

    VCELL_SET_ATTR((*vcell), v_desc->curattr);
    VCELL_SET_COLORS((*vcell), v_desc);

    // "remember" what was written in case the next call is csi REP.
    memcpy(&v_desc->last_cell, vcell, sizeof(vterm_cell_t));

    v_desc->ccol++;

    return;
}

void
vterm_render_ctrl_char(vterm_t *vterm, char c)
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

