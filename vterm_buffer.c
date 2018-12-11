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

#include <stdlib.h>

#include "vterm.h"
#include "vterm_private.h"
#include "vterm_buffer.h"

void
vterm_alloc_buffer(vterm_t *vterm, int idx, int width, int height)
{
    int i;

    if(vterm == NULL) return;
    if(idx != VTERM_BUFFER_STD && idx != VTERM_BUFFER_ALT) return;

    if(width < 0 || height < 0) return;

    vterm->vterm_desc[idx].cells =
        (vterm_cell_t**)calloc(1, (sizeof(vterm_cell_t*) * height));

    for(i = 0;i < height;i++)
    {
        vterm->vterm_desc[idx].cells[i] =
            (vterm_cell_t*)calloc(1, (sizeof(vterm_cell_t) * width));
    }

    return;
}

void
vterm_dealloc_buffer(vterm_t *vterm, int idx)
{
    vterm_desc_t    *vterm_desc;
    int             i;

    if(vterm == NULL) return;
    if(idx != VTERM_BUFFER_STD && idx != VTERM_BUFFER_ALT) return;

    vterm_desc = &vterm->vterm_desc[idx];

    for(i = 0; i < vterm_desc->rows; i++)
    {
        free(vterm_desc->cells[i]);
        vterm_desc->cells[i] = NULL;
    }

    free(vterm_desc->cells);
    vterm_desc->cells = NULL;

    return;
}

int
vterm_set_active_buffer(vterm_t *vterm, int idx)
{
    int     curr_idx;

    if(vterm == NULL) return -1;

    if(idx != VTERM_BUFFER_STD && idx != VTERM_BUFFER_ALT) return -1;

    curr_idx = vterm_get_active_buffer(vterm);

    return 0;
}

int
vterm_get_active_buffer(vterm_t *vterm)
{
    if(vterm == NULL) return -1;

    return vterm->vterm_desc_idx;
}

