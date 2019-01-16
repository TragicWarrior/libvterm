
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

            VCELL_GET_ATTR((*vcell), &attrs);

            // this is a temp change for migrating to wattr_set()
            colors = PAIR_NUMBER(attrs);

        /*
            {
                short   fg;
                short   bg;
                short   f_red;
                short   f_green;
                short   f_blue;

                if(colors > 250)
                {
                    pair_content(colors, &fg, &bg);
                    color_content(fg, &f_red, &f_green, &f_blue);

                    endwin();
                    printf("pair:   %d, f: %d, b: %d\n\r", colors, fg, bg);
                    printf("fg:     r: %d, g: %d, b: %d\n\r", f_red, f_green, f_blue);
                    exit(0);
                }
            }
        */

            wattr_set(vterm->window, attrs, colors, NULL);
            mvwadd_wch(vterm->window, r, c, &vcell->uch);

            vcell++;
        }
    }

    if(!(v_desc->buffer_state & STATE_CURSOR_INVIS))
    {
        mvwchgat(vterm->window, v_desc->crow, v_desc->ccol, 1, A_REVERSE,
            v_desc->colors, NULL);
    }

    return;
}



#endif

