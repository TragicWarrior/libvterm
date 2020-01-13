
#ifndef _VTERM_PRIVATE_H_
#define _VTERM_PRIVATE_H_

#include <unistd.h>
#include <termios.h>

#include <sys/types.h>

#ifdef __linux__
#include <ncursesw/curses.h>
#endif

#ifdef __FREEBSD__
#include <ncurses/ncurses.h>
#endif

#include "mouse_driver.h"
#include "color_map.h"
#include "color_cache.h"

#define ESEQ_BUF_SIZE           128             // escape buffer max
#define UTF8_BUF_SIZE           5               // 4 bytes + 0-terminator

#define STATE_ALT_CHARSET       (1UL << 1)
#define STATE_ESCAPE_MODE       (1UL << 2)
#define STATE_UTF8_MODE         (1UL << 3)
#define STATE_OSC_MODE          (1UL << 4)

#define STATE_PIPE_ERR          (1UL << 8)
#define STATE_CHILD_EXITED      (1UL << 9)

#define STATE_CURSOR_INVIS      (1UL << 10)
#define STATE_SCROLL_SHORT      (1UL << 11)     /*
                                                    scroll region is not
                                                    full height
                                                */
#define STATE_REPLACE_MODE      (1UL << 12)
#define STATE_NO_WRAP           (1UL << 13)
#define STATE_AUTOMATIC_LF      (1UL << 14)     //  DEC SM "20"
#define STATE_ORIGIN_MODE       (1UL << 15)     /*
                                                    cursor home is relative
                                                    to the scroll region
                                                */

#define IS_MODE_ESCAPED(x)      (x->internal_state & STATE_ESCAPE_MODE)
#define IS_MODE_ACS(x)          (x->internal_state & STATE_ALT_CHARSET)
#define IS_MODE_UTF8(x)         (x->internal_state & STATE_UTF8_MODE)

#define VTERM_TERM_MASK         0xFF            /*
                                                    the lower 8 bits of the
                                                    flags value specify term
                                                    type.
                                                */

#define VTERM_MOUSE_X10         (1 << 1)
#define VTERM_MOUSE_VT200       (1 << 2)
#define VTERM_MOUSE_SGR         (1 << 7)
#define VTERM_MOUSE_HIGHLIGHT   (1 << 12)
#define VTERM_MOUSE_ALTSCROLL   (1 << 13)


/*
    PIPE_BUF is defined as the maximum number of bytes that can be
    written to a pipe atomically.  On many systems, this value
    is relatively low (around 4KB).  We'll set our read size to
    a multiple of this to prevent blocking in the child
    process.
*/
#define MAX_PIPE_READ           (PIPE_BUF * 8)

struct _vterm_desc_s
{
    int             rows, cols;                 // buffer height & width
    vterm_cell_t    **cells;
    // vterm_cell_t    last_cell;               // contents of last cell write

    unsigned long   buffer_state;               // internal state control

    attr_t          curattr;                    // current attribute set
    int             colors;                     // current color pair

    int             default_colors;             // default fg/bg color pair

    int             ccol;                       // current cursor col
    int             crow;                       // current cursor row
    int             scroll_min;                 // top of scrolling region
    int             scroll_max;                 // bottom of scrolling region
    int             max_cols;                   /*
                                                   max width (including
                                                   offscreen columns)
                                                */

    int             saved_x, saved_y;           // saved cursor coords

    int             fg;                         // current fg color
    int             bg;                         // current bg color
    short           f_rgb[3];                   // current fg RGB values
    short           b_rgb[3];                   // current bg RGB values
};

typedef struct _vterm_desc_s    vterm_desc_t;

struct _vterm_s
{
    vterm_desc_t    vterm_desc[3];              // normal buffer and alt buffer
    int             vterm_desc_idx;             // index of active buffer;

    WINDOW          *window;                    // curses window

    color_map_t     *color_map_head;

    char            ttyname[96];                // populated with ttyname_r()

    char            title[128];                 /*
                                                    possibly the name of the
                                                    application running in the
                                                    terminal.  the data is
                                                    supplied by the Xterm OSC
                                                    code sequences.
                                                */
    char            *read_buf;                  /*
                                                    new incoming data goes
                                                    here.
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
#ifndef NOUTF8
    int             utf8_buf_len;               //  number of utf8 bytes
    char            utf8_buf[UTF8_BUF_SIZE];    /*
                                                    0-terminated string that
                                                    is the UTF-8 coding.
                                                */
#endif

    ssize_t         reply_buf_sz;               //  size of reply
    char            reply_buf[32];              /*
                                                    some CSI sequences
                                                    expect a reply.  here's
                                                    where they go.
                                                */
    int             pty_fd;                     /*
                                                    file descriptor for the pty
                                                    attached to this terminal.
                                                */
    int             sig_fd;                     /*
                                                    the file descriptor which
                                                    indicates a signal has
                                                    arrived.
                                                */ 
    pid_t           child_pid;                  //  pid of the child process
    uint32_t        flags;                      //  user options
    unsigned long   internal_state;             //  internal state control

    uint16_t        mouse_mode;                 //  mouse mode
    mouse_config_t  *mouse_config;              /*
                                                    saves and restores the
                                                    state of the mouse
                                                    if we are sharing it with
                                                    others.
                                                */
#ifdef __FreeBSD__
    char            padding[20];                //  without this padding FreeBSD
#endif                                          //  trashes mouse_config and
                                                //  keymap_str causing all sorts
                                                //  of bad things to happen

    char            **keymap_str;               //  points to keymap key
    int             *keymap_val;                //  points to keymap ke value
    int             keymap_size;                //  size of the keymap

    struct termios  term_state;                 /*
                                                    stores data returned from
                                                    tcgetattr()
                                                */
    char            *exec_path;                 //  optional binary path to use
    char            **exec_argv;                //  instead of starting shell.

    char            *debug_filepath;
    int             debug_fd;

    uint32_t        event_mask;                 /*
                                                    specificies which events
                                                    should be passed on to
                                                    the custom event hook.
                                                */

    void            *userptr;                   //  user-defined data

    // internal callbacks
    int             (*write)            (vterm_t *, uint32_t);
    int             (*esc_handler)      (vterm_t *);
    int             (*rs1_reset)        (vterm_t *, char *);
    ssize_t         (*mouse_driver)     (vterm_t *, unsigned char *);

    // callbacks that the implementer can specify
    void            (*event_hook)       (vterm_t *, int, void *);
    short           (*pair_select)      (vterm_t *, short, short);
    int             (*pair_split)       (vterm_t *, short, short *, short *);
};

#define VTERM_CELL(vterm_ptr, x, y)     ((y * vterm_ptr->cols) + x)

#endif
