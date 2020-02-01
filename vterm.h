
/* The NOCURSES flag impacts compile time.
   Contrast with VTERM_FLAG_NOCURSES which does not impact the link
   dependencies but suppresses *use* of curses at runtime.
*/

#ifndef _VTERM_H_
#define _VTERM_H_

#include <inttypes.h>

#include <sys/types.h>

#ifdef __linux__
#include <ncursesw/curses.h>
#endif

#ifdef __FreeBSD__
#include <ncurses/ncurses.h>
#endif

#if defined(__APPLE__) && defined(__MACH__)
#include <ncursesw/ncurses.h>
#endif

#undef TRUE
#define TRUE            1
#undef FALSE
#define FALSE           0

#define LIBVTERM_VERSION        "9.3"

#define VTERM_FLAG_RXVT         (1UL << 0)      //  emulate rxvt
#define VTERM_FLAG_VT100        (1UL << 1)      //  emulate vt100
#define VTERM_FLAG_XTERM        (1UL << 2)      //  emulate xterm (default)
#define VTERM_FLAG_XTERM_256    (1UL << 3)      //  emulate xterm-256
#define VTERM_FLAG_LINUX        (1UL << 4)      //  emulate linux

#define VTERM_FLAG_AIO          (1UL << 7)      //  async i/o
#define VTERM_FLAG_NOPTY        (1UL << 8)      /*
                                                    skip all the fd and pty
                                                    stuff.  just render input
                                                    args byte stream to a
                                                    buffer.
                                                */
#define VTERM_FLAG_NOCURSES     (1UL << 9)      /*
                                                    skip the curses WINDOW
                                                    stuff.  return the char
                                                    cell array if required.
                                                */
#define VTERM_FLAG_NOUTF8       (1UL << 10)     /*  disable the detection
                                                    and handling for UTF-8
                                                    completely
                                                */


#define VTERM_FLAG_C8           (1UL << 14)     /*
                                                    never try to used extended
                                                    colors (aixterm 16 colors).
                                                */

#define VTERM_FLAG_C16          (1UL << 15)     /*
                                                    emulate aixterm 16 colors
                                                    with mapped colors.
                                                */

#define VTERM_FLAG_DUMP         (1UL << 31)     /*
                                                    tell libvterm to write
                                                    stream data to a dump file
                                                    for debugging.
                                                */

/*
    Need this to be public if we want to expose the buffer to callers.

    The ncurses implementation of cchar_t internally packs an attr_t.
    However, the only portable way to access that is via getcchar()
    and setcchar() which are a bit clumsy in their operation.
*/
struct _vterm_cell_s
{
    wchar_t         wch[2];
    attr_t          attr;
    int             colors;
    short           fg;
    short           bg;
    short           f_rgb[3];
    short           b_rgb[3];
    uint8_t         dirty;
};

typedef struct _vterm_cell_s    vterm_cell_t;

typedef struct _vterm_s         vterm_t;

typedef short (*VtermPairSelect) \
                    (vterm_t *vterm, short fg, short bg);

typedef int (*VtermPairSplit) \
                    (vterm_t *vterm, short pair, short *fg, short *bg);

typedef void (*VtermEventHook) \
                    (vterm_t *vterm, int event, void *anything);

/*
    certain events will trigger a callback if it's installed.  the
    callback "hook" is installed via the vterm_install_hook() API.

    the callback is invoked by the library, it supplies 3 arguements...
    a pointer to the vterm object, the event (an integer id), and
    an 'anything' data pointer.  the contents of 'anything' are
    dictated by the type of event and described as following:

    Event                               void *anything
    -----                               --------------
    VTERM_EVENT_BUFFER_ACTIVATED        index of buffer as int*
    VTERM_EVENT_BUFFER_DEACTIVATED      index of buffer as int*
    VTERM_EVENT_BUFFER_PREFLIP          index of new buffer as int*
    VTERM_EVENT_PIPE_READ               bytes read as ssize_t*
    VTERM_EVENT_PIPE_WRITTEN            unused
    VTERM_EVENT_TERM_PRESIZE            unused
    VTERM_EVENT_TERM_RESIZED            size as struct winsize*
    VTERM_EVENT_TERM_PRECLEAR           unused
    VTERM_EVENT_TERM_SCROLLED           direction of scroll as int* (-1 or 1)
*/

enum
{
    VTERM_EVENT_BUFFER_ACTIVATED =      0x10,
    VTERM_EVENT_BUFFER_DEACTIVATED,
    VTERM_EVENT_BUFFER_PREFLIP,
    VTERM_EVENT_PIPE_READ,
    VTERM_EVENT_PIPE_WRITTEN,
    VTERM_EVENT_TERM_PRESIZE,
    VTERM_EVENT_TERM_RESIZED,
    VTERM_EVENT_TERM_PRECLEAR,
    VTERM_EVENT_TERM_SCROLLED,
};

#define VTERM_MASK_BUFFER_ACTIVATED     (1UL << 0)
#define VTERM_MASK_BUFFER_DEACTIVATED   (1UL << 1)
#define VTERM_MASK_BUFFER_PREFLIP       (1UL << 2)
#define VTERM_MASK_BUFFER_READ          (1UL << 3)
#define VTERM_MASK_PIPE_READ            (1UL << 4)
#define VTERM_MASK_PIPE_WRITTEN         (1UL << 5)
#define VTERM_MASK_TERM_PRESIZE         (1UL << 6)
#define VTERM_MASK_TERM_RESIZED         (1UL << 7)
#define VTERM_MASK_TERM_PRECLEAR        (1UL << 8)
#define VTERM_MASK_TERM_SCROLLED        (1UL << 9)

enum
{
    VTERM_BUF_STANDARD    =   0x00,     // normal / standard buffer
    VTERM_BUF_ALTERNATE,                // typically used for lineart progs
    VTERM_BUF_HISTORY                   // the history buffer
};

#define VTERM_WND_LEAVE_DIRTY           (1 << 0)    // don't clear dirty flags
#define VTERM_WND_RENDER_ALL            (1 << 1)    // render all cells

/*
    alloc a raw terminal object.

    @return:        a handle to a alloc'd vterm object.
*/
vterm_t*        vterm_alloc(void);

/*
    init a terminal object with a known height and width and alloc object
    if need be.

    @params:
        vterm           handle an already alloc'd vterm object
        width           desired width
        height          desired height
        flags           defined above as VTERM_FLAG_*

    @return:            a handle to a newly alloc'd vterm object if not
                        provided.
*/
vterm_t*        vterm_init(vterm_t *vterm, uint16_t width, uint16_t height,
                    uint32_t flags);

/*
    determines the number of lines that will be preserved in the history
    buffer.

    @params:
        vterm           handle an already alloc'd vterm object
        rows            the maximum number of lines that will be preserved.
                        when the vterm object is first initialized the
                        default limit is set to 4x the height of the
                        terminal.
*/
void            vterm_set_history_size(vterm_t *vterm, int rows);

/*
    get the maximum number of lines that will be preserved in the history
    buffer.

    @params:
        vterm           handle an already alloc'd vterm object

    @return:            the number of lines preserved by the history buffer.
*/
int            vterm_get_history_size(vterm_t *vterm);

/*
    convenience macro for alloc-ing a ready to use terminal object.

    @params:
        width           desired width
        height          desired height
        flags           defined above as VTERM_FLAG_*

    @return:            a handle to a newly alloc'd vterm object initialized
                        using the specified values.
*/
#define         vterm_create(width, height, flags) \
                    vterm_init(NULL, width, height, flags)

/*
    setup the vterm instance for asynchronous i/o (signal driven).

    @params:
        vterm           handle an already alloc'd vterm object

    @return:            libvterm aggregates and syncrhonizes all of the
                        SIGIO events from async instances using the
                        "self-pipe" trick.  the return value is the file
                        descriptor which can be used with select() or
                        poll() to detect the arrival of new data.
*/
int             vterm_set_async(vterm_t *vterm);

/*
    it's impossible for vterm to have insight into the color map because it
    is setup by the caller outside of the library.  therefore, the default
    function does an expensive iteration through all of the color pairs
    looking for a match everytime it's needed.  the caller probably knows
    a better way so this interface allows a different function to be set.

    @params:
        vterm           handle an already alloc'd vterm object
        ps              a callback which returns the index of a defined
                        color pair.

*/
void            vterm_set_pair_selector(vterm_t *vterm, VtermPairSelect ps);

/*
    returns a callback interface of the current color pair selector.  the
    default is basically a wrapper which leverages pair_content().

    @params:
        vterm           handle an already alloc'd vterm object

    @return:            a function pointer of the type VtermPairSelect
*/
VtermPairSelect vterm_get_pair_selector(vterm_t *vterm);

/*
    see note on vterm_set_pair_selector for rationale behind this API.
    this inteface provides the caller an oppurtunity to provide a more
    efficient means of unpacking a color index into it's respective
    foreground and background colors.

    @params:
        vterm           handle an already alloc'd vterm object
        ps              a callback which splits a color pair into its
                        respective foreground and background colorsl
*/
void            vterm_set_pair_splitter(vterm_t *vterm, VtermPairSplit ps);

/*
    returns a callback interface of the current color pair splitter.  the
    default is basically a wrapper for pair_content().

    @params:
        vterm           handle an already alloc'd vterm object

    @return:            a function pointer of the type VtermPairSplit
*/
VtermPairSplit  vterm_get_pair_splitter(vterm_t *vterm);

/*
    destroy a terminal object.

    @params:
        vterm           a valid vterm object handle.
*/
void            vterm_destroy(vterm_t *vterm);

/*
    fetch the process id (pid) of the terminal.

    @params:
        vterm           a valid vterm object handle.

    @return:            the process id of the terminal.
*/
pid_t           vterm_get_pid(vterm_t *vterm);

/*
    get the file descriptor of the pty.

    @params:
        vterm           a valid vterm object handle.

    @return:            the file descriptor of the terminal.
*/
int             vterm_get_pty_fd(vterm_t *vterm);

/*
    get the name of the tty.

    @params:
        vterm           a valid vterm object handle.

    @return:            the name of the tty.  it should be copied elsewhere
                        if intended to be used longterm.
*/
const char*     vterm_get_ttyname(vterm_t *vterm);

/*
    get the name of the window title (if set by OSC command)

    @params:
        vterm           a valid vterm object handle.
        buf             a buffer provided by the caller which the window
                        title will be copied into.
        buf_sz          the size of the caller provided buffer.  the
                        usable space will be buf_sz - 1 for null termination.
*/
void            vterm_get_title(vterm_t *vterm, char *buf, int buf_sz);

/*
    set a binary and args to launch instead of a shell.

    @params:
        vterm           a valid vterm object handle.
        path            the complete path to the binary (including the binary
                        itself.
        argv            a null-terminated vector of arguments to pass to the
                        binary.
*/
void            vterm_set_exec(vterm_t *vterm, char *path, char **argv);

/*
    read bytes from the terminal.

    @params:
        vterm           a valid vterm object handle.
        timeout         number of milliseconds the API should wait for new
                        data on the read descriptor.  under the hood, this
                        param is passed to poll() so a 0 or a negative
                        value has the same effect that it would on poll().
                        this parameter is irrelevant if the instance has
                        been made asynchronous via the vterm_set_async() api.

    @return:            the amount of bytes read from running program output.
                        this is somewhat analogous to stdout.  returns -1
                        on error.
*/
ssize_t         vterm_read_pipe(vterm_t *vterm, int timeout);

/*
    write a keystroke to the terminal.

    @params:
        vterm           a valid vterm object handle.

    @return:            returns 0 on success and -1 on error.
*/
int             vterm_write_pipe(vterm_t *vterm, uint32_t keycode);

/*
    sets a pointer that can be used for anything.  it is extremely useful
    for handling ancillary data or operations when an event hook is
    triggered.

    @params:
        vterm           a valid vterm object handle.
        anything        user defined data pointer
*/
void            vterm_set_userptr(vterm_t *vterm, void *anything);

/*
    retrieves the pointer to user defined data that gets passed around
    with the vterm_t object.

    @params:
        vterm           a valid vterm object handle.

    @return:            a pointer to user defined data
*/
void*           vterm_get_userptr(vterm_t *vterm);

/*
    installs an event hook.

    @params:
        vterm           a valid vterm object handle.
        hook            a callback that will be invoked by certain events.
*/
void            vterm_install_hook(vterm_t *vterm, VtermEventHook hook);

/*
    returns the current event mask.

    @params:
        vterm           a valid vterm object handle.

    @return:            returns the bitmask of subscribed events
*/
uint32_t        vterm_get_event_mask(vterm_t *vterm);

/*
    sets the mask of events that will be passed to the event hook if
    installed via vterm_install_hook().

    @params:
        vterm           a valid vterm object handle.

        mask            a bitmask of events to subscribe to.
*/
void            vterm_set_event_mask(vterm_t *vterm, uint32_t mask);


#ifndef NOCURSES
/*
    set the WINDOW * to that the terminal will use for output.

    @params:
        vterm           a valid vterm object handle.
        window          a ncurses WINDOW to use as a rendering surface
*/
void            vterm_wnd_set(vterm_t *vterm, WINDOW *window);

/*
    get the WINDOW* that the terminal is using.

    @params:
        vterm           a valid vterm object handle.

    @return:            a pointer to the WINDOW which is set as a rendering
                        surface.  returns null if none is set.
*/
WINDOW*         vterm_wnd_get(vterm_t *vterm);

/*
    get the size of the WINDOW that is attached to the vterm object

    @params:
        vterm           a valid vterm object handle.
        width           pointer to an integer where the width will be returned.
        height          pointer to an integer where the height will be returned.
*/
void            vterm_wnd_size(vterm_t *vterm, int *width, int *height);

/*
    cause updates to the terminal to be rendered

    @params:
        vterm           a valid vterm object handle.
        idx             the index of the buffer to render.  passing a value
                        of -1 will automatically select the active buffer
                        which will either be VTERM_BUF_STANDARD or
                        VTERM_BUF_ALTERNATE.
        flags           currently only VTERM_BUF_LEAVE_DIRTY.  when this
                        flag is set, the dirty flag is not removed from
                        the terminal cells.

    @return:            returns 0 or success or -1 on error if an invalid
                        buffer index was specfied.
*/
int             vterm_wnd_update(vterm_t *vterm, int idx, int offset,
                        uint8_t flags);
#endif

/*
    set the foreground and bakground colors that will be used by
    default on erase operations.

    @params:
        vterm           a valid vterm object handle.
        fg              the default foreground color for the terminal.
        bg              the default background color for the terminal.

    @return:            returns 0 on success and -1 upon error.
*/
int             vterm_set_colors(vterm_t *vterm, short fg, short bg);

/*
    get the color pair number of the default fg/bg combination.

    @params:
        vterm           a valid vterm object handle.

    @return:            returns the color pair index set as the default
                        fg/bg color combination.  returns -1 upon error.
*/
long            vterm_get_colors(vterm_t *vterm);

/*
    add a new color to the instances private palette and map it global
    color table.  this is done automatically when certain xterm OSC
    color defining sequences are interpreted.

    @params:
        vterm           a valid vterm object handle
        color           the color number according to the guest application.
                        a value of -1 indicates an anonymous color is to
                        be used.
        red             red RGB value ranging from 0 - 255
        green           green RGB value ranging from 0 - 255
        blue            blue RGB value ranging from 0 - 255

    @return:            returns the number of the color mapping in the
                        global color space.
*/
short           vterm_add_mapped_color(vterm_t *vterm, short color,
                    float red, float green, float blue);

/*
    queries the vterm instance regarding specific non-standard color
    mapping to determine the actual color number in the global
    color table.

    @params:
        vterm           a valid vterm object handle
        color           the color number according to the guest application.
                        when this is a positive value, arguments red, green,
                        and blue are ignored.

    @return:            returns the number of the color mapping in the
                        global color space.

*/
short               vterm_get_mapped_color(vterm_t *vterm, short color);

/*
    queries the vterm instance for specific RGB color
    to determine if an actual color exits in the global color table.

    @params:
        vterm           a valid vterm object handle
        red             red RGB value ranging from 0 - 255
        green           green RGB value ranging from 0 - 255
        blue            blue RGB value ranging from 0 - 255

    @return:            returns the number of the color mapping in the
                        global color space.

*/
short               vterm_get_mapped_rgb(vterm_t *vterm,
                        float red, float green, float blue);

/*
    releases the color mapping table and frees the related entries in the
    global color table.

    @params:
        vterm           a valid vterm object handle

*/
void                vterm_free_mapped_colors(vterm_t *vterm);



/*
    erase the contents of the terminal.

    @params:
        vterm           handle to a vterm object
        idx             index of the buffer or -1 for current
        fill_char       the ascii character to use for the erase character.
                        if 0 (nul) is specified, then space is used.
*/
void                vterm_erase(vterm_t *vterm, int idx, char fill_char);

/*
    erase the specified row of the terminal.

    @params:
        vterm           handle to a vterm object
        row             zero-based index of the row to delete.  specifying
                        a value of -1 indicates current row.
        reset_colors    a value of TRUE indicates that the erased row should
                        also have its color attributes reset to the default
                        foreground and background colors.
        fill_char       the ascii character to use for the erase character.
                        if 0 (nul) is specified, then space is used.
*/
void                vterm_erase_row(vterm_t *vterm, int row,
                        bool reset_color, char fill_char);

/*
    erase the terminal beginning at a certain row and toward the bottom
    margin.

    @params:
        vterm           handle to a vterm object
        start_row       zero-based index of the row and subsequent rows below
                        to erase.
        fill_char       the ascii character to use for the erase character.
                        if 0 (nul) is specified, then space is used.
*/
void                vterm_erase_rows(vterm_t *vterm, int start_row,
                        char fill_char);

/*
    erase the specified column of the terminal.

    @params:
        vterm           handle to a vterm object
        col             zero-based index of the column to delete.  specifying
                        a value of -1 indicates current column.
        fill_char       the ascii character to use for the erase character.
                        if 0 (nul) is specified, then space is used.
*/
void                vterm_erase_col(vterm_t *vterm, int col, char fill_char);

/*
    erase the terminal at a specific column and toward the right margin.

    @params:
        vterm           handle to a vterm object
        start_col       zero-based index of the column and subsequent columns
                        to the right to erase.
        fill_char       the ascii character to use for the erase character.
                        if 0 (nul) is specified, then space is used.
*/
void                vterm_erase_cols(vterm_t *vterm, int start_col,
                        char fill_char);

/*
    cause the terminal to be scrolled up by one row and placing an empty
    row at the bottom.

    @params:
        vterm           handle to a vterm object
        reset_colors    a value of TRUE indicates that the new empty row
                        should have its color attributes reset to the default
                        foreground and background colors.
*/
void                vterm_scroll_up(vterm_t *vterm, bool reset_colors);

/*
    cause the termianl to be scrolled down by one row and placing an
    empty row at the top.

    @params:
        vterm           handle to a vterm object
        reset_colors    a value of TRUE indicates that the new empty row
                        should have its color attributes reset to the default
                        foreground and background colors.
*/
void                vterm_scroll_down(vterm_t *vterm, bool reset_colors);

/*
    this is a convenience macro to keep original behavior intact for
    applications that just don't care.  it assumes resize occurs from
    the bottom right origin.

    @params:
        vterm           handle to a vterm object
        width           new width of terminal to resize to
        height          new height of terminal to resize to
*/
#define             vterm_resize(vterm, width, height)  \
                        vterm_resize_full(vterm, width, height, 0, 0, 1, 1)

/*
    resize the terminal to a specific size.  when shrinking the terminal,
    excess will be clipped and discarded based on where the grip edges
    are originating from.

    this API is typically used in reponse to a SIGWINCH signal.

    @params:
        vterm           handle to a vterm object
        width           new width of terminal to resize to
        height          new height of terminal to resize to
        grip_top        unused
        grip_left       unused
        grip_right      unused
        grip_bottom     unused
*/
void                vterm_resize_full(vterm_t *vterm,
                        uint16_t width, uint16_t height,
                        int grip_top, int grip_left,
                        int grip_right, int grip_bottom);

/*
    pushes new data into the interpreter.  if a WINDOW has been
    associated with the vterm object, the WINDOW contents are also
    updated.

    @params:
        vterm           handle to a vterm object
        data            data to push throug the intepreter.  typically this
                        comes from vterm_read_pipe().
        len             the length of the data being pushed into the
                        intepreter.
*/
void                vterm_render(vterm_t *vterm, char *data, int len);

/*
    fetches the column and row of the current cursor position.

    @params:
        vterm           handle to a vterm object
        column          a pointer to an integer where the column will be stored
        row             a pointer to an integer where the row will be stored
*/
void                vterm_get_cursor_position (vterm_t *vterm,
                        int *column, int *row);

/*
    fetches the width and height of the current terminal dimentions.

    @params:
        vterm           handle to a vterm object
        width           a pointer to an integer where the width will be stored
        height          a pointer to an integer where the height will be stored
*/
void                vterm_get_size(vterm_t *vterm, int *width, int *height);

/*
    returns a copy of the active buffer screen matrix.

    @params:
        vterm           handle to a vterm object
        rows            integer pointer indicating the row count of the matrix
        cols            integer pointer indicating the column count of the matrix

*/
vterm_cell_t**      vterm_copy_buffer(vterm_t *vterm, int *rows, int *cols);

#endif

