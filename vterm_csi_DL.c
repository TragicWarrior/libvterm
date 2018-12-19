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

#include <string.h>

#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_buffer.h"

/* Interpret a 'delete line' sequence (DL) */
void
interpret_csi_DL(vterm_t *vterm, int param[], int pcount)
{
    vterm_desc_t    *v_desc = NULL;
    int             i, j;
    int             n = 1;
    int             idx;

    // set selector for buffer description
    idx = vterm_get_active_buffer(vterm);
    v_desc = &vterm->vterm_desc[idx];

    if(pcount && param[0] > 0) n = param[0];

    for(i = v_desc->crow; i <= v_desc->scroll_max; i++)
    {
        if(i + n <= v_desc->scroll_max)
        {
            memcpy(v_desc->cells[i], v_desc->cells[i + n],
                sizeof(vterm_cell_t) * v_desc->cols);
        }
        else
        {
            for(j = 0; j < v_desc->cols; j++)
            {
                VCELL_SET_CHAR(v_desc->cells[i][j], ' ');
                VCELL_SET_ATTR(v_desc->cells[i][j], v_desc->curattr);
            }
        }
    }

    return;
}
