#ifndef _VTERM_UTF8_H_
#define _VTERM_UTF8_H_

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

#include <curses.h>

#include "vterm.h"

void    vterm_utf8_start(vterm_t *vterm);
void    vterm_utf8_cancel(vterm_t *vterm);

int     vterm_utf8_decode(vterm_t *vterm, chtype *utf8_char);

#endif
