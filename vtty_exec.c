
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>

#include "vtty.h"
#include "vtty_private.h"
#include "vtty_exec.h"
#include "stringv.h"

void
vtty_set_exec(vtty_t *vtty, char *path, char **exec_argv)
{
    if(vtty == NULL) return;
    if(path == NULL) return;
    if(exec_argv == NULL) return;

    if(vtty->exec_path != NULL)
    {
        free(path);
        path = NULL;
    }

    if(vtty->exec_argv != NULL)
    {
        strfreev(vtty->exec_argv);
        vtty->exec_argv = NULL;
    }

    vtty->exec_path = strdup(path);
    vtty->exec_argv = strdupv(exec_argv, -1);

    return;
}

/*
    child-side only (post-forkpty).  when VTTY_FLAG_START_HOME is set,
    move into the user's home before exec so the shell or alternate
    binary does not inherit the host process cwd.
*/
static void
_vtty_chdir_home(void)
{
    const char      *home;
    struct passwd   *pw;

    home = getenv("HOME");
    if(home == NULL || home[0] == '\0')
    {
        pw = getpwuid(getuid());
        if(pw != NULL) home = pw->pw_dir;
    }

    if(home != NULL && home[0] != '\0')
    {
        if(chdir(home) == -1)
            return;
    }

    return;
}

int
vtty_exec_binary(vtty_t *vtty)
{
    struct passwd   *user_profile;
    char            *user_shell = NULL;

    if(vtty->flags & VTTY_FLAG_START_HOME)
        _vtty_chdir_home();

    if(vtty->exec_path == NULL)
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
    if(execv(vtty->exec_path, vtty->exec_argv) == -1)
    {
        return -1;
    }

    return 0;
}

