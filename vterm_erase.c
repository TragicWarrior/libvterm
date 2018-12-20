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

Copyright (c) 2009 Bryan Christ
*/


#include "vterm.h"
#include "vterm_private.h"
#include "vterm_render.h"
#include "vterm_buffer.h"

void
vterm_erase(vterm_t *vterm, int idx)
{
    vterm_cell_t    *vcell;
    vterm_desc_t    *v_desc = NULL;
    int             cell_count;
    int             x, y;
    int             i;

    if(vterm == NULL) return;

    // a value of -1 means current, active buffer
    if(idx == -1)
    {
        // set the vterm description buffer selector
        idx = vterm_buffer_get_active(vterm);
    }

    v_desc = &vterm->vterm_desc[idx];

    cell_count = v_desc-> rows * v_desc->cols;

    for(i = 0;i < cell_count; i++)
    {
        x = i % v_desc->cols;
        y = (int)(i / v_desc->cols);

        // store address of cell to reduce scalar look-ups
        vcell = &v_desc->cells[y][x];

        VCELL_SET_CHAR((*vcell), ' ');
        VCELL_SET_ATTR((*vcell), (COLOR_PAIR(v_desc->colors)));
    }

    return;
}

void
vterm_erase_row(vterm_t *vterm, int row)
{
    vterm_cell_t    *vcell;
    vterm_desc_t    *v_desc = NULL;
    int             i;
    int             idx;

    if(vterm == NULL) return;

    // set the vterm description buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    if(row == -1) row = v_desc->crow;

    for(i = 0;i < v_desc->cols; i++)
    {
        // store the cell address to reduce scalar look-ups
        vcell = &v_desc->cells[row][i];

        VCELL_SET_CHAR((*vcell), ' ');
        VCELL_SET_ATTR((*vcell), (COLOR_PAIR(v_desc->colors)));
    }

    return;
}

void
vterm_erase_rows(vterm_t *vterm, int start_row)
{
    vterm_desc_t    *v_desc = NULL;
    int             idx;

    if(vterm == NULL) return;
    if(start_row < 0) return;

    // set the vterm description buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    while(start_row < v_desc->rows)
    {
        vterm_erase_row(vterm,start_row);
        start_row++;
    }

    return;
}

void
vterm_erase_col(vterm_t *vterm, int col)
{
    vterm_cell_t    *vcell;
    vterm_desc_t    *v_desc = NULL;
    int             idx;
    int             i;

    if(vterm == NULL) return;

    // set the vterm description buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    if(col == -1) col = v_desc->ccol;

    for(i = 0;i < v_desc->rows; i++)
    {
        // store the cell address to reduce scalar look-ups
        vcell = &v_desc->cells[i][col];

        VCELL_SET_CHAR((*vcell), ' ');
        VCELL_SET_ATTR((*vcell), (COLOR_PAIR(v_desc->colors)));
    }

    return;
}

void
vterm_erase_cols(vterm_t *vterm,int start_col)
{
    vterm_desc_t    *v_desc = NULL;
    int             idx;

    if(vterm == NULL) return;
    if(start_col < 0) return;

    // set the vterm description buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    while(start_col < v_desc->cols)
    {
        vterm_erase_col(vterm, start_col);
        start_col++;
    }

    return;
}

