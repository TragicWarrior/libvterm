
#include <stdio.h>
#include <wchar.h>

#ifdef __FreeBSD__
#include <ncurses/ncurses.h>
#endif

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
vterm_wnd_update(vterm_t *vterm)
{
    vterm_cell_t    *vcell;
    vterm_desc_t    *v_desc = NULL;
    int             r, c;
    int             idx;
    attr_t          attrs;
    short           colors;


    // static cchar_t  uch;                    // Mac OS blows up if not static
    cchar_t         uch;

    if(vterm == NULL) return;
    if(vterm->window == NULL) return;

    // set vterm desc buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    for(r = 0; r < v_desc->rows; r++)
    {
        // start at the beginning of the row
        vcell = &v_desc->cells[r][0];

        for(c = 0; c < v_desc->cols; c++)
        {
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

            vcell++;
        }
    }

    if(!(v_desc->buffer_state & STATE_CURSOR_INVIS))
    {
        mvwchgat(vterm->window, v_desc->crow, v_desc->ccol, 1, A_REVERSE,
            v_desc->default_colors, NULL);
    }

    return;
}

#endif

