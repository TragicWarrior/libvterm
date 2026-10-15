/* expose wcwidth(3) (XSI): the project compiles -std=c99, under which
   glibc hides it without an explicit _XOPEN_SOURCE.  must precede all
   includes.  (700 implies _POSIX_C_SOURCE 200809L, already set by the
   build, so there is no conflict.) */
#define _XOPEN_SOURCE 700

#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>      // wcwidth() for double-width handling in vtty_put_char

#include <sys/types.h>
#include <sys/stat.h>

#ifdef __FreeBSD__
#include <ncurses/ncurses.h>
#endif

#include "vtty.h"
#include "vtty_private.h"
#include "vtty_csi.h"
#include "vtty_escape.h"
#include "vtty_utf8.h"
#include "vtty_buffer.h"
#include "vtty_ctrl_char.h"
#include "vtty_osc.h"
#include "color_cache.h"
#include "macros.h"

// this is necessary for FreeBSD in some compilation scenarios
#ifndef CCHARW_MAX
#define CCHARW_MAX  5
#endif

static void
vtty_put_char(vtty_t *vtty, chtype c, wchar_t *wch);

void
vtty_render(vtty_t *vtty, char *data, int len)
{
#ifndef NOUTF8
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
            that calls vtty_put_char and bypasses the per-byte C0/C1/
            UTF-8/escape branch barrage.
        */
        if(!IS_MODE_ESCAPED(vtty)
            && !IS_MODE_UTF8(vtty)
            && vtty->rs1_off == 0)
        {
            while(i < len
                && (unsigned char)*data >= 0x20
                && (unsigned char)*data <= 0x7E)
            {
                vtty_put_char(vtty, (unsigned char)*data, NULL);
                i++;
                data++;
            }
            if(i >= len) break;
        }

        // completely ignore NUL
        if(*data == 0) continue;

        // only invoke the RS1 scanner on ESC or when mid-match
        if(vtty->rs1_reset != NULL
            && (vtty->rs1_off != 0 || *data == '\033'))
        {
            vtty->rs1_reset(vtty, (char *)data);
        }


        if(!IS_MODE_ESCAPED(vtty))
        {
            if(IS_C0(*data))
            {
                vtty_interpret_ctrl_char(vtty, data);
                continue;
            }
            else if(!IS_MODE_UTF8(vtty) && IS_C1((unsigned char)*data))
            {
                vtty_escape_start(vtty);
            }
        }

#ifndef NOUTF8
        /*
            assemble UTF-8 only when NOT inside an escape/OSC sequence.  an
            OSC string (e.g. an xterm window title) can legitimately carry
            UTF-8; those bytes must accumulate into esbuf and be consumed by
            the OSC handler below -- not get decoded and PRINTED at the
            cursor.  this leak put an app's UTF-8 title-spinner glyphs on
            screen at the cursor position.
        */
        if(!(vtty->flags & VTTY_FLAG_NOUTF8) && !IS_MODE_ESCAPED(vtty))
        {
            // UTF-8 encoding is indicated by a bit at 0x80
            if((unsigned int)*data > 0x7F && !IS_MODE_UTF8(vtty)
                && !IS_C1((unsigned char)*data))
            {
                vtty_utf8_start(vtty);
            }

            if(IS_MODE_UTF8(vtty))
            {
                if(vtty->utf8_buf_len >= UTF8_BUF_SIZE)
                {
                    vtty_utf8_cancel(vtty);
                    continue;
                }

                // append byte to utf-8 buffer
                vtty->utf8_buf[vtty->utf8_buf_len] = *data;

                // increment the buffer length and push out the NULL terminator
                vtty->utf8_buf_len++;
                vtty->utf8_buf[vtty->utf8_buf_len] = 0;

                // we're in UTF-8 mode... do this
                // memset(wch, 0, sizeof(wch));
                wch[0] = '\0';
                bytes = vtty_utf8_decode(vtty, wch);

                // we're done
                if(bytes > 0)
                {
                    vtty_utf8_cancel(vtty);
                    if(bytes == 2 && IS_C1(wch[0])) vtty_escape_start(vtty);
                    else
                    {
                        vtty_put_char(vtty, *data, wch);
                        continue;
                    }
                }
                else continue;
            }
        }
#endif

        if(IS_MODE_ESCAPED(vtty))
        {
            /*
                it's absurd but control chars can legitimately happen inside
                a escape sequence.  process them but omit them from the
                ESEQ_BUF.  the exception to this is OSC mode which will
                contain a control character to terminate the OSC string.
            */
            if(IS_CTRL_CHAR(*data) && !IS_OSC_MODE(vtty))
            {
                vtty_interpret_ctrl_char(vtty, data);
                continue;
            }
            else
            {
                /*
                    CSI stays inside esbuf_small.  OSC (especially 52)
                    grows up to ESEQ_BUF_OSC_MAX; past that, drop the
                    sequence rather than spill the payload onto the
                    screen.
                */
                if(vtty_esbuf_putc(vtty, *data) == -1)
                {
                    vtty_escape_cancel(vtty);
                    continue;
                }

                if(vtty->esc_suffix_check != NULL)
                {
                    // stray ESC inside a non-OSC sequence kills the parse
                    if(*data == '\033' && !IS_OSC_MODE(vtty))
                    {
                        vtty_escape_cancel(vtty);
                        continue;
                    }

                    if(vtty->esc_suffix_check(vtty))
                    {
                        vtty->esc_handler(vtty);
                        vtty_escape_cancel(vtty);
                    }
                }
                else
                {
                    vtty_interpret_escapes(vtty);
                }
                continue;
            }
        }
        else
        {
            vtty_put_char(vtty, *data, NULL);
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
vtty_put_char(vtty_t *vtty, chtype c, wchar_t *wch)
{
    vtty_desc_t     *v_desc = NULL;
    int             row, col;
    int             width;
    wchar_t         glyph;

    // set vtty desc buffer selector
    v_desc = vtty->v_desc_active;

    if(v_desc->ccol >= v_desc->cols)
    {
        v_desc->ccol = 0;

        if((v_desc->buffer_state & STATE_NO_WRAP) == FALSE)
        {
            vtty_scroll_up(vtty, FALSE);
        }
    }

    row = v_desc->crow;
    col = v_desc->ccol;

    // handle plain ASCII scenario
    if(wch == NULL)
    {
        attr_t  attr = v_desc->curattr;

        if(IS_MODE_ACS(vtty)) attr |= A_ALTCHARSET;

        VCELL_SET_ALL(v_desc, row, col, c, attr, v_desc->colors);

        v_desc->ccol++;

        return;
    }

    /*
        store a decoded (multibyte) glyph with double-width support.  a
        glyph whose display width is 2 (emoji, CJK, ...) occupies two
        cells: the glyph in `col` and a blank continuation in `col + 1`
        that the renderer (vtty_wnd_update) skips.  keeping the buffer's
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
            vtty_scroll_up(vtty, FALSE);
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
vtty_get_size( vtty_t *vtty, int *width, int *height )
{
    vtty_desc_t     *v_desc = NULL;

    if(vtty == NULL || width == NULL || height == NULL )
        return;

    // set vtty desc buffer selector
    v_desc = vtty->v_desc_active;

    *width = v_desc->cols;
    *height = v_desc->rows;

    return;
}

