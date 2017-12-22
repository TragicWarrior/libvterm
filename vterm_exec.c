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

#include <string.h>
#include <stdlib.h>
#include <pwd.h>

#include "vterm.h"
#include "vterm_private.h"
#include "vterm_exec.h"
#include "strings.h"

void
vterm_set_exec(vterm_t *vterm, char *path, char **exec_argv)
{
    if(vterm == NULL) return;
    if(path == NULL) return;
    if(exec_argv == NULL) return;

    if(vterm->exec_path != NULL)
    {
        free(path);
        path = NULL;
    }

    if(vterm->exec_argv != NULL)
    {
        strfreev(vterm->exec_argv);
        vterm->exec_argv = NULL;
    }

    vterm->exec_path = strdup(path);
    vterm->exec_argv = strdupv(exec_argv, -1);

    return;
}

int
vterm_exec_binary(vterm_t *vterm)
{
    struct passwd   *user_profile;
    char            *user_shell = NULL;

    if(vterm->exec_path == NULL)
    {
        user_profile = getpwuid(getuid());
        if(user_profile == NULL) user_shell = "/bin/sh";
        else if(user_profile->pw_shell == NULL) user_shell = "/bin/sh";
        else user_shell = user_profile->pw_shell;

        if(user_shell == NULL) user_shell="/bin/sh";

        // start the shell
        if(execl(user_shell, user_shell, NULL) == -1)
        {
            return -1;
        }

        return 0;
    }

    // caller provided an alternate binary to run
    if(execv(vterm->exec_path, vterm->exec_argv) == -1)
    {
        return -1;
    }

    return 0;
}

