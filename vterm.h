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

/* The NOCURSES flag impacts compile time.
   Contrast with VTERM_FLAG_NOCURSES which does not impact the link
   dependencies but suppresses *use* of curses at runtime.
*/

#ifndef _VTERM_H_
#define _VTERM_H_

#include <inttypes.h>

#include <sys/types.h>

#ifdef NOCURSES
  // short equivalent of the header, without needing the lib.
  typedef unsigned long chtype;
  typedef unsigned char bool;

  #undef TRUE
  #define TRUE    1
  #undef FALSE
  #define FALSE   0

  #define COLOR_BLACK    0
  #define COLOR_RED      1
  #define COLOR_GREEN    2
  #define COLOR_YELLOW   3
  #define COLOR_BLUE     4
  #define COLOR_MAGENTA  5
  #define COLOR_CYAN     6
  #define COLOR_WHITE    7

  #define COLOR_PAIR(n)  ((n) & 0xff)<<8

  #define A_NORMAL      0x00000000
  #define A_REVERSE     0x00040000
  #define A_BLINK       0x00080000
  #define A_BOLD        0x00200000
  #define A_INVIS       0x00800000

  #define NCURSES_ACS(x) (((x)>0&&(x)<0x80)?((x)|0x00400000):0)

  #define KEY_UP        259
  #define KEY_DOWN      258
  #define KEY_RIGHT     261
  #define KEY_LEFT      260
  #define KEY_BACKSPACE 263
  #define KEY_IC        331
  #define KEY_DC        330
  #define KEY_HOME      262
  #define KEY_END       360
  #define KEY_PPAGE     339
  #define KEY_NPAGE     338
  #define KEY_SUSPEND   407

  #define KEY_F(n)      ((n)>0 && (n)<50)?(n)+264:0

#else
  #include <curses.h>
#endif

#define LIBVTERM_VERSION       "3.5"

#define VTERM_FLAG_RXVT        0                       // default
#define VTERM_FLAG_VT100       (1 << 1)
#define VTERM_FLAG_NOPTY       (1 << 2)  // skip all the fd and pty stuff - just render input args bytestream to a buffer
#define VTERM_FLAG_NOCURSES    (1 << 3)  // skip the curses WINDOW stuff - return the char cell array if required

#define VTERM_FLAG_DUMP        (1 << 8)  // tell libvterm to write stream data to a dump file for debugging

// Need this to be public if we want to expose the buffer to callers.
struct _vterm_cell_s
{
   chtype         ch;                           // cell data
   int          attr;                         // cell attributes
};

typedef struct _vterm_cell_s vterm_cell_t;

typedef struct _vterm_s         vterm_t;

/*
    alloc a raw terminal object
*/
vterm_t*        vterm_alloc(void);

/*
    init a terminal object with a known height and width and alloc object
    if need be.
*/
vterm_t*        vterm_init(vterm_t *vterm, uint16_t width, uint16_t height,
                    unsigned int flags);

/*
    convenience macro for alloc-ing a ready to use terminal object.
*/
#define         vterm_create(width, height, flags) \
                    vterm_init(NULL, width, height, flags)

/*
    destroy a terminal object.
*/
void            vterm_destroy(vterm_t *vterm);

/*
    fetch the process id (pid) of the terminal.
*/
pid_t           vterm_get_pid(vterm_t *vterm);


/*
    get the file descriptor of the pty.
*/
int             vterm_get_pty_fd(vterm_t *vterm);

/*
    get the name of the tty.
*/
const char*     vterm_get_ttyname(vterm_t *vterm);

/*
    get the name of the window title (if set by OSC command)
*/

void            vterm_get_title(vterm_t *vterm, char *buf, int buf_sz);

/*
    set a binary and args to launch instead of a shell.
*/
void            vterm_set_exec(vterm_t *vterm, char *path, char **argv);

/*
    read bytes from the terminal.
*/
ssize_t         vterm_read_pipe(vterm_t *vterm);

/*
    write a keystroke to the terminal.
*/
int             vterm_write_pipe(vterm_t *vterm, uint32_t keycode);

#ifndef NOCURSES
/*
    set the WINDOW * to that the terminal will use for output.
*/
void            vterm_wnd_set(vterm_t *vterm, WINDOW *window);

/*
    get the WINDOW * that the terminal is using.
*/
WINDOW*         vterm_wnd_get(vterm_t *vterm);

/*
    cause updates to the terminal to be blitted
*/
void            vterm_wnd_update(vterm_t *vterm);
#endif

/*
    set the foreground and bakground colors that will be used by
    default on erase operations.
*/
int             vterm_set_colors(vterm_t *vterm, short fg, short bg);

/*
    get the color pair number of the current fg/bg combination.
*/
short           vterm_get_colors(vterm_t *vterm);

/*
    erase the contents of the terminal.
    @params:
        vterm   - handle to a vterm object
        idx     - index of the buffer or -1 for current 
*/
void            vterm_erase(vterm_t *vterm, int idx);

/*
    erase the specified row of the terminal.
*/
void            vterm_erase_row(vterm_t *vterm,int row);

/*
    erase the terminal beginning at a certain row and toward the bottom
    margin.
*/
void            vterm_erase_rows(vterm_t *vterm,int start_row);

/*
    erase the specified column of the terminal.
*/
void            vterm_erase_col(vterm_t *vterm,int col);

/*
    erase the terminal at a specific column and toward the right margin.
*/
void            vterm_erase_cols(vterm_t *vterm,int start_cols);

/*
    cause the terminal to be scrolled up by one row and placing an empty
    row at the bottom.
*/
void            vterm_scroll_up(vterm_t *vterm);

/*
    cause the termianl to be scrolled down by one row and placing an
    empty row at the top.
*/
void            vterm_scroll_down(vterm_t *vterm);

/*
    this is a convenience macro to keep original behavior intact for
    applications that just don't care.  it assumes resize occurs from
    the bottom right origin.
*/
#define         vterm_resize(vterm, width, height)  \
                    vterm_resize_full(vterm, width, height, 0, 0, 1, 1)

/*
    resize the terminal to a specific size.  when shrinking the terminal,
    excess will be clipped and discarded based on where the grip edges
    are originating from.

    this API is typically used in reponse to a SIGWINCH signal.
*/
void            vterm_resize_full(vterm_t *vterm,
                    uint16_t width, uint16_t height,
                    int grip_top, int grip_left,
                    int grip_right, int grip_bottom);

/*
    needed to allow an app to see what's on screen right now.
*/
void            vterm_render(vterm_t *, const char *data, int len);

/*
    populate width and height with the current terminal dimentions.
*/
void            vterm_get_size(vterm_t *vterm, int *width, int *height);

/*
    raw access to the screen matrix which is current ncurses 'chtype'.
*/
vterm_cell_t**  vterm_get_buffer(vterm_t *vterm);

/* Needed if we don't use curses */
int             GetFGBGFromColorIndex( int index, int* fg, int* bg );
int             FindColorPair( int fg, int bg );

#endif

