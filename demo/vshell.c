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

#include <vterm.h>

int screen_w, screen_h;
WINDOW *term_win;

int main(int argc, char **argv)
{
    vterm_t     *vterm;
    int 		i, j, ch;
	char		*locale;
    ssize_t     bytes;
    int         flags = 0;

	locale = setlocale(LC_ALL,"");

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

    if (argc > 2)
    {
        // interate through argv[] looking for params
        for (i = 1; i < argc; i++)
        {

            if (strncmp(argv[i], "--dump", 6)
            {
                flags |= VTERM_FLAGS_DUMP;
                continue;
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
    term_win = newwin(25,80,1,4);
    wattrset(term_win, COLOR_PAIR(7*8+7-0));        // black over white
    mvwprintw(term_win, 0, 27, " Term In a Box ");
    wrefresh(term_win);

    // create the terminal and have it run bash
    vterm = vterm_create(80,25,VTERM_FLAG_RXVT);
    vterm_set_colors(vterm,COLOR_WHITE,COLOR_BLACK);
    vterm_wnd_set(vterm,term_win);

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
            touchwin(term_win);
            wrefresh(term_win);
            refresh();
        }

        if(bytes == -1) break;

        ch = getch();
        if (ch != ERR) vterm_write_pipe(vterm,ch);
    }

    endwin();
    return 0;
}


