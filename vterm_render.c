#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>

#ifdef __FreeBSD__
#include <ncurses/ncurses.h>
#endif

#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_escape.h"
#include "vterm_utf8.h"
#include "vterm_buffer.h"
#include "vterm_ctrl_char.h"
#include "vterm_osc.h"
#include "color_cache.h"
#include "macros.h"

// this is necessary for FreeBSD in some compilation scenarios
#ifndef CCHARW_MAX
#define CCHARW_MAX  5
#endif

static void
vterm_put_char(vterm_t *vterm, chtype c, wchar_t *wch);

void
vterm_render(vterm_t *vterm, char *data, int len)
{
#ifndef NOUTF8
    chtype          utf8_char;
    wchar_t         wch[CCHARW_MAX];
    int             bytes = -1;
#endif
    int             i;

    for(i = 0; i < len; i++, data++)
    {
        // completely ignore NUL
        if(*data == 0) continue;

        // special processing looking for reset sequence
        if(vterm->rs1_reset != NULL) vterm->rs1_reset(vterm, (char *)data);


        if(!IS_MODE_ESCAPED(vterm))
        {
            if(IS_C0(*data))
            {
                vterm_interpret_ctrl_char(vterm, data);
                continue;
            }
            else if(!IS_MODE_UTF8(vterm) && IS_C1((unsigned char)*data))
            {
                vterm_escape_start(vterm);
            }
        }

#ifndef NOUTF8
        if(!(vterm->flags & VTERM_FLAG_NOUTF8))
        {
            // UTF-8 encoding is indicated by a bit at 0x80
            if((unsigned int)*data > 0x7F && !IS_MODE_UTF8(vterm)
                && !IS_C1((unsigned char)*data))
            {
                vterm_utf8_start(vterm);
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
                // memset(wch, 0, sizeof(wch));
                wch[0] = '\0';
                bytes = vterm_utf8_decode(vterm, &utf8_char, wch);

                // we're done
                if(bytes > 0)
                {
                    vterm_utf8_cancel(vterm);
                    if(bytes == 2 && IS_C1(wch[0])) vterm_escape_start(vterm);
                    else
                    {
                        vterm_put_char(vterm, *data, wch);
                        continue;
                    }
                }
                else continue;
            }
        }
#endif

        if(IS_MODE_ESCAPED(vterm))
        {
            if(vterm->esbuf_len >= ESEQ_BUF_SIZE)
            {
                vterm_escape_cancel(vterm);
                continue;
            }

            /*
                it's absurd but control chars can legitimately happen inside
                a escape sequence.  process them but omit them from the
                ESEQ_BUF.  the exception to this is OSC mode which will
                contain a control character to terminate the OSC string.
            */
            if(IS_CTRL_CHAR(*data) && !IS_OSC_MODE(vterm))
            {
                vterm_interpret_ctrl_char(vterm, data);
                continue;
            }
            else
            {
                // append character to ongoing escape sequence
                vterm->esbuf[vterm->esbuf_len] = *data;

                // increment the buffer length and push out the NULL terminator
                vterm->esbuf_len++;
                vterm->esbuf[vterm->esbuf_len] = 0;

                // if we are in escape mode (initiated by 0x1B) go here...
                vterm_interpret_escapes(vterm);
                continue;
            }
        }
        else
        {
            vterm_put_char(vterm, *data, NULL);
            continue;
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

        if((v_desc->buffer_state & STATE_NO_WRAP) == FALSE)
        {
            vterm_scroll_up(vterm, FALSE);
        }
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

        v_desc->ccol++;

        return;
    }

    // handle wide character
    memcpy(vcell->wch, wch, sizeof(vcell->wch));

    VCELL_SET_ATTR((*vcell), v_desc->curattr);
    VCELL_SET_COLORS((*vcell), v_desc);

    v_desc->ccol++;

    return;
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

