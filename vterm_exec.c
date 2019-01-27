
#include <string.h>
#include <stdlib.h>
#include <pwd.h>

#include "vterm.h"
#include "vterm_private.h"
#include "vterm_exec.h"
#include "stringv.h"

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

