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

#include <string.h>
#include <fcntl.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>

void vterm_scroll_down(vterm_t *vterm)
{
   int i;

   vterm->crow++;

   if(vterm->crow <= vterm->scroll_max) return;

   /* must scroll the scrolling region up by 1 line, and put cursor on 
    * last line of it */
   vterm->crow=vterm->scroll_max;

   for(i=vterm->scroll_min; i < vterm->scroll_max; i++)
   {
      // vterm->dirty_lines[i]=TRUE;
      memcpy(vterm->cells[i],vterm->cells[i+1],
         sizeof(vterm_cell_t)*vterm->cols);
   }

   /* clear last row of the scrolling region */
   vterm_erase_row(vterm,vterm->scroll_max);

   return;
}

void vterm_scroll_up(vterm_t *vterm)
{
   int i;

   vterm->crow--;

   if(vterm->crow >= vterm->scroll_min) return;

   /* must scroll the scrolling region up by 1 line, and put cursor on 
    * first line of it */
   vterm->crow=vterm->scroll_min;

   for(i=vterm->scroll_max;i > vterm->scroll_min;i--)
   {
      // vterm->dirty_lines[i]=TRUE;
      memcpy(vterm->cells[i],vterm->cells[i-1],
         sizeof(vterm_cell_t)*vterm->cols);
   }

   /* clear first row of the scrolling region */
   vterm_erase_row(vterm,vterm->scroll_min);

   return;
}
