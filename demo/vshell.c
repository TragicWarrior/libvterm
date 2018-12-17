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

typedef struct
{
    WINDOW  *term_win;
}
testwin_t;

struct _color_mtx_s
{
    int     fg;
    int     bg;
};

typedef struct _color_mtx_s     color_mtx_t;

// prototypes
void    vshell_paint_screen(vterm_t *vterm);
int     vshell_resize(testwin_t *twin, vterm_t * vterm);
void    vshell_hook(vterm_t *vterm, int event, void *anything);
void    vshell_color_init(void);
short   vshell_color_pair(short fg, short bg);

// macros
#define VWINDOW(x)  (*(WINDOW**)x)

// globals
WINDOW          *screen_wnd;
int             screen_w, screen_h;
int             frame_colors;
color_mtx_t     *color_mtx;


int main(int argc, char **argv)
{
    vterm_t     *vterm;
    int 		i, ch;
    ssize_t     bytes;
    int         flags = 0;
    testwin_t   *twin;
    char        *exec_path = NULL;
    char        **exec_argv = NULL;
    int         count = 1;

	setlocale(LC_ALL,"UTF-8");

    screen_wnd = initscr();
    noecho();
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

    vshell_color_init();

    // set default frame color
    frame_colors = vshell_color_pair(COLOR_WHITE, COLOR_BLUE);
    vshell_paint_screen(NULL);

    // VWINDOW(twin) = newwin(25,80,1,4);
    VWINDOW(twin) = newwin(screen_h - 2, screen_w - 2, 1, 1);

    wattrset(VWINDOW(twin), COLOR_PAIR(7*8+7-0));        // black over white
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
        vterm = vterm_create(screen_w - 2, screen_h - 2, flags);
    }

    vterm_set_colors(vterm, COLOR_WHITE, COLOR_BLACK);
    vterm_wnd_set(vterm, VWINDOW(twin));
    vterm_install_hook(vterm, vshell_hook);

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

        // handle special events like resize first
        if(ch == KEY_RESIZE)
        {
            vshell_resize(twin, vterm);
            continue;
        }

        if (ch != ERR) vterm_write_pipe(vterm,ch);
    }

    endwin();

    // printf("Pipe buffer size was %d\n\r", PIPE_BUF);

    return 0;
}

void
vshell_paint_screen(vterm_t *vterm)
{
    extern WINDOW   *screen_wnd;
    char            title[256] = " Term In A Box ";
    char            buf[254];
    int             len;
    int             offset;

    // paint the screen blue
    attrset(COLOR_PAIR(frame_colors));    // white on blue
    box(screen_wnd, 0, 0);

    // quick computer of title location
    vterm_get_title(vterm, buf, sizeof(buf));
    if(buf[0] != '\0')
    {
        sprintf(title, " %s ", buf);
    }

    len = strlen(title);
    offset = (screen_w >> 1) - (len >> 1);
    mvprintw(0, offset, title);

    sprintf(title, " %d x %d ", screen_w - 2, screen_h - 2);
    len = strlen(title);
    offset = (screen_w >> 1) - (len >> 1);
    mvprintw(screen_h - 1, offset, title);

    refresh();

    return;
}

int
vshell_resize(testwin_t *twin, vterm_t * vterm)
{
    getmaxyx(stdscr, screen_h, screen_w);

    vshell_paint_screen(vterm);

    wresize(VWINDOW(twin), screen_h - 2, screen_w - 2);

    vterm_resize(vterm, screen_w - 2, screen_h - 2);

    touchwin(VWINDOW(twin));
    wrefresh(VWINDOW(twin));
    refresh();

    return 0;
}

void
vshell_hook(vterm_t *vterm, int event, void *anything)
{
    extern int  frame_colors;
    int         idx;

    if(vterm == NULL) return;       // something went horribly wrong

    switch(event)
    {
        case VTERM_HOOK_BUFFER_ACTIVATED:
        {
            idx = *(int *)anything;

            if(idx == 0)
                frame_colors = vshell_color_pair(COLOR_WHITE, COLOR_BLUE);
            else
                frame_colors = vshell_color_pair(COLOR_BLACK, COLOR_RED);

            vshell_paint_screen(vterm);
            break;
        }
    }

    return;
}

void
vshell_color_init(void)
{
    extern color_mtx_t  *color_mtx;
    short               color_table[] =
                            {   COLOR_BLACK, COLOR_RED, COLOR_GREEN,
                                COLOR_YELLOW, COLOR_BLUE, COLOR_MAGENTA,
                                COLOR_CYAN, COLOR_WHITE };
    int                 color_count;
    short               fg,bg;
    int                 i;
    int                 max_colors;
    int                 hard_pair = -1;

    start_color();
    color_count = sizeof(color_table) / sizeof(color_table[0]);

    /*
        calculate the size of the matrix.  this is necessary because
        some terminals support 256 colors and we don't want to deal
        with that.
    */
    max_colors = color_count * color_count;

    color_mtx  = (color_mtx_t *)calloc(1, max_colors * sizeof(color_mtx_t));

    for(i = 0;i < max_colors; i++)
    {
        fg = i / color_count;
        bg = color_count - (i % color_count) - 1;

        color_mtx[i].bg = fg;
        color_mtx[i].fg = bg;

        /*
            according to ncurses documentation, color pair 0 is assumed to
            be WHITE foreground on BLACK background.  when we discover this
            pair, we need to make sure it gets swapped into index 0 and
            whatever is in index 0 gets put into this location.
        */
        if(color_mtx[i].fg == COLOR_WHITE && color_mtx[i].bg == COLOR_BLACK)
            hard_pair = i;
    }

    /*
        if hard_pair is no longer -1 then we found the "hard pair"
        during our enumeration process and we need to do the swap.
    */
    if(hard_pair != -1)
    {
        fg = color_mtx[0].fg;
        bg = color_mtx[0].bg;
        color_mtx[hard_pair].fg = fg;
        color_mtx[hard_pair].bg = bg;
    }

    for(i = 1; i < max_colors; i++)
    {
        init_pair(i, color_mtx[i].fg, color_mtx[i].bg);
    }

    return;
}

inline short
vshell_color_pair(short fg, short bg)
{
    extern color_mtx_t  *color_mtx;
    int                 i = 0;

    if(fg == COLOR_WHITE && bg == COLOR_BLACK) return 0;

    while(TRUE)
    {
        if(color_mtx[i].fg == fg && color_mtx[i].bg == bg) break;
        i++;
    }

    return i;
}

