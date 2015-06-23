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
#include "vterm_misc.h"

void clamp_cursor_to_bounds(vterm_t *vterm)
{
    if(vterm->crow < 0) vterm->crow = 0;

    if(vterm->ccol < 0) vterm->ccol = 0;

    if(vterm->crow >= vterm->rows) vterm->crow = vterm->rows - 1;

    if(vterm->ccol >= vterm->cols) vterm->ccol = vterm->cols - 1;

    return;
}

