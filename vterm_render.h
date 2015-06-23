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

#ifndef _VTERM_RENDER_H_
#define _VTERM_RENDER_H_

#include <curses.h>

#include "vterm.h"

void vterm_render(vterm_t *,const char *data,int len);
void vterm_put_char(vterm_t *vterm,chtype c);
void vterm_render_ctrl_char(vterm_t *vterm,char c);
void try_interpret_escape_seq(vterm_t *vterm);

#endif

