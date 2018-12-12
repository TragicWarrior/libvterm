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
#include "vterm_private.h"
#include "vterm_buffer.h"

inline void
clamp_cursor_to_bounds(vterm_t *vterm)
{
    vterm_desc_t    *v_desc = NULL;
    int             idx;

    // set vterm description buffer seletor
    idx = vterm_get_active_buffer(vterm);
    v_desc = &vterm->vterm_desc[idx];

    if(v_desc->crow < 0) v_desc->crow = 0;

    if(v_desc->ccol < 0) v_desc->ccol = 0;

    if(v_desc->crow >= v_desc->rows) v_desc->crow = v_desc->rows - 1;

    if(v_desc->ccol >= v_desc->cols) v_desc->ccol = v_desc->cols - 1;

    return;
}

