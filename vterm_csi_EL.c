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


/*
    From VT100 User Guide

    ESC [ Ps K

    default value: 0
    Erases some or all characters in the active line
    according to the parameter. Editor Function
    Parameter Parameter Meaning

    0   Erase from the active position to the end of the
        line, inclusive (default)

    1   Erase from the start of the screen to the active
        position, inclusive

    2   Erase all of the line, inclusive
*/

/* Interpret the 'erase line' escape sequence */
void
interpret_csi_EL(vterm_t *vterm, int param[], int pcount)
{
    vterm_cell_t    *vcell;
    vterm_desc_t    *v_desc = NULL;
    int             erase_start, erase_end, i;
    int             cmd = 0;
    int             idx;

    // set the vterm description buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    if(pcount > 0) cmd = param[0];

    switch(cmd)
    {
        case 1:
        {
            erase_start = 0;
            erase_end = v_desc->ccol;
            break;
        }

        case 2:
        {
            erase_start = 0;
            erase_end = v_desc->cols - 1;
            break;
        }

        case 0:
        default:
        {
            erase_start = v_desc->ccol;
            erase_end = v_desc->cols - 1;
            break;
        }
    }

    for(i = erase_start; i <= erase_end; i++)
    {
        vcell = &v_desc->cells[v_desc->crow][i];

        VCELL_SET_CHAR((*vcell), ' ');
        VCELL_SET_ATTR((*vcell), v_desc->curattr);
    }

    return;
}

