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

#define LIBVTERM_VERSION       "2.1"

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

vterm_t*        vterm_alloc(void);
vterm_t*        vterm_init(vterm_t *vterm, uint16_t width, uint16_t height,
                    unsigned int flags);
#define         vterm_create(width, height, flags) \
                    vterm_init(NULL, width, height, flags)
void            vterm_destroy(vterm_t *vterm);
pid_t           vterm_get_pid(vterm_t *vterm);
int             vterm_get_pty_fd(vterm_t *vterm);
const char*     vterm_get_ttyname(vterm_t *vterm);
void            vterm_set_exec(vterm_t *vterm, char *path, char **argv);

ssize_t         vterm_read_pipe(vterm_t *vterm);
void            vterm_write_pipe(vterm_t *vterm,uint32_t keycode);

#ifndef NOCURSES
void            vterm_wnd_set(vterm_t *vterm,WINDOW *window);
WINDOW*         vterm_wnd_get(vterm_t *vterm);
void            vterm_wnd_update(vterm_t *vterm);
#endif

int             vterm_set_colors(vterm_t *vterm,short fg,short bg);
short           vterm_get_colors(vterm_t *vterm);

void            vterm_erase(vterm_t *vterm);
void            vterm_erase_row(vterm_t *vterm,int row);
void            vterm_erase_rows(vterm_t *vterm,int start_row);
void            vterm_erase_col(vterm_t *vterm,int col);
void            vterm_erase_cols(vterm_t *vterm,int start_cols);
void            vterm_scroll_up(vterm_t *vterm);
void            vterm_scroll_down(vterm_t *vterm);

void            vterm_resize(vterm_t *vterm,uint16_t width,uint16_t height);

/* Needed to allow an app to see what's on screen right now */
void            vterm_render(vterm_t *, const char *data, int len);
void            vterm_get_size(vterm_t *vterm, int *width, int *height);
vterm_cell_t**  vterm_get_buffer(vterm_t *vterm);

/* Needed if we don't use curses */
int             GetFGBGFromColorIndex( int index, int* fg, int* bg );
int             FindColorPair( int fg, int bg );

#endif

