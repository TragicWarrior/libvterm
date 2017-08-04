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
#include <pwd.h>
#include <utmp.h>
#include <termios.h>
#include <signal.h>
#include <limits.h>

#include <sys/types.h>

#include "vterm.h"
#include "vterm_private.h"
#include "vterm_write.h"

vterm_t*
vterm_create(uint16_t width,uint16_t height,unsigned int flags)
{
    vterm_t         *vterm;
    struct passwd   *user_profile;
    char            *user_shell = NULL;
    pid_t           child_pid;
    int             master_fd;
    struct winsize  ws = {.ws_xpixel = 0,.ws_ypixel = 0};
    int             i;

#ifdef NOCURSES
    flags = flags | VTERM_FLAG_NOCURSES;
#endif

    if(height <= 0 || width <= 0) return NULL;

    vterm = (vterm_t*)calloc(1,sizeof(vterm_t));

    // record dimensions
    vterm->rows = height;
    vterm->cols = width;

    // create the cell matrix
    vterm->cells = (vterm_cell_t**)calloc(1,(sizeof(vterm_cell_t*) * height));

    for(i = 0;i < height;i++)
    {
        vterm->cells[i] = (vterm_cell_t*)calloc(1,
            (sizeof(vterm_cell_t) * width));
    }

    // initialize all cells with defaults
    vterm_erase(vterm);

    // initialization of other public fields
    vterm->crow = 0;
    vterm->ccol = 0;

    // default active colors
    // uses ncurses macros even if we aren't using ncurses.
    // it is just a bit mask/shift operation.
    if(flags & VTERM_FLAG_NOCURSES)
    {
        int colorIndex = FindColorPair( COLOR_WHITE, COLOR_BLACK );
        if( colorIndex<0 || colorIndex>255 )
            colorIndex = 0;
        vterm->curattr = (colorIndex & 0xff) << 8;
    }
    else
    {
#ifdef NOCURSES
        vterm->curattr = 0;
#else
        vterm->curattr = COLOR_PAIR( 0 );
#endif
    }

    if(flags & VTERM_FLAG_DUMP)
    {
        vterm->debug_filepath = (char*)calloc(PATH_MAX + 1,sizeof(char));
        getcwd(vterm->debug_filepath, PATH_MAX);

        // TODO:  write full path string
    }

    // initial scrolling area is the whole window
    vterm->scroll_min = 0;
    vterm->scroll_max = height - 1;

    vterm->flags = flags;

    memset(&ws,0,sizeof(ws));
    ws.ws_row = height;
    ws.ws_col = width;

    if(flags & VTERM_FLAG_NOPTY)
    { // skip all the child process and fd stuff.
    }
    else
    {
        child_pid = forkpty(&master_fd,NULL,NULL,&ws);
        vterm->pty_fd = master_fd;
    
        if(child_pid < 0)
        {
            vterm_destroy(vterm);
            return NULL;
        }
    
        if(child_pid == 0)
        {
            signal(SIGINT,SIG_DFL);
    
            // default is rxvt emulation
            setenv("TERM","rxvt",1);
    
            if(flags & VTERM_FLAG_VT100)
            {
                setenv("TERM","vt100",1);
            }
    
            user_profile = getpwuid(getuid());
            if(user_profile == NULL) user_shell = "/bin/sh";
            else if(user_profile->pw_shell == NULL) user_shell = "/bin/sh";
            else user_shell = user_profile->pw_shell;
    
            if(user_shell == NULL) user_shell="/bin/sh";
    
            // start the shell
            if(execl(user_shell,user_shell,"-l",NULL) == -1)
            {
                exit(EXIT_FAILURE);
            }
    
            exit(EXIT_SUCCESS);
        }
    
        vterm->child_pid = child_pid;
    
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

    for(i = 0;i < vterm->rows;i++) free(vterm->cells[i]);
    free(vterm->cells);

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
