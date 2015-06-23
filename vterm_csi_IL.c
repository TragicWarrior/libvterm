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

/* Interpret an 'insert line' sequence (IL) */
void interpret_csi_IL(vterm_t *vterm,int param[],int pcount)
{
   int i, j;
   int n=1;

   if(pcount && param[0] > 0) n=param[0];

   for(i=vterm->scroll_max;i >= vterm->crow+n;i--)
   {
      memcpy(vterm->cells[i],vterm->cells[i - n],
         sizeof(vterm_cell_t)*vterm->cols);
   }

   for(i=vterm->crow;i < vterm->crow+n; i++)
   {
      if(i>vterm->scroll_max) break;

      // vterm->dirty_lines[i]=TRUE;

      for(j=0;j < vterm->cols; j++)
      {
         vterm->cells[i][j].ch = 0x20;
         vterm->cells[i][j].attr=vterm->curattr;
      }
   }

   return;
}
