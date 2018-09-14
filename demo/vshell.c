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

#include <ncurses.h>
#include <stdio.h>
#include <signal.h>
#include <locale.h>
#include <string.h>
#include <stdlib.h>

#include "../vterm.h"
#include "../strings.h"

int screen_w, screen_h;

typedef struct
{
    WINDOW  *term_win;
}
testwin_t;

#define VWINDOW(x)  (*(WINDOW**)x)

int main(int argc, char **argv)
{
    vterm_t     *vterm;
    int 		i, j, ch;
    ssize_t     bytes;
    int         flags = 0;
    testwin_t   *twin;
    char        *exec_path = NULL;
    char        **exec_argv = NULL;
    int         count = 1;

	setlocale(LC_ALL,"UTF-8");

    initscr();
    noecho();
    start_color();
    raw();
    nodelay(stdscr, TRUE);              /*
                                           prevents getch() from blocking;
                                           rather it will return ERR when
                                           there is no keypress available
                                        */

    keypad(stdscr, TRUE);
    getmaxyx(stdscr, screen_h, screen_w);

    twin = (testwin_t*)calloc(1, sizeof(testwin_t));

    if (argc > 1)
    {
        // interate through argv[] looking for params
        for (i = 1; i < argc; i++)
        {

            if (strncmp(argv[i], "--dump", strlen("--dump")) == 0)
            {
                flags |= VTERM_FLAG_DUMP;
                continue;
            }

            if (strncmp(argv[i], "--vt100", strlen("--vt100")) == 0)
            {
                flags |= VTERM_FLAG_VT100;
                continue;
            }

            if (strncmp(argv[i], "--exec", strlen("--exec")) == 0)
            {
                // must have at least exec path
                i++;
                if (i < argc)
                {
                    exec_path = strdup(argv[i]);

                    // first arg shouldbe same as path
                    exec_argv = (char **)calloc(argc, sizeof(char *));
                    exec_argv[0] = strdup(argv[i]);
                    i++;
                }

                count = 1;
                while(i < argc)
                {
                    exec_argv[count] = strdup(argv[i]);
                    count++;
                    i++;
                }

                // this will always be the last set of params we handle
                break;
            }
        }
    }


    /*
        initialize the color pairs the way rote_vt_draw expects it. You might
        initialize them differently, but in that case you would need
        to supply a custom conversion function for rote_vt_draw to
        call when setting attributes. The idea of this "default" mapping
        is to map (fg,bg) to the color pair bg * 8 + 7 - fg. This way,
        the pair (white,black) ends up mapped to 0, which means that
        it does not need a color pair (since it is the default). Since
        there are only 63 available color pairs (and 64 possible fg/bg
        combinations), we really have to save 1 pair by assigning no pair
        to the combination white/black.
    */
    for (i = 0; i < 8; i++)
    {
        for (j = 0; j < 8; j++)
        {
            if (i != 7 || j != 0) init_pair(j*8+7-i, i, j);
        }
    }

    // paint the screen blue
    attrset(COLOR_PAIR(32));
    for (i = 0; i < screen_h; i++)
    {
        for (j = 0; j < screen_w; j++) addch(' ');
    }
    refresh();

    // create a window with a frame

    // VWINDOW(twin) = newwin(25,80,1,4);
    VWINDOW(twin) = newwin(screen_h - 2, screen_w - 2, 1, 1);

    wattrset(VWINDOW(twin), COLOR_PAIR(7*8+7-0));        // black over white
    mvwprintw(VWINDOW(twin), 0, 27, " Term In a Box ");
    wrefresh(VWINDOW(twin));

    // create the terminal and have it run bash

    if(exec_path != NULL)
    {
        vterm = vterm_alloc();
        vterm_set_exec(vterm, exec_path, exec_argv);
        vterm_init(vterm, 80, 25, flags);
    }
    else
    {
        vterm = vterm_create(screen_w - 4, screen_h - 2, flags);
    }

    vterm_set_colors(vterm,COLOR_WHITE,COLOR_BLACK);
    vterm_wnd_set(vterm, VWINDOW(twin));

    /*
        keep reading keypresses from the user and passing them to
        the terminal;  also, redraw the terminal to the window at each
        iteration
    */
    ch = '\0';
    while (TRUE)
    {
        bytes = vterm_read_pipe(vterm);
        if(bytes > 0)
        {
            vterm_wnd_update(vterm);
            touchwin(VWINDOW(twin));
            wrefresh(VWINDOW(twin));
            refresh();
        }

        if(bytes == -1) break;

        ch = getch();
        if (ch != ERR) vterm_write_pipe(vterm,ch);
    }

    endwin();
    return 0;
}


