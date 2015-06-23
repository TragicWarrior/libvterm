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

/* Interpret a 'delete line' sequence (DL) */
void interpret_csi_DL(vterm_t *vterm,int param[],int pcount)
{
   int i, j;
   int n=1;

   if(pcount && param[0] > 0) n=param[0];

   for(i=vterm->crow;i <= vterm->scroll_max; i++)
   {
      if(i+n <= vterm->scroll_max)
      {
         memcpy(vterm->cells[i],vterm->cells[i+n],
            sizeof(vterm_cell_t)*vterm->cols);
      }
      else
      {
         for(j=0;j < vterm->cols;j++)
         {
            vterm->cells[i][j].ch=0x20;
            vterm->cells[i][j].attr=vterm->curattr;
         }
      }
   }

   return;
}
