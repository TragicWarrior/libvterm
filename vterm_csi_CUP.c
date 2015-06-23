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

/* interprets a 'move cursor' (CUP) escape sequence */
void interpret_csi_CUP(vterm_t *vterm, int param[], int pcount)
{
   if (pcount == 0)
   {
      /* special case */
      vterm->crow=0;
      vterm->ccol=0;
      return;
   }
   else if (pcount < 2) return;  // malformed

   vterm->crow=param[0]-1;       // convert from 1-based to 0-based.
   vterm->ccol=param[1]-1;       // convert from 1-based to 0-based.

   // vterm->state |= STATE_DIRTY_CURSOR;

   clamp_cursor_to_bounds(vterm);
}

