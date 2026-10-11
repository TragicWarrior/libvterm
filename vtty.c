
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <signal.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>

#ifdef __linux__                                // forkpty() in linux
#include <pty.h>
#include <utmp.h>
#endif

#ifdef __FreeBSD__                              // forkpty() in freebsd
#include <sys/ioctl.h>
#include <libutil.h>
#endif

#if defined(__APPLE__) && defined(__MACH__)     // forkpty() in osx
#include <util.h>
#else
#include <term.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>


#include "stringv.h"
#include "vtty.h"
#include "vtty_private.h"
#include "vtty_write.h"
#include "vtty_exec.h"
#include "vtty_escape.h"
#include "vtty_buffer.h"
#include "vtty_csi.h"
#include "color_cache.h"
#include "mouse_driver.h"

#include "macros.h"
#include "utlist.h"

/*
    as a shared library, it's necssary for all the terminals to be able to
    share color palette and mouse state information because these are
    shared resources.
*/
color_cache_t   *color_cache = NULL;

void
_vtty_set_guest_env(vtty_t *vtty);

void
_vtty_set_host_env(vtty_t *vtty);


vtty_t*
vtty_alloc(void)
{
    vtty_t *vtty;

    vtty = (vtty_t*)calloc(1, sizeof(vtty_t));
    if(vtty != NULL)
        vtty_esbuf_init(vtty);

    return  vtty;
}

vtty_t*
vtty_init(vtty_t *vtty, uint16_t width, uint16_t height, uint32_t flags)
{
    pid_t                   child_pid = 0;
    int                     master_fd;
    struct winsize          ws = {.ws_xpixel = 0, .ws_ypixel = 0};
    char                    *pos = NULL;
    int                     retval;
    int                     fd_flags;
    // xterm emulation is the default if none specified
    if((flags & VTTY_TERM_MASK) == 0) flags |= VTTY_FLAG_XTERM_256;

#ifdef NOCURSES
    flags = flags | VTTY_FLAG_NOCURSES;
#endif
#ifdef NOUTF8
    flags = flags | VTTY_FLAG_NOUTF8;
#endif

    if(height <= 0 || width <= 0) return NULL;

    // a new instance
    if(vtty == NULL)
    {
        vtty = (vtty_t*)calloc(1, sizeof(vtty_t));
        if(vtty == NULL) return NULL;
        vtty_esbuf_init(vtty);
    }
    else if(vtty->esbuf == NULL)
    {
        vtty_esbuf_init(vtty);
    }

    // seed the active descriptor cache.  vtty_desc_idx defaults to 0
    // (VTTY_BUF_STANDARD) from calloc; the inline array address is stable
    // for the lifetime of the vtty_t.
    vtty->v_desc_active = &vtty->vtty_desc[VTTY_BUF_STANDARD];

    // allocate a the buffer (a matrix of cells)
    vtty_buffer_alloc(vtty, VTTY_BUF_STANDARD, width, height);

    /*
        allocate the scrollback buffer to 4x height and erase the buffer
        with all blanks.
    */
    vtty_buffer_alloc(vtty, VTTY_BUF_HISTORY, width, height * 4);
    vtty_erase(vtty, VTTY_BUF_HISTORY, '-');

    // initializes the color cache or updates the ref count
    color_cache_init();

    // default active colors
    // uses ncurses macros even if we aren't using ncurses.
    // it is just a bit mask/shift operation.
    if(flags & VTTY_FLAG_NOCURSES)
    {
        // todo:  gracefully handle NOCURSES
    }
    else
    {
        vtty->vtty_desc[0].curattr = 0;
    }

    // initialize all cells with defaults
    vtty_erase(vtty, VTTY_BUF_STANDARD, 0);

    if(flags & VTTY_FLAG_DUMP)
    {
        // setup the base filepath for the dump file
        vtty->debug_filepath = (char*)calloc(PATH_MAX + 1,sizeof(char));
        if( getcwd(vtty->debug_filepath, PATH_MAX) == NULL )
        {
            fprintf(stderr,"ERROR: Failed to getcwd()\n");
            pos = 0;
        }
        else
        {
            pos = vtty->debug_filepath;
            pos += strlen(vtty->debug_filepath);
            if (pos[0] != '/')
            {
                pos[0] = '/';
                pos++;
            }
        }
    }

    vtty->flags = flags;

    memset(&ws, 0, sizeof(ws));
    ws.ws_row = height;
    ws.ws_col = width;

    /*
        rxvt has a crazy escape sequence for resetting the terminal.
        others typically use \Ec
    */
    if(flags & VTTY_FLAG_RXVT)
    {
        vtty->rs1_reset = interpret_csi_RS1_rxvt;
    }

    if(flags & VTTY_FLAG_NOPTY)
    { // skip all the child process and fd stuff.
    }
    else
    {
        child_pid = forkpty(&master_fd, NULL, NULL, &ws);

        if(child_pid < 0)
        {
            /* no pty was made: master_fd is not a descriptor, and
               vtty_destroy must not close whatever number it holds */
            vtty->pty_fd = -1;
            vtty_destroy(vtty);
            exit(EXIT_FAILURE);
        }

        vtty->pty_fd = master_fd;

        if(child_pid == 0)
        {
            signal(SIGINT, SIG_DFL);

            _vtty_set_guest_env(vtty);

            retval = vtty_exec_binary(vtty);

            if(retval == -1) exit(EXIT_FAILURE);

            exit(EXIT_SUCCESS);
        }

        vtty->child_pid = child_pid;
        _vtty_set_host_env(vtty);

        if(flags & VTTY_FLAG_DUMP)
        {
            sprintf(pos, "vtty-%d.dump", child_pid);
            vtty->debug_fd = open(vtty->debug_filepath,
                O_CREAT | O_TRUNC | O_WRONLY, S_IWUSR | S_IRUSR);
        }

        if(ttyname_r(master_fd,vtty->ttyname,sizeof(vtty->ttyname) - 1) != 0)
        {
            snprintf(vtty->ttyname,sizeof(vtty->ttyname) - 1, "vtty");
        }
    }

    /* status flags (O_NONBLOCK), not FD flags (FD_CLOEXEC).  Was F_SETFD,
       which silently no-op'd the nonblock request and left the master
       blocking.  Nonblocking + write-all in _vtty_write_pty keeps paste
       and keystroke writes from hanging the host while still completing. */
    fd_flags = fcntl(vtty->pty_fd, F_GETFL);
    if(fd_flags != -1)
        fcntl(vtty->pty_fd, F_SETFL, fd_flags | O_NONBLOCK);

    use_extended_names(TRUE);
    vtty->write = vtty_write_keymap;

    vtty->read_buf = (char *)calloc(MAX_PIPE_READ + 10, sizeof(char));

    return vtty;
}

void
vtty_destroy(vtty_t *vtty)
{
    int   i;

    if(vtty == NULL) return;

    /* the pty master is ours: forkpty() opened it in vtty_create and
       nothing else closes it.  Left open, every destroyed terminal
       leaked one descriptor (and kept its pty alive) for the life of
       the host.  Not in NOPTY mode -- there pty_fd was never assigned
       and still holds calloc's 0, which is stdin. */
    if(!(vtty->flags & VTTY_FLAG_NOPTY) && vtty->pty_fd >= 0)
    {
        close(vtty->pty_fd);
        vtty->pty_fd = -1;
    }

    /* likewise the dump file opened under VTTY_FLAG_DUMP, and the path
       buffer it was built in */
    if(vtty->flags & VTTY_FLAG_DUMP)
    {
        if(vtty->debug_fd > 0) close(vtty->debug_fd);
        free(vtty->debug_filepath);
    }

    color_cache_free_pairs(vtty);
    vtty_free_mapped_colors(vtty);
    color_cache_release();

    // unload the mouse driver on exit
    mouse_driver_free(vtty);

    // todo:  do something more elegant in the future
    for(i = 0; i < 3; i++)
    {
        vtty_buffer_dealloc(vtty, i);
    }

    free(vtty->read_buf);
    free(vtty->title);
    free(vtty->clipboard);

    /* the program path and argument list copied by vtty_set_exec.
       Both are NULL when it was never called (free and strfreev take
       that). */
    free(vtty->exec_path);
    strfreev(vtty->exec_argv);

    vtty_esbuf_reset(vtty);

    free(vtty);

    vtty = NULL;

    return;
}

pid_t
vtty_get_pid(vtty_t *vtty)
{
    if(vtty == NULL) return -1;

    return vtty->child_pid;
}

int
vtty_get_pty_fd(vtty_t *vtty)
{
    if(vtty == NULL) return -1;

    return vtty->pty_fd;
}

const char*
vtty_get_ttyname(vtty_t *vtty)
{
   if(vtty == NULL) return NULL;

   return (const char*)vtty->ttyname;
}

void
_vtty_set_host_env(vtty_t *vtty)
{
    int     term_colors = 0;
    int     broken_bits = 0;

    term_colors = tigetnum("colors");
    broken_bits = tigetnum("ncv");

    // bad value passed.  fallback to normal xterm mode
    if(term_colors < 16)
    {
        vtty->flags &= ~VTTY_FLAG_XTERM_256;
        vtty->flags |= VTTY_FLAG_XTERM;
    }

    // probably not safe to use 16-colors
    if(broken_bits > 0)
    {
        if((term_colors < 16) && (broken_bits & 0x10))
        {
            vtty->flags |= VTTY_FLAG_C8;
            vtty->flags &= ~VTTY_FLAG_C16;
        }
    }

    return;
}

void
_vtty_set_guest_env(vtty_t *vtty)
{
    int     term_colors = 0;

    term_colors = tigetnum("colors");

    if(vtty->flags & VTTY_FLAG_RXVT)
    {
        setenv("TERM", "rxvt", 1);
        setenv("COLORTERM", "rxvt", 1);
    }

    if(vtty->flags & VTTY_FLAG_XTERM)
    {
        setenv("TERM", "xterm", 1);

        if(vtty->flags & VTTY_FLAG_TRUECOLOR)
            setenv("COLORTERM", "truecolor", 1);
        else
            setenv("COLORTERM", "xterm", 1);
    }

    if(vtty->flags & VTTY_FLAG_XTERM_256)
    {
        setenv("TERM", "xterm", 1);

#ifndef __FreeBSD__
        if(term_colors == 16)
            setenv("TERM", "xterm-16color", 1);

        if(term_colors > 16)
            // setenv("TERM", "xterm-direct", 1);
            setenv("TERM", "xterm-256color", 1);
#endif

        if(vtty->flags & VTTY_FLAG_TRUECOLOR)
            setenv("COLORTERM", "truecolor", 1);
        else
            setenv("COLORTERM", "xterm", 1);
    }

    if(vtty->flags & VTTY_FLAG_LINUX)
    {
        setenv("TERM", "linux", 1);
        unsetenv("COLORTERM");
    }

    if(vtty->flags & VTTY_FLAG_VT100)
    {
        setenv("TERM", "vt100", 1);
        unsetenv("COLORTERM");
    }

    return;
}

