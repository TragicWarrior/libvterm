#define _XOPEN_SOURCE 500               // needed for wcswidth()
#define _XOPEN_SOURCE_EXTNEDED

#include <stdio.h>
#include <signal.h>
#include <locale.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <wchar.h>

#ifdef __FreeBSD__
#include <ncurses/ncurses.h>
#else
#include <ncurses.h>
#endif

#include "ctimer.h"
#include "../vterm.h"
#include "../stringv.h"


#define WCS_UARROW      0x2191
#define WCS_DARROW      0x2193

#define TERM_MODE_NORMAL    0
#define TERM_MODE_ALT       1
#define TERM_MODE_HISTORY   (1 << 4)

short   color_table[] =
            {
                COLOR_BLACK, COLOR_RED, COLOR_GREEN,
                COLOR_YELLOW, COLOR_BLUE, COLOR_MAGENTA,
                COLOR_CYAN, COLOR_WHITE
            };

int      color_count = sizeof(color_table) / sizeof(color_table[0]);


struct _color_mtx_s
{
    int     fg;
    int     bg;
};

typedef struct _color_mtx_s     color_mtx_t;

typedef struct _vshell_s        vshell_t;

// global struct
struct _vshell_s
{
    int             argc;
    char            **argv;

    char            *exec_path;
    char            **exec_argv;

    vterm_t         *vterm;
    uint32_t        flags;
    WINDOW          *screen_wnd;
    WINDOW          *term_wnd;
    int             screen_w;
    int             screen_h;
    int             term_mode;
    color_mtx_t     *color_mtx;
    int             cursor_pos;

    // object "methods"
    void            (*render)       (vshell_t *, void *);
    void            (*kinput)       (vshell_t *, int32_t);
};

// prototypes
void    vshell_parse_cmdline(vshell_t *vshell);
void    vshell_print_help(void);
void    vshell_color_init(vshell_t *vshell);

void    vshell_paint_frame(vshell_t *vshell);
int32_t vshell_get_key(void);
int     vshell_resize(vshell_t *vshell);

void    vshell_render_normal(vshell_t *vshell, void *anything);
void    vshell_render_history(vshell_t *vshell, void *anything);
void    vshell_kinput_normal(vshell_t *vshell, int32_t keystroke);
void    vshell_kinput_history(vshell_t *vshell, int32_t keystroke);

void    vshell_hook(vterm_t *vterm, int event, void *anything);
short   vshell_pair_selector(vterm_t *vterm, short fg, short bg);


vshell_t    *vshell;

int main(int argc, char **argv)
{
    int32_t         keystroke;
    // int		        i;
    ssize_t         bytes;
    ssize_t         bytes_buffered = 0;
    char            *locale;

    vshell = (vshell_t *)calloc(1, sizeof(vshell_t));

    // 1,000,000 usec = 1 sec
    // effective 30000 usec = 30 fps
    struct timeval  refresh_interval = { .tv_sec = 0, .tv_usec = 30000 };
    ctimer_t        *refresh_timer;

    locale = getenv("LANG");
    if(locale == NULL) locale = "en_US.UTF-8";

	setlocale(LC_ALL, locale);

    vshell->screen_wnd = initscr();
    noecho();
    raw();
    curs_set(0);                        // hide cursor
    nodelay(stdscr, TRUE);              /*
                                           prevents getch() from blocking;
                                           rather it will return ERR when
                                           there is no keypress available
                                        */
    keypad(stdscr, TRUE);
    getmaxyx(stdscr, vshell->screen_h, vshell->screen_w);

    vshell->argc = argc;
    vshell->argv = argv;

    vshell_parse_cmdline(vshell);

    vshell_color_init(vshell);
    vshell->render = vshell_render_normal;
    vshell->kinput = vshell_kinput_normal;

    // set default frame color
    vshell_paint_frame(vshell);

    vshell->term_wnd = newwin(vshell->screen_h - 2, vshell->screen_w - 2, 1, 1);

    wattrset(vshell->term_wnd, COLOR_PAIR(7 * 8 + 7 - 0)); // black over white
    wrefresh(vshell->term_wnd);

    vshell->vterm = vterm_alloc();

    // set the exec path if specified
    if(vshell->exec_path != NULL)
    {
        vterm_set_exec(vshell->vterm, vshell->exec_path, vshell->exec_argv);
    }

    vshell->vterm = vterm_init(vshell->vterm,
        vshell->screen_w - 2, vshell->screen_h - 2, vshell->flags);

    vterm_set_colors(vshell->vterm, COLOR_WHITE, COLOR_BLACK);
    vterm_wnd_set(vshell->vterm, vshell->term_wnd);

    // this illustrates how to install an event hook
    vterm_set_event_mask(vshell->vterm, VTERM_MASK_BUFFER_ACTIVATED);
    vterm_install_hook(vshell->vterm, vshell_hook);

    refresh_timer = ctimer_create();

    /*
        keep reading keypresses from the user and passing them to
        the terminal;  also, redraw the terminal to the window at each
        iteration
    */
    keystroke = '\0';
    while (TRUE)
    {
        bytes = vterm_read_pipe(vshell->vterm);
        bytes_buffered += bytes;

        if(ctimer_compare(refresh_timer, &refresh_interval) > 0)
        {
            if(bytes_buffered > 0)
            {
                vshell->render(vshell, &(int){0});

                bytes_buffered = 0;
            }

            ctimer_reset(refresh_timer);
        }

        if(bytes == -1) break;

        keystroke = vshell_get_key();

        // handle special events like resize first
        if(keystroke == KEY_RESIZE)
        {
            vshell_resize(vshell);
            continue;
        }

        // pass the keystroke into the active keyboard handler
        vshell->kinput(vshell, keystroke);
    }

    endwin();

    vterm_destroy(vshell->vterm);

    // print some debug info
    printf("%s\n\r", locale);
    printf("ncurses = %d.%d (%d)\n\r", NCURSES_VERSION_MAJOR,
        NCURSES_VERSION_MINOR, NCURSES_VERSION_PATCH);
    fflush(NULL);

    return 0;
}

int32_t
vshell_get_key(void)
{
    int32_t keystroke;
    int     ch;

    ch = getch();
    keystroke = ch;

    if(ch == 0x1b)
    {
        for(;;)
        {
            ch = getch();
            if(ch == -1) break;

            keystroke = keystroke << 8;
            keystroke |= ch;
        }
    }

    return keystroke;
}

void
vshell_paint_frame(vshell_t *vshell)
{
    char            title[256] = " Term In A Box ";
    char            buf[254];
    wchar_t         wbuf[512];
    static cchar_t  cbuf[512];
    int             len;
    int             offset;
    int             frame_colors;
    int             i = 0;

    if(vshell->term_mode == TERM_MODE_NORMAL)
    {
        frame_colors = vshell_pair_selector(NULL, COLOR_WHITE, COLOR_BLUE);
    }

    if(vshell->term_mode == TERM_MODE_ALT)
    {
        frame_colors = vshell_pair_selector(NULL, COLOR_WHITE, COLOR_RED);
    }

    if(vshell->term_mode & TERM_MODE_HISTORY)
    {
        frame_colors = vshell_pair_selector(NULL, COLOR_WHITE, COLOR_MAGENTA);
    }

    // paint the screen blue
    attrset(COLOR_PAIR(frame_colors));    // white on blue
    wattron(vshell->screen_wnd, A_BOLD);
    box(vshell->screen_wnd, 0, 0);

    // quick computer of title location
    if(vshell->vterm != NULL)
    {
        vterm_get_title(vshell->vterm, buf, sizeof(buf));
        if(buf[0] != '\0')
        {
            sprintf(title, " %s ", buf);
        }
    }

    len = strlen(title);

    // a right shift is the same as divide by 2 but quicker
    offset = (vshell->screen_w >> 1) - (len >> 1);
    mvprintw(0, offset, title);

    if(vshell->term_mode & TERM_MODE_HISTORY)
    {
        len = swprintf(wbuf, 512,
            L" Press [alt + z] to exit history | "
            L" [%lc] / [%lc] Scroll | Line %03d ",
            WCS_UARROW, WCS_DARROW, vshell->cursor_pos);
        memset(cbuf, 0, sizeof(cbuf));

        for(i = 0; i < len; i++)
        {
            setcchar(&cbuf[i], &wbuf[i], WA_BOLD, frame_colors, NULL);
        }

        offset = (vshell->screen_w >> 1) - (len >> 1);

        mvwadd_wchnstr(vshell->screen_wnd,
            vshell->screen_h - 1, offset, cbuf, -1);
    }
    else
    {
        len = sprintf(title, " Press [alt + z] for history | %s | %d x %d ",
            (vshell->term_mode == TERM_MODE_NORMAL) ? "std" : "alt",
            vshell->screen_w - 2, vshell->screen_h - 2);

        offset = (vshell->screen_w >> 1) - (len >> 1);

        mvwprintw(vshell->screen_wnd, vshell->screen_h - 1, offset, title);
    }

    refresh();

    return;
}

int
vshell_resize(vshell_t *vshell)
{
    getmaxyx(stdscr, vshell->screen_h, vshell->screen_w);

    vshell->render(vshell, &(int){0});

    wresize(vshell->term_wnd, vshell->screen_h - 2, vshell->screen_w - 2);

    vterm_resize(vshell->vterm, vshell->screen_w - 2, vshell->screen_h - 2);

    touchwin(vshell->term_wnd);
    wrefresh(vshell->term_wnd);
    refresh();

    return 0;
}

/*
    this illustarates a callback "event hook".  a simple switch
    statement is all that is needed to "listen" for the event
    you want to trigger on.
*/
void
vshell_hook(vterm_t *vterm, int event, void *anything)
{
    int         idx;

    if(vterm == NULL) return;       // something went horribly wrong

    switch(event)
    {
        case VTERM_EVENT_BUFFER_ACTIVATED:
        {
            idx = *(int *)anything;

            if(idx == 0)
                vshell->term_mode = TERM_MODE_NORMAL;
            else
                vshell->term_mode = TERM_MODE_ALT;

            vshell->render(vshell, &(int){0});
            break;
        }
    }

    return;
}

void
vshell_color_init(vshell_t *vshell)
{
    extern short        color_table[];
    extern int          color_count;
    short               fg, bg;
    int                 i;
    int                 max_colors;
    int                 hard_pair = -1;

    start_color();

    // if(COLOR_PAIRS > 0x7FFF) COLOR_PAIRS = 0x7FFF;

    /*
        calculate the size of the matrix.  this is necessary because
        some terminals support 256 colors and we don't want to deal
        with that.
    */
    max_colors = color_count * color_count;

    vshell->color_mtx  = (color_mtx_t *)calloc(1,
        max_colors * sizeof(color_mtx_t));

    for(i = 0;i < max_colors; i++)
    {
        fg = i / color_count;
        bg = color_count - (i % color_count) - 1;

        vshell->color_mtx[i].bg = fg;
        vshell->color_mtx[i].fg = bg;

        /*
            according to ncurses documentation, color pair 0 is assumed to
            be WHITE foreground on BLACK background.  when we discover this
            pair, we need to make sure it gets swapped into index 0 and
            whatever is in index 0 gets put into this location.
        */
        if( vshell->color_mtx[i].fg == COLOR_WHITE &&
            vshell->color_mtx[i].bg == COLOR_BLACK )
        {
            hard_pair = i;
        }
    }

    /*
        if hard_pair is no longer -1 then we found the "hard pair"
        during our enumeration process and we need to do the swap.
    */
    if(hard_pair != -1)
    {
        fg = vshell->color_mtx[0].fg;
        bg = vshell->color_mtx[0].bg;
        vshell->color_mtx[hard_pair].fg = fg;
        vshell->color_mtx[hard_pair].bg = bg;
    }

    for(i = 1; i < max_colors; i++)
    {
        init_pair(i, vshell->color_mtx[i].fg, vshell->color_mtx[i].bg);
    }

    return;
}

short
vshell_pair_selector(vterm_t *vterm, short fg, short bg)
{
    extern short        color_table[];
    extern int          color_count;
    int                 i = 0;

    (void)vterm;        // make compiler happy

    if(fg == COLOR_WHITE && bg == COLOR_BLACK) return 0;

    i = (bg * color_count) + ((color_count - 1) - fg);

    return i;
}

void
vshell_kinput_normal(vshell_t *vshell, int32_t keystroke)
{
    // alt-z
    if(keystroke == 0x1b7a)
    {
        vshell->term_mode |= TERM_MODE_HISTORY;
        vshell->render = vshell_render_history;
        vshell->kinput = vshell_kinput_history;

        vshell->render(vshell, &(int){0});

        return;
    }

    if(keystroke != ERR)
    {
        vterm_write_pipe(vshell->vterm, keystroke);

        return;
    }
}


void
vshell_render_normal(vshell_t *vshell, void *anything)
{
    (void)anything;     // make compiler happy

    vshell_paint_frame(vshell);
    vterm_wnd_update(vshell->vterm, -1, 0);
    touchwin(vshell->term_wnd);
    wrefresh(vshell->term_wnd);
    refresh();

    return;
}

void
vshell_kinput_history(vshell_t *vshell, int32_t keystroke)
{
    // alt-z
    if(keystroke == 0x1b7a)
    {
        vshell->term_mode &= ~TERM_MODE_HISTORY;
        vshell->cursor_pos = 0;
        vshell->render = vshell_render_normal;
        vshell->kinput = vshell_kinput_normal;

        vshell->render(vshell, &(int){0});

        return;
    }

    if(keystroke == KEY_UP)
    {
        vshell->render(vshell, &(int){1});

        return;
    }

    if(keystroke == KEY_DOWN)
    {
        vshell->render(vshell, &(int){-1});

        return;
    }
}

void
vshell_render_history(vshell_t *vshell, void *anything)
{
    int         *scrolled;
    int         history_sz;
    int         height, width;
    int         offset;

    scrolled = (int *)anything;

    history_sz = vterm_get_history_size(vshell->vterm);
    vterm_wnd_size(vshell->vterm, &width, &height);

    if((vshell->cursor_pos + *scrolled) < 0)
    {
        vshell->cursor_pos = 0;
        *scrolled = 0;
    }

    if((vshell->cursor_pos + *scrolled) > history_sz - height)
    {
        vshell->cursor_pos = history_sz - height;
        *scrolled = 0;
    }

    vshell->cursor_pos += *scrolled;
    offset = history_sz - height - vshell->cursor_pos;

    vshell_paint_frame(vshell);
    vterm_wnd_update(vshell->vterm, VTERM_BUF_HISTORY, offset);
    touchwin(vshell->term_wnd);
    wrefresh(vshell->term_wnd);
    refresh();

    return;
}



void
vshell_parse_cmdline(vshell_t *vshell)
{
    int     count = 1;
    int     i;

    if(vshell == NULL) return;

    if (vshell->argc > 1)
    {
        // interate through argv[] looking for params
        for (i = 1; i < vshell->argc; i++)
        {
            if (strncmp(vshell->argv[i], "--help", strlen("--help")) == 0)
            {
                endwin();
                vshell_print_help();
                exit(0);
            }

            if (strncmp(vshell->argv[i], "--version", strlen("--version")) == 0)
            {
                endwin();
                vshell_print_help();
                exit(0);
            }

            if (strncmp(vshell->argv[i], "--dump", strlen("--dump")) == 0)
            {
                vshell->flags |= VTERM_FLAG_DUMP;
                continue;
            }

            if (strncmp(vshell->argv[i], "--rxvt", strlen("--rxvt")) == 0)
            {
                vshell->flags |= VTERM_FLAG_RXVT;
                continue;
            }

            if (strncmp(vshell->argv[i], "--vt100", strlen("--vt100")) == 0)
            {
                vshell->flags |= VTERM_FLAG_VT100;
                continue;
            }

            if (strncmp(vshell->argv[i], "--linux", strlen("--linux")) == 0)
            {
                vshell->flags |= VTERM_FLAG_LINUX;
                continue;
            }

            if (strncmp(vshell->argv[i], "--c16", strlen("--c16")) == 0)
            {
                vshell->flags |= VTERM_FLAG_C16;
                continue;
            }

            if (strncmp(vshell->argv[i], "--exec", strlen("--exec")) == 0)
            {
                // must have at least exec path
                i++;
                if (i < vshell->argc)
                {
                    vshell->exec_path = strdup(vshell->argv[i]);

                    // first arg should be same as path
                    vshell->exec_argv = (char **)calloc(vshell->argc,
                        sizeof(char *));

                    vshell->exec_argv[0] = strdup(vshell->argv[i]);
                    i++;
                }

                count = 1;
                while(i < vshell->argc)
                {
                    vshell->exec_argv[count] = strdup(vshell->argv[i]);

                    count++;
                    i++;
                }

                // this will always be the last set of params we handle
                break;
            }
        }
    }

    return;
}

void
vshell_print_help(void)
{
    char    *help_msg =
                "Usage:  vshell  <args...>\n\r"
                " \n\r"
                "[Args]         [Description]\n\r"
                "---------------------------------------------------------\n\r"
                "--help         Show usage information.\n\r"
                "--version      Show version information.\n\r"
                "--dump         Write escape sequences to a log file.\n\r"
                "--vt100        Set emulation mode to vt100.\n\r"
                "--rxvt         Set emulation mode to rxvt.\n\r"
                "--linux        Set emulation mode to linux.\n\r"
                "--xterm        Set emulation mode to xterm (default).\n\r"
                "               If the underlying terminal supports it,\n\r"
                "               vshell will attempt to use hi-color mode.\n\r"
                "--c16          When running in 16 color mode use the\n\r"
                "               libvterm palette instead.\n\r"
                "--exec <prg>   Launch program at start up.\n\r";

    printf("\n\rLibvterm version: %s\n\r\n\r%s\n\r",
        LIBVTERM_VERSION, help_msg);

    return;
}
