
#include "vterm.h"
#include "vterm_private.h"
#include "vterm_buffer.h"
#include "vterm_colors.h"

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
    wchar_t         wch[CCHARW_MAX];

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
            // get character from wide storage
            getcchar(&vcell->uch, wch, &attrs, &colors, NULL);

            VCELL_GET_COLORS((*vcell), &colors);

            /*
                on Mac OS, the color and ACS attributes stored
                in the cchar_t will trump what's set by
                wattr_set() so we have to explicitly sync them
            */
            if(attrs & A_ALTCHARSET)
            {
                VCELL_GET_ATTR((*vcell), &attrs);
                attrs |= A_ALTCHARSET;
            }
            else
            {
                VCELL_GET_ATTR((*vcell), &attrs);
            }

            setcchar(&vcell->uch, wch, attrs, colors, NULL);

            wattr_set(vterm->window, attrs, colors, NULL);
            mvwadd_wch(vterm->window, r, c, &vcell->uch);

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

