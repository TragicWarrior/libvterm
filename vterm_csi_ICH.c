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
#include "vterm_csi.h"
#include "vterm_buffer.h"

/* Interpret the 'insert blanks' sequence (ICH) */
void
interpret_csi_ICH(vterm_t *vterm, int param[], int pcount)
{
    vterm_desc_t    *v_desc = NULL;
    int             i;
    int             n = 1;
    int             idx;

    // set the vterm desciption selector
    idx = vterm_get_active_buffer(vterm);
    v_desc = &vterm->vterm_desc[idx];

    if(pcount && param[0] > 0) n = param[0];

    for(i = v_desc->cols - 1; i >= v_desc->ccol + n; i--)
    {
        v_desc->cells[v_desc->crow][i] = v_desc->cells[v_desc->crow][i - n];
    }

    for(i = v_desc->ccol; i < v_desc->ccol + n; i++)
    {
        v_desc->cells[v_desc->crow][i].ch = 0x20;
        v_desc->cells[v_desc->crow][i].attr = v_desc->curattr;
    }

    return;
}
