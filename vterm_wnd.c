
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
short
_vterm_set_color_pair_safe(vterm_desc_t *v_desc, vterm_t *vterm, short colors,
    int fg, int bg);

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
vterm_wnd_update(vterm_t *vterm, int idx, int offset)
{
    vterm_cell_t    *vcell;
    vterm_desc_t    *v_desc = NULL;
    int             width;
    int             height;
    int             r, c;
    attr_t          attrs;
    short           colors;
    vterm_color_t   fg, bg;
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
            VCELL_GET_COLORS((*vcell), &fg, &bg);
            colors = _vterm_set_color_pair_safe(v_desc, vterm, -1,
                fg.type == VTERM_COLOR_TYPE_DEFAULT? v_desc->default_fg: fg.index,
                bg.type == VTERM_COLOR_TYPE_DEFAULT? v_desc->default_bg: bg.index);
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

    if(idx != VTERM_BUF_HISTORY)
    {
        if(!(v_desc->buffer_state & STATE_CURSOR_INVIS))
        {
            mvwchgat(vterm->window, v_desc->crow, v_desc->ccol, 1, A_REVERSE,
                v_desc->default_colors, NULL);
        }
    }

    return -1;
}

inline short
_vterm_set_color_pair_safe(vterm_desc_t *v_desc, vterm_t *vterm, short colors,
    int fg, int bg)
{
    // find the required pair in the cache
    if(colors == -1) colors = color_cache_find_pair(fg, bg);

    // no color pair found so we'll try and add it (if requested)
    if(colors == -1) colors = color_cache_add_pair(vterm, fg, bg);

    // one addtl safeguard
    if(colors == -1) colors = 0;
    v_desc->colors = colors;

    color_content(fg, &v_desc->f_rgb[0], &v_desc->f_rgb[1], &v_desc->f_rgb[2]);
    color_content(bg, &v_desc->b_rgb[0], &v_desc->b_rgb[1], &v_desc->b_rgb[2]);

    return colors;
}

#endif

