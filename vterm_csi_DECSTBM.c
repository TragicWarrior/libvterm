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


/* Interpret a 'set scrolling region' (DECSTBM) sequence */
void
interpret_csi_DECSTBM(vterm_t *vterm,int param[],int pcount)
{
    vterm_desc_t    *v_desc = NULL;
    int             newtop, newbottom;
    int             idx;

    // set the vterm description selector
    idx = vterm_get_active_buffer(vterm);
    v_desc = &vterm->vterm_desc[idx];

    if(!pcount)
    {
        newtop = 0;
        newbottom = v_desc->rows-1;
    }
    else if(pcount < 2) return; /* malformed */
    else
    {
        newtop = param[0] - 1;
        newbottom = param[1] - 1;
    }

    /* clamp to bounds */
    if(newtop < 0) newtop = 0;
    if(newtop >= v_desc->rows) newtop = v_desc->rows - 1;
    if(newbottom < 0) newbottom = 0;
    if(newbottom >= v_desc->rows) newbottom = v_desc->rows - 1;

    /* check for range validity */
    if(newtop > newbottom) return;
    v_desc->scroll_min = newtop;
    v_desc->scroll_max = newbottom;

    if(v_desc->scroll_min != 0)
        v_desc->buffer_state |= STATE_SCROLL_SHORT;

    if(v_desc->scroll_max != v_desc->rows - 1)
        v_desc->buffer_state |= STATE_SCROLL_SHORT;

    if((v_desc->scroll_min == 0) && (v_desc->scroll_max == v_desc->rows - 1))
        v_desc->buffer_state &= ~STATE_SCROLL_SHORT;

   return;
}
