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

/* interprets an 'erase display' (ED) escape sequence */
void
interpret_csi_ED(vterm_t *vterm, int param[], int pcount)
{
    vterm_desc_t    *v_desc = NULL;
    int             r, c;
    int             start_row, start_col, end_row, end_col;
    int             idx;

    // set vterm desc buffer selector
    idx = vterm_get_active_buffer(vterm);
    v_desc = &vterm->vterm_desc[idx];

    /* decide range */
    if(pcount && param[0] == 2)
    {
        start_row = 0;
        start_col = 0;
        end_row = v_desc->rows - 1;
        end_col = v_desc->cols - 1;
    }
    else if(pcount && param[0] == 1)
    {
        start_row = 0;
        start_col = 0;
        end_row = v_desc->crow;
        end_col = v_desc->ccol;
    }
    else
    {
        start_row = v_desc->crow;
        start_col = v_desc->ccol;
        end_row = v_desc->rows - 1;
        end_col = v_desc->cols-1;
    }

    /* clean range */
    for(r = start_row;r <= end_row; r++)
    {
        for(c = start_col; c <= end_col; c++)
        {
            // erase with blanks.
            VCELL_SET_CHAR(v_desc->cells[r][c], ' ');

            // set to current attributes.
            VCELL_SET_ATTR(v_desc->cells[r][c], v_desc->curattr);
        }
    }
}
