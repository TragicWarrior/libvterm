/* expose wcwidth(3) (XSI): the project compiles -std=c99, under which
   glibc hides it without an explicit _XOPEN_SOURCE.  must precede all
   includes.  (700 implies _POSIX_C_SOURCE 200809L, already set by the
   build, so there is no conflict.) */
#define _XOPEN_SOURCE 700

#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>      // wcwidth() for double-width handling in vterm_put_char

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
        /*
            plain-ASCII fast path.  most terminal output is long runs of
            printable ASCII (shell prompts, log lines, source code, build
            output).  when no escape sequence, no UTF-8 assembly, and no
            rxvt RS1 scanner is mid-match, drop straight into a tight loop
            that calls vterm_put_char and bypasses the per-byte C0/C1/
            UTF-8/escape branch barrage.
        */
        if(!IS_MODE_ESCAPED(vterm)
            && !IS_MODE_UTF8(vterm)
            && vterm->rs1_off == 0)
        {
            while(i < len
                && (unsigned char)*data >= 0x20
                && (unsigned char)*data <= 0x7E)
            {
                vterm_put_char(vterm, (unsigned char)*data, NULL);
                i++;
                data++;
            }
            if(i >= len) break;
        }

        // completely ignore NUL
        if(*data == 0) continue;

        // only invoke the RS1 scanner on ESC or when mid-match
        if(vterm->rs1_reset != NULL
            && (vterm->rs1_off != 0 || *data == '\033'))
        {
            vterm->rs1_reset(vterm, (char *)data);
        }


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

                if(vterm->esc_suffix_check != NULL)
                {
                    // stray ESC inside a non-OSC sequence kills the parse
                    if(*data == '\033' && !IS_OSC_MODE(vterm))
                    {
                        vterm_escape_cancel(vterm);
                        continue;
                    }

                    if(vterm->esc_suffix_check(vterm))
                    {
                        vterm->esc_handler(vterm);
                        vterm_escape_cancel(vterm);
                    }
                }
                else
                {
                    vterm_interpret_escapes(vterm);
                }
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
    int             row, col;
    int             width;
    wchar_t         glyph;

    // set vterm desc buffer selector
    v_desc = vterm->v_desc_active;

    if(v_desc->ccol >= v_desc->cols)
    {
        v_desc->ccol = 0;

        if((v_desc->buffer_state & STATE_NO_WRAP) == FALSE)
        {
            vterm_scroll_up(vterm, FALSE);
        }
    }

    row = v_desc->crow;
    col = v_desc->ccol;

    // handle plain ASCII scenario
    if(wch == NULL)
    {
        attr_t  attr = v_desc->curattr;

        if(IS_MODE_ACS(vterm)) attr |= A_ALTCHARSET;

        VCELL_SET_ALL(v_desc, row, col, c, attr, v_desc->colors);

        v_desc->ccol++;

        return;
    }

    /*
        store a decoded (multibyte) glyph with double-width support.  a
        glyph whose display width is 2 (emoji, CJK, ...) occupies two
        cells: the glyph in `col` and a blank continuation in `col + 1`
        that the renderer (vterm_wnd_update) skips.  keeping the buffer's
        column accounting equal to the on-screen footprint is what stops
        ncurses' wide-char bookkeeping from desyncing during the host's
        copywin()/overwrite() composite.  width 1 (and the width < 0
        non-printable fallback) advance one column, as before.

        (the decoder terminates at wch[1], so wch[0] is the whole glyph.)
    */
    glyph = wch[0];
    width = wcwidth(glyph);
    if(width < 0) width = 1;

    if(width == 2 && col == v_desc->cols - 1)
    {
        /*
            a double-width glyph cannot straddle the right margin: blank
            the last cell and wrap, then place the pair at the start of
            the next line (DEC auto-wrap of wide characters).
        */
        VCELL_SET_ALL(v_desc, row, col, ' ', v_desc->curattr, v_desc->colors);
        v_desc->ccol = 0;
        if((v_desc->buffer_state & STATE_NO_WRAP) == FALSE)
            vterm_scroll_up(vterm, FALSE);
        row = v_desc->crow;
        col = v_desc->ccol;
    }

    VCELL_SET_ALL(v_desc, row, col, glyph, v_desc->curattr, v_desc->colors);

    if(width == 2)
    {
        /*
            blank continuation cell.  the renderer skips it (the wide
            glyph's mvwadd_wch paints both halves), and storing a space
            keeps it clean if either half is later erased.
        */
        VCELL_SET_ALL(v_desc, row, col + 1, ' ', v_desc->curattr,
            v_desc->colors);
        v_desc->ccol += 2;
    }
    else
    {
        v_desc->ccol += 1;
    }

    return;
}

void
vterm_get_size( vterm_t *vterm, int *width, int *height )
{
    vterm_desc_t    *v_desc = NULL;

    if(vterm == NULL || width == NULL || height == NULL )
        return;

    // set vterm desc buffer selector
    v_desc = vterm->v_desc_active;

    *width = v_desc->cols;
    *height = v_desc->rows;

    return;
}

