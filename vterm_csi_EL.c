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

/* Interpret the 'erase line' escape sequence */
void interpret_csi_EL(vterm_t *vterm, int param[], int pcount)
{
   int erase_start, erase_end, i;
   int cmd=0;

   if(pcount>0) cmd=param[0];

   switch(cmd)
   {
      case 1:
      {
         erase_start=0;
         erase_end=vterm->ccol;
         break;
      }
      case 2:
      {
         erase_start=0;
         erase_end=vterm->cols-1;
         break;
      }
      default:
      {
         erase_start=vterm->ccol;
         erase_end=vterm->cols-1;
         break;
      }
   }

   for(i=erase_start;i <= erase_end;i++)
   {
      vterm->cells[vterm->crow][i].ch = 0x20; 
      vterm->cells[vterm->crow][i].attr = vterm->curattr;
   }

   return;
}

