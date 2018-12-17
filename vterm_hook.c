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

Copyright (c) 2009 Bryan Christ
*/

#include "vterm.h"
#include "vterm_private.h"

void
vterm_install_hook(vterm_t *vterm, vterm_hook_t hook)
{
    if(vterm == NULL) return;

    vterm->event_hook = hook;

    return;
}
