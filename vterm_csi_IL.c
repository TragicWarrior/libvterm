/*
LICENSE INFORMATION:
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License (LGPL) as published by the Free Software Foundation.

Please refer to the COPYING file for more information.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

Copyright (c) 2004 Bruno T. C. de Oliveira
*/

#include <string.h>

#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_buffer.h"

/* Interpret an 'insert line' sequence (IL) */
void
interpret_csi_IL(vterm_t *vterm,int param[],int pcount)
{
    vterm_cell_t    *vcell;
    vterm_desc_t    *v_desc = NULL;
    int             i, j;
    int             n = 1;
    int             idx;

    // set vterm description buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    if(pcount && param[0] > 0) n = param[0];

    for(i = v_desc->scroll_max; i >= v_desc->crow + n; i--)
    {
        memcpy(v_desc->cells[i], v_desc->cells[i - n],
            sizeof(vterm_cell_t) * v_desc->cols);
    }

    for(i = v_desc->crow; i < v_desc->crow + n; i++)
    {
        if(i > v_desc->scroll_max) break;

        for(j = 0;j < v_desc->cols; j++)
        {
            // store cell location so to miminize scalar look-ups
            vcell = &v_desc->cells[i][j];

            VCELL_SET_CHAR((*vcell), ' ');
            VCELL_SET_ATTR((*vcell), v_desc->curattr);
        }
    }

    return;
}
