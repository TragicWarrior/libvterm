/*
Copyright (C) 2009 Bryan Christ

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/*
This library is based on ROTE written by Bruno Takahashi C. de Oliveira
*/

#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include "vterm.h"
#include "vterm_private.h"
#include "vterm_colors.h"
#include "vterm_buffer.h"

struct _my_color_pair_s
{
    short fg;
    short bg;
};

typedef struct _my_color_pair_s my_color_pair_t;

#define MAX_COLOR_PAIRS 512

my_color_pair_t *color_palette = NULL;
int palette_size = 0;

void
init_color_space()
{
    int i, j, n;

    if(color_palette == NULL)
    {
        color_palette = calloc(MAX_COLOR_PAIRS, sizeof(my_color_pair_t));

        if(color_palette == NULL)
        {
            fprintf(stderr, "ERROR: cannot allocate color palette!\n");
            exit(1);
        }
    }

    for(i = 0; i < 8; i++)
    {
        for(j = 0; j < 8; j++)
        {
            n = i * 8 + j;

            if(n >= MAX_COLOR_PAIRS)
            {
                fprintf(stderr, "ERROR: cannot set color pair %d!\n", n);
                exit(1);
            }

            color_palette[n].fg = 7 - i;
            color_palette[n].bg = j;

            if(n + 1 > palette_size )
            {
                palette_size = n + 1;
            }
        }
    }
}

void
free_color_space()
{
    if(color_palette == NULL) return;

    free(color_palette);
    color_palette = NULL;
    palette_size = 0;
}

short
find_color_pair_simple(vterm_t *vterm, short fg, short bg)
{
    my_color_pair_t *cp;
    int i = 0;

    (void)vterm;            // for later use.  suppress warnings for now

    if(color_palette == NULL)
    {
        init_color_space();
    }

    for(i = 0; i < palette_size; ++i)
    {
        cp = color_palette + i;

        if(cp->fg == fg && cp->bg == bg)
        {
            return i;
        }
    }

    return -1;
}

int
GetFGBGFromColorIndex(int index,int* fg, int* bg )
{
    if(color_palette == NULL || index >= palette_size || index < 0)
    {
        *fg = 0;
        *bg = 0;
        return -1;
    }

    *fg = color_palette[index].fg;
    *bg = color_palette[index].bg;

    return 0;
}

int
vterm_set_colors(vterm_t *vterm, short fg, short bg)
{
    vterm_desc_t    *v_desc = NULL;
    short           colors;
    int             idx;

    if(vterm == NULL) return -1;

    // set vterm description buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    if(vterm->flags & VTERM_FLAG_NOCURSES ) // no ncurses
    {
        colors = (short)find_color_pair_simple(vterm, fg, bg);
        if(colors == -1) colors = 0;
        v_desc->colors = colors;
    }
    else // ncurses
    {
#ifdef NOCURSES
        colors = find_color_pair(fg, bg);
#else
        if(has_colors() == FALSE)
            return -1;

        colors = find_color_pair(vterm, fg, bg);
#endif
        if(colors == -1) colors = 0;

        v_desc->colors = colors;
    }

    return 0;
}

short
vterm_get_colors(vterm_t *vterm)
{
    vterm_desc_t    *v_desc = NULL;
    int             idx;

    if(vterm == NULL) return -1;

    // set vterm description buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    if(vterm->flags & VTERM_FLAG_NOCURSES ) // no ncurses
    {
    }
    else // ncurses
    {
#ifndef NOCURSES
        if(has_colors() == FALSE) return -1;
#endif
    }

    return v_desc->colors;
}

short
find_color_pair(vterm_t *vterm, short fg,short bg)
{
    int             i;

    if(vterm->flags & VTERM_FLAG_NOCURSES ) // no ncurses
    {
        return find_color_pair_simple(vterm, fg, bg);
    }
    else // ncurses
    {
#ifdef NOCURSES
        return -1;
#else
        short   fg_color, bg_color;
        if(has_colors() == FALSE)
            return -1;

        for(i = 1; i < COLOR_PAIRS; i++)
        {
            pair_content(i, &fg_color, &bg_color);
            if(fg_color == fg && bg_color == bg) break;
        }

        if(i == COLOR_PAIRS)
            return -1;
#endif
    }

    return i;
}
