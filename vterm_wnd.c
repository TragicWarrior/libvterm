
/* expose wcwidth(3) (XSI) under -std=c99; must precede all includes. */
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#ifdef __FreeBSD__
#include <ncurses/ncurses.h>
#endif

#include "macros.h"
#include "vterm.h"
#include "vterm_private.h"
#include "vterm_buffer.h"
#include "color_cache.h"

#ifndef NOCURSES

/*
    emit a homogeneous attr/color run of pre-built cchar_t cells.
    callers flush and start a new span whenever attrs or colors change:
    a single wattr_set for the whole span keeps window style in sync
    with the cchar attrs (required on Mac OS; on Linux a mixed-style
    bulk write let the first cell's wattr_set bleed -- e.g. green path
    over a gray bash `$`).
*/
static void
_wnd_flush_span(WINDOW *win, int row, int col,
                cchar_t *buf, int n, attr_t attrs, short colors)
{
    if(n <= 0) return;

    wattr_set(win, attrs, colors, NULL);
    mvwadd_wchnstr(win, row, col, buf, n);

    return;
}

void
vterm_wnd_set(vterm_t *vterm,WINDOW *window)
{
    if(vterm == NULL) return;

    vterm->window = window;

    return;
}

WINDOW*
vterm_wnd_get(vterm_t *vterm)
{
    return vterm->window;
}

void
vterm_wnd_size(vterm_t *vterm, int *width, int *height)
{
    if(vterm == NULL) return;
    if(vterm->window == NULL) return;

    getmaxyx(vterm->window, *height, *width);

    return;
}

int
vterm_wnd_update(vterm_t *vterm, int idx, int offset, uint8_t flags)
{
    vterm_cell_t    *vcell;
    vterm_desc_t    *v_desc = NULL;
    cchar_t         *row_buf = NULL;
    int             width;
    int             height;
    int             r, c;
    attr_t          attrs;
    short           colors;

    if(vterm == NULL) return -1;
    if(vterm->window == NULL) return -1;

    // set vterm desc buffer selector
    if(idx == -1)
    {
        idx = vterm->vterm_desc_idx;
        v_desc = vterm->v_desc_active;
    }
    else
    {
        v_desc = &vterm->vterm_desc[idx];
    }

    getmaxyx(vterm->window, height, width);
    VAR_UNUSED(width);

    height = USE_MIN(height, v_desc->rows);

    /*
        one cchar buffer reused every row: dirty spans are packed then
        emitted with a single mvwadd_wchnstr (phase 2 of the paint
        hot-path work -- replaces per-cell mvwadd_wch).
    */
    if(v_desc->cols > 0)
    {
        row_buf = (cchar_t *)malloc(sizeof(cchar_t) * (size_t)v_desc->cols);
        if(row_buf == NULL) return -1;
    }

    for(r = 0; r < height; r++)
    {
        /*
            logical -> physical row mapping; identity for STD / ALT
            (head is 0), rotation-aware for the HISTORY ring.
        */
        int     prow = vterm_desc_row_phys(v_desc, r + offset);
        int     skip_next = 0;
        int     span_start = 0;
        int     span_n = 0;
        attr_t  span_attrs = 0;
        short   span_colors = 0;

        for(c = 0; c < v_desc->cols; c++)
        {
            if(skip_next)
            {
                /*
                    right half of a wide glyph: the left half's wide
                    cchar already claims this column when the span is
                    flushed.  drop the dirty bit and keep the span open
                    so the next dirty cell appends after the wide glyph.
                */
                skip_next = 0;
                if(!(flags & VTERM_WND_LEAVE_DIRTY))
                    VCELL_DIRTY_CLEAR(v_desc, prow, c);
                continue;
            }

            vcell = &v_desc->cells[prow][c];

            /*
                a double-width glyph claims the next cell too; mark it to
                be skipped on the next pass.  done before the dirty gate
                so a clean (non-dirty) wide cell still protects its right
                half.  the > 0x7F guard keeps wcwidth() off the ASCII path.
            */
            if(vcell->wch[0] > 0x7F && wcwidth(vcell->wch[0]) == 2)
                skip_next = 1;

            if(!VCELL_DIRTY_TEST(v_desc, prow, c)
                && !(flags & VTERM_WND_RENDER_ALL))
            {
                /*
                    clean cell breaks the span -- flush what we have so
                    we never overwrite a clean column with a bulk write.
                */
                _wnd_flush_span(vterm->window, r, span_start, row_buf,
                    span_n, span_attrs, span_colors);
                span_n = 0;
                continue;
            }

            VCELL_GET_COLORS((*vcell), &colors);
            VCELL_GET_ATTR((*vcell), &attrs);

            /*
                style change ends the current homogeneous span so the
                next flush's wattr_set matches every cell in the batch.
            */
            if(span_n > 0
                && (attrs != span_attrs || colors != span_colors))
            {
                _wnd_flush_span(vterm->window, r, span_start, row_buf,
                    span_n, span_attrs, span_colors);
                span_n = 0;
            }

            if(span_n == 0)
            {
                span_start = c;
                span_attrs = attrs;
                span_colors = colors;
            }

            /*
                on Mac OS, the color and ACS attributes stored
                in the cchar_t will trump what's set by
                wattr_set() so we have to explicitly sync them
            */
            if(setcchar(&row_buf[span_n], vcell->wch, attrs, colors, NULL)
                == ERR)
            {
                wchar_t blank[2] = { L' ', L'\0' };

                VCELL_SET_CHAR(v_desc, prow, c, ' ');
                setcchar(&row_buf[span_n], blank, attrs, colors, NULL);
            }
            span_n++;

            if(!(flags & VTERM_WND_LEAVE_DIRTY))
            {
                VCELL_DIRTY_CLEAR(v_desc, prow, c);
            }
        }

        _wnd_flush_span(vterm->window, r, span_start, row_buf,
            span_n, span_attrs, span_colors);
    }

    free(row_buf);

    if(idx != VTERM_BUF_HISTORY)
    {
        if(!(v_desc->buffer_state & STATE_CURSOR_INVIS))
        {
            /*
                at DEC pending-wrap the cursor rests at ccol == cols (a
                last-column glyph advances ccol past the margin and the
                wrap is deferred to the next glyph).  Draw + dirty the
                last real column instead: VCELL_DIRTY_SET(crow, cols)
                would index dirty_bits[crow][cols>>3], one byte past the
                row's VCELL_DIRTY_ROW_BYTES(cols) when cols is a multiple
                of 8 (the common 80-column case) -- a heap write off the
                end of the dirty block on the bottom row.
            */
            int cur_col = v_desc->ccol;
            if(cur_col >= v_desc->cols) cur_col = v_desc->cols - 1;
            if(cur_col < 0) cur_col = 0;

            mvwchgat(vterm->window, v_desc->crow, cur_col, 1, A_REVERSE,
                v_desc->default_colors, NULL);

            VCELL_DIRTY_SET(v_desc, v_desc->crow, cur_col);
        }
    }

    return -1;
}

/*
    Render a scrollback view that composes the newest `nlines` evicted history
    rows above the head of the live standard screen -- the way a hardware
    terminal scrolls back.  At nlines == 0 the window is exactly the live
    screen; each extra line slides one evicted row in at the top and pushes
    the live screen down by one.  This reveals fewer-than-one-screen of
    history against the live buffer, which vterm_wnd_update(VTERM_BUF_HISTORY)
    cannot -- HISTORY holds only evicted rows, never the live screen.

    `nlines` is the scroll-back distance; the caller clamps it to
    0 .. vterm_get_history_used().  No cursor is drawn (a frozen view).
*/
int
vterm_wnd_scrollback(vterm_t *vterm, int nlines, uint8_t flags)
{
    vterm_cell_t    *vcell;
    vterm_desc_t    *hist;
    vterm_desc_t    *live;
    vterm_desc_t    *v_desc;
    cchar_t         *row_buf = NULL;
    int             width, height;
    int             capacity;
    int             r, c, lrow, prow;
    int             max_cols;
    attr_t          attrs;
    short           colors;

    if(vterm == NULL) return -1;
    if(vterm->window == NULL) return -1;

    hist = &vterm->vterm_desc[VTERM_BUF_HISTORY];
    live = &vterm->vterm_desc[VTERM_BUF_STANDARD];
    capacity = hist->rows;

    if(nlines < 0) nlines = 0;

    getmaxyx(vterm->window, height, width);
    VAR_UNUSED(width);
    VAR_UNUSED(flags);

    height = USE_MIN(height, live->rows);

    /*
        history and live share geometry; size the row buffer once for
        the wider of the two (they should match).
    */
    max_cols = live->cols;
    if(hist->cols > max_cols) max_cols = hist->cols;

    if(max_cols > 0)
    {
        row_buf = (cchar_t *)malloc(sizeof(cchar_t) * (size_t)max_cols);
        if(row_buf == NULL) return -1;
    }

    for(r = 0; r < height; r++)
    {
        int     skip_next = 0;
        int     span_start = 0;
        int     span_n = 0;
        attr_t  span_attrs = 0;
        short   span_colors = 0;

        /*
            top `lines` rows come from the tail of the history ring (newest
            evicted first); the rest is the live screen shifted down by
            `lines`.  history rows are right-aligned in the ring, so the
            newest evicted row is at capacity-1.
        */
        if(r < nlines)
        {
            v_desc = hist;
            lrow = capacity - nlines + r;
        }
        else
        {
            v_desc = live;
            lrow = r - nlines;
        }

        if(lrow < 0 || lrow >= v_desc->rows) continue;
        prow = vterm_desc_row_phys(v_desc, lrow);

        for(c = 0; c < v_desc->cols; c++)
        {
            if(skip_next)
            {
                skip_next = 0;
                continue;
            }

            vcell = &v_desc->cells[prow][c];

            if(vcell->wch[0] > 0x7F && wcwidth(vcell->wch[0]) == 2)
                skip_next = 1;

            VCELL_GET_COLORS((*vcell), &colors);
            VCELL_GET_ATTR((*vcell), &attrs);

            if(span_n > 0
                && (attrs != span_attrs || colors != span_colors))
            {
                _wnd_flush_span(vterm->window, r, span_start, row_buf,
                    span_n, span_attrs, span_colors);
                span_n = 0;
            }

            if(span_n == 0)
            {
                span_start = c;
                span_attrs = attrs;
                span_colors = colors;
            }

            if(setcchar(&row_buf[span_n], vcell->wch, attrs, colors, NULL)
                == ERR)
            {
                wchar_t blank[2] = { L' ', L'\0' };
                setcchar(&row_buf[span_n], blank, attrs, colors, NULL);
            }
            span_n++;
        }

        _wnd_flush_span(vterm->window, r, span_start, row_buf, span_n,
            span_attrs, span_colors);
    }

    free(row_buf);

    return 0;
}

#endif
