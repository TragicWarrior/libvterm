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

#include <string.h>

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

// todo:  fix this up later to implement getcchar() and setcchar()

#define VCELL_ZERO_ALL(_cell) \
            { memset(&_cell, 0, sizeof(_cell)); }

#define VCELL_SET_CHAR(_cell, _ch) \
            { _cell.sch.ch = _ch; }

#define VCELL_SET_ATTR(_cell, _attr) \
            { _cell.sch.attr = _attr; }

#define VCELL_GET_CHAR(_cell, _ch_ptr) \
            { *_ch_ptr = _cell.sch.ch; }

#define VCELL_GET_ATTR(_cell, _attr_ptr) \
            { *_attr_ptr = _cell.sch.attr; }

#endif

