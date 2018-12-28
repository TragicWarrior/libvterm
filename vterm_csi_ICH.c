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
    vterm_cell_t    *vcell;
    vterm_desc_t    *v_desc = NULL;
    int             c;
    int             n = 1;
    int             idx;
    int             max_stride;
    int             max_col;
    int             scr_end;

    // set the vterm desciption selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    if(pcount && param[0] > 0) n = param[0];

    /*
        ICH has no effect beyond the edge so calculate the maximum stride
        possible and clamp 'n' to that value if ncessary.
    */
    max_stride = v_desc->cols - v_desc->ccol;
    if(n > max_stride) n = max_stride;

    /*
        calculate where the maximum column would be which is current
        column + n (the stride).
    */
    max_col = v_desc->ccol + n;

    // zero-based index of last column
    scr_end = v_desc->cols - 1;

    for(c = scr_end; c >= v_desc->ccol + n; c--)
    {
        v_desc->cells[v_desc->crow][c] = v_desc->cells[v_desc->crow][c - n];
    }

    vcell = &v_desc->cells[v_desc->crow][0];
    for(c = v_desc->ccol; c < max_col; c++)
    {
        VCELL_SET_CHAR((*vcell), ' ');
        VCELL_SET_ATTR((*vcell), v_desc->curattr);

        vcell++;
    }

    return;
}


