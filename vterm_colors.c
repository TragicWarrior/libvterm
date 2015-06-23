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

#include "vterm.h"
#include "vterm_private.h"
#include "vterm_colors.h"

int
vterm_set_colors(vterm_t *vterm,short fg,short bg)
{
    short   colors;

    if(vterm == NULL) return -1;
    if(has_colors() == FALSE) return -1;

    colors = find_color_pair(fg,bg);
    if(colors == -1) colors = 0;

    vterm->colors = colors;

    return 0;
}

short
vterm_get_colors(vterm_t *vterm)
{
    if(vterm == NULL) return -1;
    if(has_colors() == FALSE) return -1;

    return vterm->colors;
}

short
find_color_pair(short fg,short bg)
{
    short   fg_color,bg_color;
    int     i;

    if(has_colors() == FALSE) return -1;

    for(i = 1;i < COLOR_PAIRS;i++)
    {
        pair_content(i,&fg_color,&bg_color);
        if(fg_color == fg && bg_color == bg) break;
    }

    if(i == COLOR_PAIRS) return -1;

    return i;
}


