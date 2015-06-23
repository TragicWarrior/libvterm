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
    int             i;
    int             x,y;
    int             cell_count;
    chtype          ch;
    unsigned int    attr;

    if(vterm == NULL) return;
    if(vterm->window == NULL) return;

    cell_count = vterm->rows * vterm->cols;

    for(i = 0;i < cell_count;i++)
    {
        x = i % vterm->cols;
        y = (int)(i / vterm->cols);

        ch = vterm->cells[y][x].ch;
        attr = vterm->cells[y][x].attr;

        wattrset(vterm->window,attr);
        wmove(vterm->window,y,x);
        waddch(vterm->window,ch);
    }

    if(!(vterm->state & STATE_CURSOR_INVIS))
    {
        mvwchgat(vterm->window,vterm->crow,vterm->ccol,1,A_REVERSE,
            vterm->colors,NULL);
    }

    return;
}

