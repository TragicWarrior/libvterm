
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


#include "vterm.h"
#include "vterm_private.h"
#include "vterm_write.h"
#include "vterm_exec.h"
#include "vterm_buffer.h"
#include "vterm_csi.h"
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
_vterm_set_guest_env(vterm_t *vterm);

void
_vterm_set_host_env(vterm_t *vterm);


vterm_t*
vterm_alloc(void)
{
    vterm_t *vterm;

    vterm = (vterm_t*)calloc(1, sizeof(vterm_t));

    return  vterm;
}

vterm_t*
vterm_init(vterm_t *vterm, uint16_t width, uint16_t height, uint32_t flags)
{
    pid_t                   child_pid = 0;
    int                     master_fd;
    struct winsize          ws = {.ws_xpixel = 0, .ws_ypixel = 0};
    char                    *pos = NULL;
    int                     retval;
    int                     fd_flags;
    // xterm emulation is the default if none specified
    if((flags & VTERM_TERM_MASK) == 0) flags |= VTERM_FLAG_XTERM_256;

#ifdef NOCURSES
    flags = flags | VTERM_FLAG_NOCURSES;
#endif
#ifdef NOUTF8
    flags = flags | VTERM_FLAG_NOUTF8;
#endif

    if(height <= 0 || width <= 0) return NULL;

    // a new instance
    if(vterm == NULL)
        vterm = (vterm_t*)calloc(1, sizeof(vterm_t));

    // allocate a the buffer (a matrix of cells)
    vterm_buffer_alloc(vterm, VTERM_BUF_STANDARD, width, height);

    /*
        allocate the scrollback buffer to 4x height and erase the buffer
        with all blanks.
    */
    vterm_buffer_alloc(vterm, VTERM_BUF_HISTORY, width, height * 4);
    vterm_erase(vterm, VTERM_BUF_HISTORY, '-');

    // initializes the color cache or updates the ref count
    color_cache_init();

    // default active colors
    // uses ncurses macros even if we aren't using ncurses.
    // it is just a bit mask/shift operation.
    if(flags & VTERM_FLAG_NOCURSES)
    {
        // todo:  gracefully handle NOCURSES
    }
    else
    {
        vterm->vterm_desc[0].curattr = 0;
    }

    // initialize all cells with defaults
    vterm_erase(vterm, VTERM_BUF_STANDARD, 0);

    if(flags & VTERM_FLAG_DUMP)
    {
        // setup the base filepath for the dump file
        vterm->debug_filepath = (char*)calloc(PATH_MAX + 1,sizeof(char));
        if( getcwd(vterm->debug_filepath, PATH_MAX) == NULL )
        {
            fprintf(stderr,"ERROR: Failed to getcwd()\n");
            pos = 0;
        }
        else
        {
            pos = vterm->debug_filepath;
            pos += strlen(vterm->debug_filepath);
            if (pos[0] != '/')
            {
                pos[0] = '/';
                pos++;
            }
        }
    }

    vterm->flags = flags;

    memset(&ws, 0, sizeof(ws));
    ws.ws_row = height;
    ws.ws_col = width;

    /*
        rxvt has a crazy escape sequence for resetting the terminal.
        others typically use \Ec
    */
    if(flags & VTERM_FLAG_RXVT)
        vterm->rs1_reset = interpret_csi_RS1_rxvt;

    if(flags & VTERM_FLAG_NOPTY)
    { // skip all the child process and fd stuff.
    }
    else
    {
        child_pid = forkpty(&master_fd, NULL, NULL, &ws);
        vterm->pty_fd = master_fd;

        if(child_pid < 0)
        {
            vterm_destroy(vterm);
            exit(EXIT_FAILURE);
        }

        if(child_pid == 0)
        {
            signal(SIGINT, SIG_DFL);

            _vterm_set_guest_env(vterm);

            retval = vterm_exec_binary(vterm);

            if(retval == -1) exit(EXIT_FAILURE);

            exit(EXIT_SUCCESS);
        }

        vterm->child_pid = child_pid;
        _vterm_set_host_env(vterm);

        if(flags & VTERM_FLAG_DUMP)
        {
            sprintf(pos, "vterm-%d.dump", child_pid);
            vterm->debug_fd = open(vterm->debug_filepath,
                O_CREAT | O_TRUNC | O_WRONLY, S_IWUSR | S_IRUSR);
        }

        if(ttyname_r(master_fd,vterm->ttyname,sizeof(vterm->ttyname) - 1) != 0)
        {
            snprintf(vterm->ttyname,sizeof(vterm->ttyname) - 1, "vterm");
        }
    }

    fd_flags = fcntl(vterm->pty_fd, F_GETFD);
    fcntl(vterm->pty_fd, F_SETFD, fd_flags | O_NONBLOCK);

    use_extended_names(TRUE);
    vterm->write = vterm_write_keymap;

    vterm->read_buf = (char *)calloc(MAX_PIPE_READ + 10, sizeof(char));

    return vterm;
}

void
vterm_destroy(vterm_t *vterm)
{
    int   i;

    if(vterm == NULL) return;

    // DEBUG_COLOR_PAIRS(vterm->color_cache, 20);

/*
    {
        color_pair_t    *pair;
        int             limit = 20;

        CDL_FOREACH(vterm->color_cache->pair_head, pair)
        {
            if(limit == 0) break;

            printf("Pair Num:   %d\n\r", pair->num);
            printf("RGB 256 Fg: r: %d, g: %d, b: %d\n\r",
                pair->rgb_values[0].r / 4,
                pair->rgb_values[0].g / 4,
                pair->rgb_values[0].b / 4);
            printf("HSL fg:     h: %f, s: %f, l: %f\n\r",
                pair->hsl_values[0].h,
                pair->hsl_values[0].s,
                pair->hsl_values[0].l);
            printf("CIE fg:     l: %f, a: %f, b: %f\n\r",
                pair->cie_values[0].l,
                pair->cie_values[0].a,
                pair->cie_values[0].b);
            printf("RGB 256 Bg: r: %d, g: %d, b: %d\n\r",
                pair->rgb_values[1].r / 4,
                pair->rgb_values[1].g / 4,
                pair->rgb_values[1].b / 4);

            limit--;
        }
    }
*/

    color_cache_free_pairs(vterm);
    vterm_free_mapped_colors(vterm);
    color_cache_release();

    // unload the mouse driver on exit
    mouse_driver_free(vterm);

    // todo:  do something more elegant in the future
    for(i = 0; i < 3; i++)
    {
        vterm_buffer_dealloc(vterm, i);
    }

    free(vterm->read_buf);

    free(vterm);

    vterm = NULL;

    return;
}

pid_t
vterm_get_pid(vterm_t *vterm)
{
    if(vterm == NULL) return -1;

    return vterm->child_pid;
}

int
vterm_get_pty_fd(vterm_t *vterm)
{
    if(vterm == NULL) return -1;

    return vterm->pty_fd;
}

const char*
vterm_get_ttyname(vterm_t *vterm)
{
   if(vterm == NULL) return NULL;

   return (const char*)vterm->ttyname;
}

void
_vterm_set_host_env(vterm_t *vterm)
{
    int     term_colors = 0;
    int     broken_bits = 0;

    term_colors = tigetnum("colors");
    broken_bits = tigetnum("ncv");

    // bad value passed.  fallback to normal xterm mode
    if(term_colors < 16)
    {
        vterm->flags &= ~VTERM_FLAG_XTERM_256;
        vterm->flags |= VTERM_FLAG_XTERM;
    }

    // probably not safe to use 16-colors
    if(broken_bits > 0)
    {
        if((term_colors < 16) && (broken_bits & 0x10))
        {
            vterm->flags |= VTERM_FLAG_C8;
            vterm->flags &= ~VTERM_FLAG_C16;
        }
    }

    return;
}

void
_vterm_set_guest_env(vterm_t *vterm)
{
    int     term_colors = 0;

    term_colors = tigetnum("colors");

    if(vterm->flags & VTERM_FLAG_RXVT)
    {
        setenv("TERM", "rxvt", 1);
        setenv("COLORTERM", "rxvt", 1);
    }

    if(vterm->flags & VTERM_FLAG_XTERM)
    {
        setenv("TERM", "xterm", 1);
        setenv("COLORTERM", "xterm", 1);
    }

    if(vterm->flags & VTERM_FLAG_XTERM_256)
    {
        setenv("TERM", "xterm", 1);

#ifndef __FreeBSD__
        if(term_colors == 16)
            setenv("TERM", "xterm-16color", 1);

        if(term_colors > 16)
            // setenv("TERM", "xterm-direct", 1);
            setenv("TERM", "xterm-256color", 1);
#endif

        setenv("COLORTERM", "xterm", 1);
    }

    if(vterm->flags & VTERM_FLAG_LINUX)
    {
        setenv("TERM", "linux", 1);
        unsetenv("COLORTERM");
    }

    if(vterm->flags & VTERM_FLAG_VT100)
    {
        setenv("TERM", "vt100", 1);
        unsetenv("COLORTERM");
    }

    return;
}

