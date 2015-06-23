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


/* Interpret a 'set scrolling region' (DECSTBM) sequence */
void interpret_csi_DECSTBM(vterm_t *vterm,int param[],int pcount)
{
   int newtop, newbottom;

   if(!pcount)
   {
      newtop=0;
      newbottom=vterm->rows-1;
   }
   else if(pcount < 2) return; /* malformed */
   else
   {
      newtop=param[0]-1;
      newbottom=param[1]-1;
   }

   /* clamp to bounds */
   if(newtop < 0) newtop=0;
   if(newtop >= vterm->rows) newtop=vterm->rows-1;
   if(newbottom < 0) newbottom=0;
   if(newbottom >= vterm->rows) newbottom=vterm->rows-1;

   /* check for range validity */
   if(newtop > newbottom) return;
   vterm->scroll_min=newtop;
   vterm->scroll_max=newbottom;

   if(vterm->scroll_min != 0)
      vterm->state |= STATE_SCROLL_SHORT;

   if(vterm->scroll_max != vterm->rows-1)
      vterm->state |= STATE_SCROLL_SHORT;

   if((vterm->scroll_min == 0) && (vterm->scroll_max == vterm->rows-1))
      vterm->state &= ~STATE_SCROLL_SHORT;

   return;
}

