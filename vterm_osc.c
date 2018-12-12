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
#include <string.h>

#include "vterm.h"
#include "vterm_private.h"

void
vterm_get_title(vterm_t *vterm, char *buf, int buf_sz)
{
    if(vterm == NULL) return;
    if(buf == NULL) return;
    if(buf_sz < 2) return;

    memset(buf, 0, buf_sz);
    strncpy(buf, vterm->title, buf_sz - 1);

    return;
}
