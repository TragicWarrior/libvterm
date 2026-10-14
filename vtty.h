
/* The NOCURSES flag impacts compile time.
   Contrast with VTTY_FLAG_NOCURSES which does not impact the link
   dependencies but suppresses *use* of curses at runtime.
*/

#ifndef _VTTY_H_
#define _VTTY_H_

#include <inttypes.h>
#include <stddef.h>

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

#define LIBVTTY_VERSION         "11.2.1"

#define VTTY_FLAG_RXVT          (1UL << 0)      //  emulate rxvt
#define VTTY_FLAG_VT100         (1UL << 1)      //  emulate vt100
#define VTTY_FLAG_XTERM         (1UL << 2)      //  emulate xterm (default)
#define VTTY_FLAG_XTERM_256     (1UL << 3)      //  emulate xterm-256
#define VTTY_FLAG_LINUX         (1UL << 4)      //  emulate linux
#define VTTY_FLAG_TRUECOLOR     (1UL << 11)     //  set COLORTERM=truecolor

#define VTTY_FLAG_EXTMOUSE      (1UL << 5)      /*
                                                    the host owns the real
                                                    terminal's mouse (e.g.
                                                    libvtty is embedded in a
                                                    window manager like vwm
                                                    that enables mouse itself,
                                                    possibly via raw escapes
                                                    that bypass ncurses' mask
                                                    so has_mouse() can't see
                                                    it).  libvtty then never
                                                    touches ncurses' mousemask;
                                                    it only tracks the child's
                                                    mouse mode and re-encodes
                                                    the events the host hands
                                                    it.  (The child still gets
                                                    its mouse -- this is about
                                                    who drives ncurses, not
                                                    whether mouse works.)
                                                */

#define VTTY_FLAG_START_HOME    (1UL << 6)      /*
                                                    after forkpty, chdir the
                                                    child into the user's home
                                                    directory (HOME, else
                                                    passwd pw_dir) before
                                                    exec'ing the shell or
                                                    alternate binary.  without
                                                    this flag the child inherits
                                                    the host process cwd.
                                                */

#define VTTY_FLAG_AIO           (1UL << 7)      //  async i/o
#define VTTY_FLAG_NOPTY         (1UL << 8)      /*
                                                    skip all the fd and pty
                                                    stuff.  just render input
                                                    args byte stream to a
                                                    buffer.
                                                */
#define VTTY_FLAG_NOCURSES      (1UL << 9)      /*
                                                    skip the curses WINDOW
                                                    stuff.  return the char
                                                    cell array if required.
                                                */
#define VTTY_FLAG_NOUTF8        (1UL << 10)     /*  disable the detection
                                                    and handling for UTF-8
                                                    completely
                                                */


#define VTTY_FLAG_C8            (1UL << 14)     /*
                                                    never try to used extended
                                                    colors (aixterm 16 colors).
                                                */

#define VTTY_FLAG_C16           (1UL << 15)     /*
                                                    emulate aixterm 16 colors
                                                    with mapped colors.
                                                */

#define VTTY_FLAG_DUMP          (1UL << 31)     /*
                                                    tell libvtty to write
                                                    stream data to a dump file
                                                    for debugging.
                                                */

/*
    Need this to be public if we want to expose the buffer to callers.

    The ncurses implementation of cchar_t internally packs an attr_t.
    However, the only portable way to access that is via getcchar()
    and setcchar() which are a bit clumsy in their operation.
*/
struct _vtty_cell_s
{
    wchar_t         wch[2];
    attr_t          attr;
    int             colors;
};

typedef struct _vtty_cell_s     vtty_cell_t;

typedef struct _vtty_s          vtty_t;

typedef short (*VttyPairSelect) \
                    (vtty_t *vtty, short fg, short bg);

typedef int (*VttyPairSplit) \
                    (vtty_t *vtty, short pair, short *fg, short *bg);

typedef void (*VttyEventHook) \
                    (vtty_t *vtty, int event, void *anything);

/*
    payload of VTTY_EVENT_CLIPBOARD.  data is the base64-decoded OSC 52
    SET payload (binary; not necessarily NUL-terminated).  valid only
    for the duration of the hook.  copy it if you need it afterwards.
    a clear (empty Pd) arrives as data == NULL, len == 0.
*/
struct _vtty_clipboard_s
{
    const char      *data;
    size_t          len;
    char            selection;      /* 'c' CLIPBOARD, 'p' PRIMARY,
                                       's' SELECT, '0'-'7' cut buffer;
                                       'c' when Pc is empty          */
};

typedef struct _vtty_clipboard_s    vtty_clipboard_t;

/*
    certain events will trigger a callback if it's installed.  the
    callback "hook" is installed via the vtty_install_hook() API.

    the callback is invoked by the library, it supplies 3 arguements...
    a pointer to the vtty object, the event (an integer id), and
    an 'anything' data pointer.  the contents of 'anything' are
    dictated by the type of event and described as following:

    Event                               void *anything
    -----                               --------------
    VTTY_EVENT_BUFFER_ACTIVATED         index of buffer as int*
    VTTY_EVENT_BUFFER_DEACTIVATED       index of buffer as int*
    VTTY_EVENT_BUFFER_PREFLIP           index of new buffer as int*
    VTTY_EVENT_PIPE_READ                bytes read as ssize_t*
    VTTY_EVENT_PIPE_WRITTEN             unused
    VTTY_EVENT_TERM_PRESIZE             unused
    VTTY_EVENT_TERM_RESIZED             size as struct winsize*
    VTTY_EVENT_TERM_PRECLEAR            unused
    VTTY_EVENT_TERM_SCROLLED            direction of scroll as int* (-1 or 1)
    VTTY_EVENT_CLIPBOARD                vtty_clipboard_t*
    VTTY_EVENT_TITLE                    the new title as const char*
*/

enum
{
    VTTY_EVENT_BUFFER_ACTIVATED =       0x10,
    VTTY_EVENT_BUFFER_DEACTIVATED,
    VTTY_EVENT_BUFFER_PREFLIP,
    VTTY_EVENT_PIPE_READ,
    VTTY_EVENT_PIPE_WRITTEN,
    VTTY_EVENT_TERM_PRESIZE,
    VTTY_EVENT_TERM_RESIZED,
    VTTY_EVENT_TERM_PRECLEAR,
    VTTY_EVENT_TERM_SCROLLED,
    VTTY_EVENT_CLIPBOARD,
    VTTY_EVENT_TITLE,
};

#define VTTY_MASK_BUFFER_ACTIVATED      (1UL << 0)
#define VTTY_MASK_BUFFER_DEACTIVATED    (1UL << 1)
#define VTTY_MASK_BUFFER_PREFLIP        (1UL << 2)
#define VTTY_MASK_BUFFER_READ           (1UL << 3)
#define VTTY_MASK_PIPE_READ             (1UL << 4)
#define VTTY_MASK_PIPE_WRITTEN          (1UL << 5)
#define VTTY_MASK_TERM_PRESIZE          (1UL << 6)
#define VTTY_MASK_TERM_RESIZED          (1UL << 7)
#define VTTY_MASK_TERM_PRECLEAR         (1UL << 8)
#define VTTY_MASK_TERM_SCROLLED         (1UL << 9)
#define VTTY_MASK_CLIPBOARD             (1UL << 10)
#define VTTY_MASK_TITLE                 (1UL << 11)     //  2048

enum
{
    VTTY_BUF_STANDARD     =   0x00,     // normal / standard buffer
    VTTY_BUF_ALTERNATE,                 // typically used for lineart progs
    VTTY_BUF_HISTORY                    // the history buffer
};

#define VTTY_WND_LEAVE_DIRTY            (1 << 0)    // don't clear dirty flags
#define VTTY_WND_RENDER_ALL             (1 << 1)    // render all cells

/*
    alloc a raw terminal object.

    @return:        a handle to a alloc'd vtty object.
*/
vtty_t*         vtty_alloc(void);

/*
    init a terminal object with a known height and width and alloc object
    if need be.

    @params:
        vtty            handle an already alloc'd vtty object
        width           desired width
        height          desired height
        flags           defined above as VTTY_FLAG_*

    @return:            a handle to a newly alloc'd vtty object if not
                        provided.
*/
vtty_t*         vtty_init(vtty_t *vtty, uint16_t width, uint16_t height,
                    uint32_t flags);

/*
    determines the number of lines that will be preserved in the history
    buffer.

    @params:
        vtty            handle an already alloc'd vtty object
        rows            the maximum number of lines that will be preserved.
                        when the vtty object is first initialized the
                        default limit is set to 4x the height of the
                        terminal.
*/
void            vtty_set_history_size(vtty_t *vtty, int rows);

/*
    get the maximum number of lines that will be preserved in the history
    buffer.

    @params:
        vtty            handle an already alloc'd vtty object

    @return:            the number of lines preserved by the history buffer.
*/
int            vtty_get_history_size(vtty_t *vtty);

/*
    get the number of history rows actually populated, as opposed to the
    ring's total capacity reported by vtty_get_history_size.

    @params:
        vtty            handle an already alloc'd vtty object

    @return:            the number of scrollback rows written so far, in the
                        range 0 .. vtty_get_history_size().
*/
int            vtty_get_history_used(vtty_t *vtty);

/*
    get the index of the currently active buffer.  full-screen apps (vim,
    less, myman, ...) switch to the alternate buffer, which has no scrollback.

    @params:
        vtty            handle an already alloc'd vtty object

    @return:            VTTY_BUF_STANDARD or VTTY_BUF_ALTERNATE.
*/
int            vtty_get_active_buffer(vtty_t *vtty);

/*
    convenience macro for alloc-ing a ready to use terminal object.

    @params:
        width           desired width
        height          desired height
        flags           defined above as VTTY_FLAG_*

    @return:            a handle to a newly alloc'd vtty object initialized
                        using the specified values.
*/
#define         vtty_create(width, height, flags) \
                    vtty_init(NULL, width, height, flags)

/*
    setup the vtty instance for asynchronous i/o (signal driven).

    @params:
        vtty            handle an already alloc'd vtty object

    @return:            libvtty aggregates and syncrhonizes all of the
                        SIGIO events from async instances using the
                        "self-pipe" trick.  the return value is the file
                        descriptor which can be used with select() or
                        poll() to detect the arrival of new data.
*/
int             vtty_set_async(vtty_t *vtty);

/*
    it's impossible for vtty to have insight into the color map because it
    is setup by the caller outside of the library.  therefore, the default
    function does an expensive iteration through all of the color pairs
    looking for a match everytime it's needed.  the caller probably knows
    a better way so this interface allows a different function to be set.

    @params:
        vtty            handle an already alloc'd vtty object
        ps              a callback which returns the index of a defined
                        color pair.

*/
void            vtty_set_pair_selector(vtty_t *vtty, VttyPairSelect ps);

/*
    returns a callback interface of the current color pair selector.  the
    default is basically a wrapper which leverages pair_content().

    @params:
        vtty            handle an already alloc'd vtty object

    @return:            a function pointer of the type VttyPairSelect
*/
VttyPairSelect vtty_get_pair_selector(vtty_t *vtty);

/*
    see note on vtty_set_pair_selector for rationale behind this API.
    this inteface provides the caller an oppurtunity to provide a more
    efficient means of unpacking a color index into it's respective
    foreground and background colors.

    @params:
        vtty            handle an already alloc'd vtty object
        ps              a callback which splits a color pair into its
                        respective foreground and background colorsl
*/
void            vtty_set_pair_splitter(vtty_t *vtty, VttyPairSplit ps);

/*
    returns a callback interface of the current color pair splitter.  the
    default is basically a wrapper for pair_content().

    @params:
        vtty            handle an already alloc'd vtty object

    @return:            a function pointer of the type VttyPairSplit
*/
VttyPairSplit   vtty_get_pair_splitter(vtty_t *vtty);

/*
    destroy a terminal object.

    @params:
        vtty            a valid vtty object handle.
*/
void            vtty_destroy(vtty_t *vtty);

/*
    fetch the process id (pid) of the terminal.

    @params:
        vtty            a valid vtty object handle.

    @return:            the process id of the terminal.
*/
pid_t           vtty_get_pid(vtty_t *vtty);

/*
    get the file descriptor of the pty.

    @params:
        vtty            a valid vtty object handle.

    @return:            the file descriptor of the terminal.
*/
int             vtty_get_pty_fd(vtty_t *vtty);

/*
    get the name of the tty.

    @params:
        vtty            a valid vtty object handle.

    @return:            the name of the tty.  it should be copied elsewhere
                        if intended to be used longterm.
*/
const char*     vtty_get_ttyname(vtty_t *vtty);

/*
    get the name of the window title (if set by OSC command)

    the child sets it with OSC 0 or OSC 2.  OSC 1 (the icon name) does
    not change it.  the title is at most 127 bytes; a longer one is cut
    at a character boundary.  an empty title means the child gave the
    title back, and the host should show its own name for the window.

    a host that wants to know when the title changes, instead of
    polling, subscribes to VTTY_MASK_TITLE.  VTTY_EVENT_TITLE then
    arrives with the new title as a const char*, valid for the duration
    of the hook.  it is sent only when the title actually changes.

    @params:
        vtty            a valid vtty object handle.
        buf             a buffer provided by the caller which the window
                        title will be copied into.
        buf_sz          the size of the caller provided buffer.  the
                        usable space will be buf_sz - 1 for null termination.
*/
void            vtty_get_title(vtty_t *vtty, char *buf, int buf_sz);

/*
    last OSC 52 SET received from the child.

    libvtty does not talk to the host clipboard (xclip, OSC 52 out,
    Wayland).  it only decodes the inner hop and stores the payload.
    the embedder decides whether and how to push it onward.

    OSC 52 query (Pd = '?') is refused: no reply is written to the
    child.  an empty Pd clears the stored payload.

    @params:
        vtty            a valid vtty object handle.
        data            optional; set to the decoded bytes (may contain
                        NULs).  pointer is valid until the next SET,
                        clear, or vtty_destroy.
        len             optional; set to the byte length.

    @return:            1 if a payload is stored, 0 if none / cleared,
                        -1 on a NULL vtty.
*/
int             vtty_clipboard_get(vtty_t *vtty, const char **data,
                    size_t *len);

/*
    drop the stored OSC 52 payload.  does not notify the event hook.
*/
void            vtty_clipboard_clear(vtty_t *vtty);

/*
    set a binary and args to launch instead of a shell.

    @params:
        vtty            a valid vtty object handle.
        path            the complete path to the binary (including the binary
                        itself.
        argv            a null-terminated vector of arguments to pass to the
                        binary.
*/
void            vtty_set_exec(vtty_t *vtty, char *path, char **argv);

/*
    read bytes from the terminal.

    @params:
        vtty            a valid vtty object handle.
        timeout         number of milliseconds the API should wait for new
                        data on the read descriptor.  under the hood, this
                        param is passed to poll() so a 0 or a negative
                        value has the same effect that it would on poll().
                        this parameter is irrelevant if the instance has
                        been made asynchronous via the vtty_set_async() api.

    @return:            the amount of bytes read from running program output.
                        this is somewhat analogous to stdout.  returns -1
                        on error.
*/
ssize_t         vtty_read_pipe(vtty_t *vtty, int timeout);

/*
    write a keystroke to the terminal.

    @params:
        vtty            a valid vtty object handle.

    @return:            returns 0 on success and -1 on error.
*/
int             vtty_write_pipe(vtty_t *vtty, uint32_t keycode);

/*
    write a raw byte buffer to the child's pty (paste, bulk input).

    Completes the full write: short writes and EINTR/EAGAIN are retried
    until every byte is delivered or a hard error occurs.  Prefer this
    over a per-byte vtty_write_pipe loop for paste -- one buffer, one
    logical transfer, no dropped characters on a full pty.

    When the child has enabled bracketed paste (DECSET 2004), the payload
    is wrapped with ESC[200~ ... ESC[201~ so the guest can treat it as a
    single paste rather than typed keystrokes.  Per-keystroke
    vtty_write_pipe is not wrapped (that would be wrong for real typing).

    @params:
        vtty            a valid vtty object handle.
        data            bytes to write; may be NULL only when len == 0.
        len             number of bytes in data.

    @return:            len on success, -1 on error.  Bracket wrappers are
                        not counted in the return value.
*/
ssize_t         vtty_write_data(vtty_t *vtty, const void *data, size_t len);

int             vtty_write_mouse_event(vtty_t *vtty, MEVENT *mouse_event);

/*
    sets a pointer that can be used for anything.  it is extremely useful
    for handling ancillary data or operations when an event hook is
    triggered.

    @params:
        vtty            a valid vtty object handle.
        anything        user defined data pointer
*/
void            vtty_set_userptr(vtty_t *vtty, void *anything);

/*
    retrieves the pointer to user defined data that gets passed around
    with the vtty_t object.

    @params:
        vtty            a valid vtty object handle.

    @return:            a pointer to user defined data
*/
void*           vtty_get_userptr(vtty_t *vtty);

/*
    installs an event hook.

    @params:
        vtty            a valid vtty object handle.
        hook            a callback that will be invoked by certain events.
*/
void            vtty_install_hook(vtty_t *vtty, VttyEventHook hook);

/*
    returns the current event mask.

    @params:
        vtty            a valid vtty object handle.

    @return:            returns the bitmask of subscribed events
*/
uint32_t        vtty_get_event_mask(vtty_t *vtty);

/*
    sets the mask of events that will be passed to the event hook if
    installed via vtty_install_hook().

    @params:
        vtty            a valid vtty object handle.

        mask            a bitmask of events to subscribe to.
*/
void            vtty_set_event_mask(vtty_t *vtty, uint32_t mask);


#ifndef NOCURSES
/*
    set the WINDOW * to that the terminal will use for output.

    @params:
        vtty            a valid vtty object handle.
        window          a ncurses WINDOW to use as a rendering surface
*/
void            vtty_wnd_set(vtty_t *vtty, WINDOW *window);

/*
    get the WINDOW* that the terminal is using.

    @params:
        vtty            a valid vtty object handle.

    @return:            a pointer to the WINDOW which is set as a rendering
                        surface.  returns null if none is set.
*/
WINDOW*         vtty_wnd_get(vtty_t *vtty);

/*
    get the size of the WINDOW that is attached to the vtty object

    @params:
        vtty            a valid vtty object handle.
        width           pointer to an integer where the width will be returned.
        height          pointer to an integer where the height will be returned.
*/
void            vtty_wnd_size(vtty_t *vtty, int *width, int *height);

/*
    cause updates to the terminal to be rendered

    @params:
        vtty            a valid vtty object handle.
        idx             the index of the buffer to render.  passing a value
                        of -1 will automatically select the active buffer
                        which will either be VTTY_BUF_STANDARD or
                        VTTY_BUF_ALTERNATE.
        flags           currently only VTTY_BUF_LEAVE_DIRTY.  when this
                        flag is set, the dirty flag is not removed from
                        the terminal cells.

    @return:            returns 0 or success or -1 on error if an invalid
                        buffer index was specfied.
*/
int             vtty_wnd_update(vtty_t *vtty, int idx, int offset,
                        uint8_t flags);

/*
    Render a scrollback view into the attached window: the newest `nlines`
    evicted history rows composed above the head of the live standard screen,
    the way a hardware terminal scrolls back.  nlines == 0 renders the live
    screen; each additional line slides one evicted row in at the top and
    pushes the live screen down one row.  Unlike vtty_wnd_update() with
    VTTY_BUF_HISTORY -- which can only show whole screens of evicted rows --
    this reveals fewer-than-one-screen of history against the live buffer.

    @param vtty:        the vtty instance.
    @param nlines:      scroll-back distance in rows; clamp to
                        0 .. vtty_get_history_used().
    @param flags:       reserved (the whole window is repainted).
    @return:            0 on success, -1 on error.
*/
int             vtty_wnd_scrollback(vtty_t *vtty, int nlines, uint8_t flags);
#endif

/*
    set the foreground and bakground colors that will be used by
    default on erase operations.

    @params:
        vtty            a valid vtty object handle.
        fg              the default foreground color for the terminal.
        bg              the default background color for the terminal.

    @return:            returns 0 on success and -1 upon error.
*/
int             vtty_set_colors(vtty_t *vtty, short fg, short bg);

/*
    get the color pair number of the default fg/bg combination.

    @params:
        vtty            a valid vtty object handle.

    @return:            returns the color pair index set as the default
                        fg/bg color combination.  returns -1 upon error.
*/
long            vtty_get_colors(vtty_t *vtty);

/*
    add a new color to the instances private palette and map it global
    color table.  this is done automatically when certain xterm OSC
    color defining sequences are interpreted.

    @params:
        vtty            a valid vtty object handle
        color           the color number according to the guest application.
                        a value of -1 indicates an anonymous color is to
                        be used.
        red             red RGB value ranging from 0 - 255
        green           green RGB value ranging from 0 - 255
        blue            blue RGB value ranging from 0 - 255

    @return:            returns the number of the color mapping in the
                        global color space.
*/
short           vtty_add_mapped_color(vtty_t *vtty, short color,
                    float red, float green, float blue);

/*
    queries the vtty instance regarding specific non-standard color
    mapping to determine the actual color number in the global
    color table.

    @params:
        vtty            a valid vtty object handle
        color           the color number according to the guest application.
                        when this is a positive value, arguments red, green,
                        and blue are ignored.

    @return:            returns the number of the color mapping in the
                        global color space.

*/
short               vtty_get_mapped_color(vtty_t *vtty, short color);

/*
    queries the vtty instance for specific RGB color
    to determine if an actual color exits in the global color table.

    @params:
        vtty            a valid vtty object handle
        red             red RGB value ranging from 0 - 255
        green           green RGB value ranging from 0 - 255
        blue            blue RGB value ranging from 0 - 255

    @return:            returns the number of the color mapping in the
                        global color space.

*/
short               vtty_get_mapped_rgb(vtty_t *vtty,
                        float red, float green, float blue);

/*
    releases the color mapping table and frees the related entries in the
    global color table.

    @params:
        vtty            a valid vtty object handle

*/
void                vtty_free_mapped_colors(vtty_t *vtty);



/*
    erase the contents of the terminal.

    @params:
        vtty            handle to a vtty object
        idx             index of the buffer or -1 for current
        fill_char       the ascii character to use for the erase character.
                        if 0 (nul) is specified, then space is used.
*/
void                vtty_erase(vtty_t *vtty, int idx, char fill_char);

/*
    erase the specified row of the terminal.

    @params:
        vtty            handle to a vtty object
        row             zero-based index of the row to delete.  specifying
                        a value of -1 indicates current row.
        reset_colors    a value of TRUE indicates that the erased row should
                        also have its color attributes reset to the default
                        foreground and background colors.
        fill_char       the ascii character to use for the erase character.
                        if 0 (nul) is specified, then space is used.
*/
void                vtty_erase_row(vtty_t *vtty, int row,
                        bool reset_color, char fill_char);

/*
    erase the terminal beginning at a certain row and toward the bottom
    margin.

    @params:
        vtty            handle to a vtty object
        start_row       zero-based index of the row and subsequent rows below
                        to erase.
        fill_char       the ascii character to use for the erase character.
                        if 0 (nul) is specified, then space is used.
*/
void                vtty_erase_rows(vtty_t *vtty, int start_row,
                        char fill_char);

/*
    erase the specified column of the terminal.

    @params:
        vtty            handle to a vtty object
        col             zero-based index of the column to delete.  specifying
                        a value of -1 indicates current column.
        fill_char       the ascii character to use for the erase character.
                        if 0 (nul) is specified, then space is used.
*/
void                vtty_erase_col(vtty_t *vtty, int col, char fill_char);

/*
    erase the terminal at a specific column and toward the right margin.

    @params:
        vtty            handle to a vtty object
        start_col       zero-based index of the column and subsequent columns
                        to the right to erase.
        fill_char       the ascii character to use for the erase character.
                        if 0 (nul) is specified, then space is used.
*/
void                vtty_erase_cols(vtty_t *vtty, int start_col,
                        char fill_char);

/*
    cause the terminal to be scrolled up by one row and placing an empty
    row at the bottom.

    @params:
        vtty            handle to a vtty object
        reset_colors    a value of TRUE indicates that the new empty row
                        should have its color attributes reset to the default
                        foreground and background colors.
*/
void                vtty_scroll_up(vtty_t *vtty, bool reset_colors);

/*
    cause the termianl to be scrolled down by one row and placing an
    empty row at the top.

    @params:
        vtty            handle to a vtty object
        reset_colors    a value of TRUE indicates that the new empty row
                        should have its color attributes reset to the default
                        foreground and background colors.
*/
void                vtty_scroll_down(vtty_t *vtty, bool reset_colors);

/*
    this is a convenience macro to keep original behavior intact for
    applications that just don't care.  it assumes resize occurs from
    the bottom right origin.

    @params:
        vtty            handle to a vtty object
        width           new width of terminal to resize to
        height          new height of terminal to resize to
*/
#define             vtty_resize(vtty, width, height)    \
                        vtty_resize_full(vtty, width, height, 0, 0, 1, 1)

/*
    resize the terminal to a specific size.  when shrinking the terminal,
    excess will be clipped and discarded based on where the grip edges
    are originating from.

    this API is typically used in reponse to a SIGWINCH signal.

    @params:
        vtty            handle to a vtty object
        width           new width of terminal to resize to
        height          new height of terminal to resize to
        grip_top        unused
        grip_left       unused
        grip_right      unused
        grip_bottom     unused
*/
void                vtty_resize_full(vtty_t *vtty,
                        uint16_t width, uint16_t height,
                        int grip_top, int grip_left,
                        int grip_right, int grip_bottom);

/*
    pushes new data into the interpreter.  if a WINDOW has been
    associated with the vtty object, the WINDOW contents are also
    updated.

    @params:
        vtty            handle to a vtty object
        data            data to push throug the intepreter.  typically this
                        comes from vtty_read_pipe().
        len             the length of the data being pushed into the
                        intepreter.
*/
void                vtty_render(vtty_t *vtty, char *data, int len);

/*
    fetches the column and row of the current cursor position.

    @params:
        vtty            handle to a vtty object
        column          a pointer to an integer where the column will be stored
        row             a pointer to an integer where the row will be stored
*/
void                vtty_get_cursor_position (vtty_t *vtty,
                        int *column, int *row);

/*
    fetches the width and height of the current terminal dimentions.

    @params:
        vtty            handle to a vtty object
        width           a pointer to an integer where the width will be stored
        height          a pointer to an integer where the height will be stored
*/
void                vtty_get_size(vtty_t *vtty, int *width, int *height);

/*
    returns a copy of the active buffer screen matrix.

    @params:
        vtty            handle to a vtty object
        rows            integer pointer indicating the row count of the matrix
        cols            integer pointer indicating the column count of the matrix

*/
vtty_cell_t**       vtty_copy_buffer(vtty_t *vtty, int *rows, int *cols);

/*
    vtty_copy_scrollback

    Like vtty_copy_buffer, but returns the SCROLLBACK view: the newest
    `nlines` evicted history rows composed above the live screen -- the
    copy-buffer counterpart to vtty_wnd_scrollback.  nlines == 0 yields the
    live buffer.  The returned matrix is freed per row by the caller, exactly
    like vtty_copy_buffer.
*/
vtty_cell_t**       vtty_copy_scrollback(vtty_t *vtty, int nlines,
                        int *rows, int *cols);

#endif

