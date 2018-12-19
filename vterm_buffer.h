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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 
    02110-1301, USA.
*/

/*
    This library is based on ROTE written by Bruno Takahashi C. de Oliveira
*/

#ifndef _VTERM_BUFFER_H_
#define _VTERM_BUFFER_H_

#include "vterm.h"

enum
{
    VTERM_BUFFER_STD    =   0x00,
    VTERM_BUFFER_ALT,
};

void    vterm_alloc_buffer(vterm_t *vterm, int idx, int width, int height);

void    vterm_realloc_buffer(vterm_t *vterm, int idx, int width, int height);

void    vterm_dealloc_buffer(vterm_t *vterm, int idx);

int     vterm_set_active_buffer(vterm_t *vterm, int idx);

int     vterm_get_active_buffer(vterm_t *vterm);

#endif

