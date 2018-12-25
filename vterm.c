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

#include <stdlib.h>
#include <pty.h>
#include <stdio.h>
#include <string.h>
#include <term.h>
#include <utmp.h>
#include <termios.h>
#include <signal.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "vterm.h"
#include "vterm_private.h"
#include "vterm_write.h"
#include "vterm_exec.h"
#include "vterm_buffer.h"

/*
    this string is emitted by for resetting an RXVT terminal (RS1).
    xterm is far more sane using a single \Ec control code to perform
    the same termcap operation.  the sequence is codified in terminfo
    database.  this can be inspected with the infocmp tool.
*/
#define RXVT_RS1    "\e>\e[1;3;4;5;6l\e[?7h\em\er\e[2J\e[H"

vterm_t*
vterm_alloc(void)
{
    vterm_t *vterm;

    vterm = (vterm_t*)calloc(1,sizeof(vterm_t));

    return  vterm;
}

vterm_t*
vterm_init(vterm_t *vterm, uint16_t width, uint16_t height, unsigned int flags)
{
    pid_t           child_pid = 0;
    int             master_fd;
    struct winsize  ws = {.ws_xpixel = 0,.ws_ypixel = 0};
    char            *pos = NULL;
    int             retval;

#ifdef NOCURSES
    flags = flags | VTERM_FLAG_NOCURSES;
#endif

    if(height <= 0 || width <= 0) return NULL;

    if(vterm == NULL)
        vterm = (vterm_t*)calloc(1, sizeof(vterm_t));

    // allocate a the buffer (a matrix of cells)
    vterm_buffer_alloc(vterm, VTERM_BUFFER_STD, width, height);

    // default active colors
    // uses ncurses macros even if we aren't using ncurses.
    // it is just a bit mask/shift operation.
    if(flags & VTERM_FLAG_NOCURSES)
    {
        int colorIndex = FindColorPair( COLOR_WHITE, COLOR_BLACK );
        if( colorIndex < 0 || colorIndex > 255 )
            colorIndex = 0;
        vterm->vterm_desc[0].curattr = (colorIndex & 0xff) << 8;
    }
    else
    {
#ifdef NOCURSES
        vterm->vterm_desc[0].curattr = 0;
#else
        vterm->vterm_desc[0].curattr = COLOR_PAIR( 0 );
#endif
    }

    // initialize all cells with defaults
    vterm_erase(vterm, VTERM_BUFFER_STD);

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

    if(flags & VTERM_FLAG_VT100)
    {
        vterm->reset_rs1 = NULL;
    }
    else
    {
        vterm->reset_rs1 = RXVT_RS1;
    }

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
            return NULL;
        }

        if(child_pid == 0)
        {
            signal(SIGINT, SIG_DFL);

            // default is rxvt emulation
            setenv("TERM", "rxvt", 1);
            setenv("COLORTERM", "rxvt", 1);

            if(flags & VTERM_FLAG_VT100)
            {
                setenv("TERM", "vt100", 1);
                unsetenv("COLORTERM");
            }

            retval = vterm_exec_binary(vterm);

            if(retval == -1) exit(EXIT_FAILURE);

            exit(EXIT_SUCCESS);
        }

        vterm->child_pid = child_pid;

        if(flags & VTERM_FLAG_DUMP)
        {
            sprintf(pos, "vterm-%d.dump", child_pid);
            vterm->debug_fd = open(vterm->debug_filepath,
                O_CREAT | O_TRUNC | O_WRONLY, S_IWUSR | S_IRUSR);
        }

        if(ttyname_r(master_fd,vterm->ttyname,sizeof(vterm->ttyname) - 1) != 0)
        {
            snprintf(vterm->ttyname,sizeof(vterm->ttyname) - 1,"vterm");
        }
    }

    if(flags & VTERM_FLAG_VT100) vterm->write = vterm_write_vt100;
    else vterm->write = vterm_write_rxvt;

    return vterm;
}

void
vterm_destroy(vterm_t *vterm)
{
    int   i;

    if(vterm == NULL) return;

    // todo:  do something more elegant in the future
    for(i = 0; i < 2; i++)
    {
        vterm_buffer_dealloc(vterm, i);
    }

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

