
#include <stdio.h>
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
    int             width;
    int             height;
    int             r, c;
    attr_t          attrs;
    short           colors;
    cchar_t         uch;

    if(vterm == NULL) return -1;
    if(vterm->window == NULL) return -1;

    // set vterm desc buffer selector
    if(idx == -1)
    {
        idx = vterm_buffer_get_active(vterm);
    }

    v_desc = &vterm->vterm_desc[idx];

    getmaxyx(vterm->window, height, width);
    VAR_UNUSED(width);

    height = USE_MIN(height, v_desc->rows);

    for(r = 0; r < height; r++)
    {
        // start at the beginning of the row
        vcell = &v_desc->cells[r + offset][0];

        for(c = 0; c < v_desc->cols; c++)
        {
            // if(vcell->dirty == 0)
            if(vcell->dirty == 0 && !(flags & VTERM_WND_RENDER_ALL))
            {
                vcell++;
                continue;
            }

            VCELL_GET_COLORS((*vcell), &colors);
            VCELL_GET_ATTR((*vcell), &attrs);

            /*
                on Mac OS, the color and ACS attributes stored
                in the cchar_t will trump what's set by
                wattr_set() so we have to explicitly sync them
            */
            if(setcchar(&uch, vcell->wch, attrs, colors, NULL) == ERR)
            {
                VCELL_SET_CHAR((*vcell), ' ');
            }

            wattr_set(vterm->window, attrs, colors, NULL);
            mvwadd_wch(vterm->window, r, c, &uch);

            if(!(flags & VTERM_WND_LEAVE_DIRTY))
            {
                VCELL_ROW_SET_CLEAN(vcell, 1);
            }
            vcell++;
        }
    }

    if(idx != VTERM_BUF_HISTORY)
    {
        if(!(v_desc->buffer_state & STATE_CURSOR_INVIS))
        {
            mvwchgat(vterm->window, v_desc->crow, v_desc->ccol, 1, A_REVERSE,
                v_desc->default_colors, NULL);

            VCELL_ROW_SET_DIRTY(&v_desc->cells[v_desc->crow][v_desc->ccol], 1);
        }
    }

    return -1;
}

#endif

