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
#include "vterm_misc.h"

/* Interpret the 'relative mode' sequences: CUU, CUD, CUF, CUB, CNL,
 * CPL, CHA, HPR, VPA, VPR, HPA */
void interpret_csi_CUx(vterm_t *vterm,char verb,int param[],int pcount)
{
   int n=1;

   if(pcount && param[0]>0) n=param[0];

   switch (verb)
   {
      case 'A':         vterm->crow -= n;          break;
      case 'B':
      case 'e':         vterm->crow += n;          break;
      case 'C':
      case 'a':         vterm->ccol += n;          break;
      case 'D':         vterm->ccol -= n;          break;
      case 'E':
      {
         vterm->crow += n;
         vterm->ccol = 0;
         break;
      }
      case 'F':
      {
         vterm->crow -= n;
         vterm->ccol = 0;
         break;
      }
      case 'G':
      case '`':         vterm->ccol=param[0]-1;    break;
      case 'd':         vterm->crow=param[0]-1;    break;
   }

   // vterm->state |= STATE_DIRTY_CURSOR;
   clamp_cursor_to_bounds(vterm);

   return;
}
