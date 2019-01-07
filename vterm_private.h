
#ifndef _VTERM_PRIVATE_H_
#define _VTERM_PRIVATE_H_

#include <unistd.h>

#include <sys/types.h>

#include "vterm_colors.h"

#ifndef NOCURSES
#  include <ncursesw/curses.h>
#endif

#define ESEQ_BUF_SIZE           128             // escape buffer max
#define UTF8_BUF_SIZE           5               // 4 bytes + 0-terminator

#define STATE_ALT_CHARSET       (1 << 1)
#define STATE_ESCAPE_MODE       (1 << 2)
#define STATE_UTF8_MODE         (1 << 3)

#define STATE_PIPE_ERR          (1 << 8)
#define STATE_CHILD_EXITED      (1 << 9)
#define STATE_CURSOR_INVIS      (1 << 10)
#define STATE_SCROLL_SHORT      (1 << 11)       /*
                                                    scroll region is not
                                                    full height
                                                */

#define IS_MODE_ESCAPED(x)      (x->internal_state & STATE_ESCAPE_MODE)
#define IS_MODE_ACS(x)          (x->internal_state & STATE_ALT_CHARSET)
#define IS_MODE_UTF8(x)         (x->internal_state & STATE_UTF8_MODE)

#define VTERM_TERM_MASK         0x0F            /*
                                                    the lower 4 bits of the
                                                    flags value specify term
                                                    type.
                                                */

#define VTERM_MOUSE_X10         9
#define VTERM_MOUSE_VT2000      1000
#define VTERM_MOUSE_SGR         1006

struct _vterm_desc_s
{
    int             rows, cols;                 // terminal height & width
    vterm_cell_t    **cells;
    vterm_cell_t    last_cell;                  // contents of last cell write

    unsigned int    buffer_state;               // internal state control

    attr_t          curattr;                    // current attribute set
    int             crow, ccol;                 // current cursor column & row
    int             scroll_min;                 // top of scrolling region
    int             scroll_max;                 // bottom of scrolling region
    int             saved_x, saved_y;           // saved cursor coords
    int             fg, bg;                     // current fg/bg colors
    short           colors;                     // color pair for default fg/bg
};

typedef struct _vterm_desc_s    vterm_desc_t;

struct _vterm_s
{
    vterm_desc_t    vterm_desc[2];              // normal buffer and alt buffer
    int             vterm_desc_idx;             // index of active buffer;

    color_cache_t   *color_cache;               // color cache

#ifndef NOCURSES
    WINDOW          *window;                    // curses window
#endif
    char            ttyname[96];                // populated with ttyname_r()

    char            title[128];                 /*
                                                    possibly the name of the
                                                    application running in the
                                                    terminal.  the data is
                                                    supplied by the Xterm OSC
                                                    code sequences.
                                                */


    char            esbuf[ESEQ_BUF_SIZE];       /*
                                                    0-terminated string. Does
                                                    NOT include the initial
                                                    escape (\x1B) character.
                                                */

    int             esbuf_len;                  /*
                                                    length of buffer. The
                                                    following property is
                                                    always kept:
                                                    esbuf[esbuf_len] == '\0'
                                                */

    char            utf8_buf[UTF8_BUF_SIZE];    /*
                                                    0-terminated string that
                                                    is the UTF-8 coding.
                                                */

    int             utf8_buf_len;               //  number of utf8 bytes

    int             pty_fd;                     /*
                                                    file descriptor for the pty
                                                    attached to this terminal.
                                                */

    pid_t           child_pid;                  //  pid of the child process
    unsigned int    flags;                      //  user options
    unsigned int    internal_state;             //  internal state control

    unsigned int    mouse;                      //  mouse mode

    char            *exec_path;                 //  optional binary path to use
    char            **exec_argv;                //  instead of starting shell.

    char            *debug_filepath;
    int             debug_fd;

    // internal callbacks
    int             (*write)            (vterm_t *, uint32_t);
    int             (*esc_handler)      (vterm_t *);
    int             (*rs1_reset)        (vterm_t *, char *);

    // callbacks that the implementer can specify
    void            (*event_hook)       (vterm_t *, int, void *);
    short           (*pair_select)      (vterm_t *, short, short);
    int             (*pair_split)       (vterm_t *, short, short *, short *);
};

#define VTERM_CELL(vterm_ptr,x,y)   ((y * vterm_ptr->cols) + x)

#endif
