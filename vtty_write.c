
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <poll.h>
#include <limits.h>

#include "vtty.h"
#include "vtty_private.h"
#include "vtty_write.h"
#include "vtty_error.h"
#include "strings.h"
#include "stringv.h"
#include "macros.h"

#define KEY_BUFFER_SZ           64

#define KEY_DYNAMIC             (KEY_MAX + 1)

// necessary for KEYMAP x-macro to work correctly
// standard function keys
#define KEY_F1                  KEY_F(1)
#define KEY_F2                  KEY_F(2)
#define KEY_F3                  KEY_F(3)
#define KEY_F4                  KEY_F(4)
#define KEY_F5                  KEY_F(5)
#define KEY_F6                  KEY_F(6)
#define KEY_F7                  KEY_F(7)
#define KEY_F8                  KEY_F(8)
#define KEY_F9                  KEY_F(9)
#define KEY_F10                 KEY_F(10)
#define KEY_F11                 KEY_F(11)
#define KEY_F12                 KEY_F(12)

// shift + function keys
#define SHIFT_KEY_F1            KEY_F(13)
#define SHIFT_KEY_F2            KEY_F(14)
#define SHIFT_KEY_F3            KEY_F(15)
#define SHIFT_KEY_F4            KEY_F(16)
#define SHIFT_KEY_F5            KEY_F(17)
#define SHIFT_KEY_F6            KEY_F(18)
#define SHIFT_KEY_F7            KEY_F(19)
#define SHIFT_KEY_F8            KEY_F(20)
#define SHIFT_KEY_F9            KEY_F(21)
#define SHIFT_KEY_F10           KEY_F(22)
#define SHIFT_KEY_F11           KEY_F(23)
#define SHIFT_KEY_F12           KEY_F(24)

// ctrl + function keys
#define CTRL_KEY_F1             KEY_F(25)
#define CTRL_KEY_F2             KEY_F(26)
#define CTRL_KEY_F3             KEY_F(27)
#define CTRL_KEY_F4             KEY_F(28)
#define CTRL_KEY_F5             KEY_F(29)
#define CTRL_KEY_F6             KEY_F(30)
#define CTRL_KEY_F7             KEY_F(31)
#define CTRL_KEY_F8             KEY_F(32)
#define CTRL_KEY_F9             KEY_F(33)
#define CTRL_KEY_F10            KEY_F(34)
#define CTRL_KEY_F11            KEY_F(35)
#define CTRL_KEY_F12            KEY_F(36)

// modified home key
#define SHIFT_HOME              KEY_DYNAMIC
#define ALT_HOME                KEY_DYNAMIC
#define ALT_SHIFT_HOME          KEY_DYNAMIC
#define CTRL_HOME               KEY_DYNAMIC
#define CTRL_SHIFT_HOME         KEY_DYNAMIC
#define CTRL_ALT_HOME           KEY_DYNAMIC
#define CTRL_ALT_SHIFT_HOME     KEY_DYNAMIC

// modified end key
#define SHIFT_END               KEY_DYNAMIC
#define ALT_END                 KEY_DYNAMIC
#define ALT_SHIFT_END           KEY_DYNAMIC
#define CTRL_END                KEY_DYNAMIC
#define CTRL_SHIFT_END          KEY_DYNAMIC
#define CTRL_ALT_END            KEY_DYNAMIC
#define CTRL_ALT_SHIFT_END      KEY_DYNAMIC

// modified up key
#define SHIFT_UP                KEY_DYNAMIC
#define ALT_UP                  KEY_DYNAMIC
#define ALT_SHIFT_UP            KEY_DYNAMIC
#define CTRL_UP                 KEY_DYNAMIC
#define CTRL_SHIFT_UP           KEY_DYNAMIC
#define CTRL_ALT_UP             KEY_DYNAMIC
#define CTRL_ALT_SHIFT_UP       KEY_DYNAMIC

// modified down key
#define SHIFT_DOWN              KEY_DYNAMIC
#define ALT_DOWN                KEY_DYNAMIC
#define ALT_SHIFT_DOWN          KEY_DYNAMIC
#define CTRL_DOWN               KEY_DYNAMIC
#define CTRL_SHIFT_DOWN         KEY_DYNAMIC
#define CTRL_ALT_DOWN           KEY_DYNAMIC
#define CTRL_ALT_SHIFT_DOWN     KEY_DYNAMIC

// modified right key
#define SHIFT_RIGHT             KEY_DYNAMIC
#define ALT_RIGHT               KEY_DYNAMIC
#define ALT_SHIFT_RIGHT         KEY_DYNAMIC
#define CTRL_RIGHT              KEY_DYNAMIC
#define CTRL_SHIFT_RIGHT        KEY_DYNAMIC
#define CTRL_ALT_RIGHT          KEY_DYNAMIC
#define CTRL_ALT_SHIFT_RIGHT    KEY_DYNAMIC

// modified left key
#define SHIFT_LEFT              KEY_DYNAMIC
#define ALT_LEFT                KEY_DYNAMIC
#define ALT_SHIFT_LEFT          KEY_DYNAMIC
#define CTRL_LEFT               KEY_DYNAMIC
#define CTRL_SHIFT_LEFT         KEY_DYNAMIC
#define CTRL_ALT_LEFT           KEY_DYNAMIC
#define CTRL_ALT_SHIFT_LEFT     KEY_DYNAMIC

// modified insert key
#define SHIFT_IC                KEY_DYNAMIC
#define ALT_IC                  KEY_DYNAMIC
#define ALT_SHIFT_IC            KEY_DYNAMIC
#define CTRL_IC                 KEY_DYNAMIC
#define CTRL_SHIFT_IC           KEY_DYNAMIC
#define CTRL_ALT_IC             KEY_DYNAMIC
#define CTRL_ALT_SHIFT_IC       KEY_DYNAMIC

// modified delete key
#define SHIFT_DC                KEY_DYNAMIC
#define ALT_DC                  KEY_DYNAMIC
#define ALT_SHIFT_DC            KEY_DYNAMIC
#define CTRL_DC                 KEY_DYNAMIC
#define CTRL_SHIFT_DC           KEY_DYNAMIC
#define CTRL_ALT_DC             KEY_DYNAMIC
#define CTRL_ALT_SHIFT_DC       KEY_DYNAMIC

// modified page up
#define SHIFT_PPAGE             KEY_DYNAMIC
#define ALT_PPAGE               KEY_DYNAMIC
#define ALT_SHIFT_PPAGE         KEY_DYNAMIC
#define CTRL_PPAGE              KEY_DYNAMIC
#define CTRL_SHIFT_PPAGE        KEY_DYNAMIC
#define CTRL_SHIFT_PPAGE        KEY_DYNAMIC
#define CTRL_ALT_PPAGE          KEY_DYNAMIC
#define CTRL_ALT_SHIFT_PPAGE    KEY_DYNAMIC

// modified page down
#define SHIFT_NPAGE             KEY_DYNAMIC
#define ALT_NPAGE               KEY_DYNAMIC
#define ALT_SHIFT_NPAGE         KEY_DYNAMIC
#define CTRL_NPAGE              KEY_DYNAMIC
#define CTRL_SHIFT_NPAGE        KEY_DYNAMIC
#define CTRL_SHIFT_NPAGE        KEY_DYNAMIC
#define CTRL_ALT_NPAGE          KEY_DYNAMIC
#define CTRL_ALT_SHIFT_NPAGE    KEY_DYNAMIC




// load keymap table for xterm and xterm256
#define KEYMAP(k, s)    k,
static int keymap_xterm_val[] = {
#include "keymap_xterm.def"
};
#undef KEYMAP

#define KEYMAP(k, s)    s,
static char *keymap_xterm_str[] = {
#include "keymap_xterm.def"
};
#undef KEYMAP

// load keymap table for rxvt
#define KEYMAP(k, s)    k,
static int keymap_rxvt_val[] = {
#include "keymap_rxvt.def"
};
#undef KEYMAP

#define KEYMAP(k, s)    s,
static char *keymap_rxvt_str[] = {
#include "keymap_rxvt.def"
};
#undef KEYMAP

// load keymap table for linux
#define KEYMAP(k, s)    k,
static int keymap_linux_val[] = {
#include "keymap_linux.def"
};
#undef KEYMAP

#define KEYMAP(k, s)    s,
static char *keymap_linux_str[] = {
#include "keymap_linux.def"
};
#undef KEYMAP

// load keymap table for vt100
#define KEYMAP(k, s)    k,
static int keymap_vt100_val[] = {
#include "keymap_vt100.def"
};
#undef KEYMAP

#define KEYMAP(k, s)    s,
static char *keymap_vt100_str[] = {
#include "keymap_vt100.def"
};
#undef KEYMAP


typedef union _key_alignment_u  key_alignment_t;


int
_vtty_write_pty(vtty_t *vtty, const unsigned char *buf, ssize_t bytes);

int
vtty_write_pipe(vtty_t *vtty, uint32_t keycode)
{
    int     retval;

    if(vtty == NULL) return -1;

    retval = vtty->write(vtty, keycode);

    if(vtty->event_mask & VTTY_MASK_PIPE_WRITTEN)
    {
        if(vtty->event_hook != NULL)
        {
            vtty->event_hook(vtty, VTTY_MASK_PIPE_WRITTEN, NULL);
        }
    }

    return retval;
}


int
vtty_write_keymap(vtty_t *vtty, uint32_t keycode)
{
    unsigned char           buf[KEY_BUFFER_SZ];
    int                     mapped_key = 0;
    ssize_t                 bytes = 0;
    int                     retval = 0;
    int                     i;

    // zero out buf for safe measure
    memset(buf, 0, sizeof(buf));

    // set the default to Xterm handler
    if(vtty->keymap_str == NULL)
    {
        // default keymap is xterm unless set differently via below
        vtty->keymap_size = ARRAY_SZ(keymap_xterm_val);
        vtty->keymap_str = keymap_xterm_str;
        vtty->keymap_val = keymap_xterm_val;

        if(vtty->flags & VTTY_FLAG_RXVT)
        {
            vtty->keymap_size = ARRAY_SZ(keymap_rxvt_val);
            vtty->keymap_str = keymap_rxvt_str;
            vtty->keymap_val = keymap_rxvt_val;
        }

        if(vtty->flags & VTTY_FLAG_LINUX)
        {
            vtty->keymap_size = ARRAY_SZ(keymap_linux_val);
            vtty->keymap_str = keymap_linux_str;
            vtty->keymap_val = keymap_linux_val;
        }

        if(vtty->flags & VTTY_FLAG_VT100)
        {
            vtty->keymap_size = ARRAY_SZ(keymap_vt100_val);
            vtty->keymap_str = keymap_vt100_str;
            vtty->keymap_val = keymap_vt100_val;
        }
    }

    // look in KEYMAP x-macro table for a match
    for(i = 0; i < vtty->keymap_size; i++)
    {
        // check for end of list
        if(vtty->keymap_val[i] == -1)
        {
            break;
        }

        if(vtty->keymap_val[i] == KEY_DYNAMIC)
        {
            mapped_key = key_defined(vtty->keymap_str[i]);
            if(mapped_key > 0)
            {
                vtty->keymap_val[i] = mapped_key;
            }

            // the key cannot be mapped.  move on.
            if(mapped_key < 1)
            {
                continue;
            }
        }

        // the key keycode is a match
        if(keycode == (uint32_t)vtty->keymap_val[i])
        {
            // cant imagine a sequence longer than 16 bytes
            bytes = strlen(vtty->keymap_str[i]);
            if(bytes > 16) bytes = 16;

            strcpy((char *)buf, vtty->keymap_str[i]);
            break;
        }
    }

    if(bytes == 3 && buf[0] == '\x1b' && buf[1] == 'O'
        && !(vtty->internal_state & STATE_CURSOR_APP))
    {
        char suffix = buf[2];
        if(suffix == 'A' || suffix == 'B' || suffix == 'C' || suffix == 'D'
            || suffix == 'H' || suffix == 'F')
        {
            buf[1] = '[';
        }
    }

    if(keycode == KEY_MOUSE)
    {
        if(vtty->mouse_driver != NULL)
        {
            bytes = vtty->mouse_driver(vtty, buf);

            /*
                if bytes == 0 then
                there was a mouse event but it was unhandled for some
                reason so we need to move along.
            */
        }

        /*
            don't pass unhandled mouse senquences through.
            bad things will happen.
        */
        if(bytes == 0) return 0;
    }

    if(keycode == KEY_BACKSPACE)
    {
        tcgetattr(vtty->pty_fd, &vtty->term_state);

        if(vtty->term_state.c_cc[VERASE] != '\b')
            sprintf((char *)buf, "%c", vtty->term_state.c_cc[VERASE]);
        else
            sprintf((char *)buf, "\b");

        bytes = strlen((char *)buf);
    }

    /*
        Function keys as a last-chance map.  If the terminfo table walk
        missed KEY_F(n) -- wrong curses, KEY_DYNAMIC mutation, stale .so
        -- the pack fallback below would emit KEY_F(10)=274 as 0x12 0x01
        and the child would never see F10.
    */
    if(bytes == 0)
    {
        static const struct
        {
            uint32_t    key;
            const char  *seq;
        } fkeys[] =
        {
            { KEY_F(1),  "\x1bOP" },
            { KEY_F(2),  "\x1bOQ" },
            { KEY_F(3),  "\x1bOR" },
            { KEY_F(4),  "\x1bOS" },
            { KEY_F(5),  "\x1b[15~" },
            { KEY_F(6),  "\x1b[17~" },
            { KEY_F(7),  "\x1b[18~" },
            { KEY_F(8),  "\x1b[19~" },
            { KEY_F(9),  "\x1b[20~" },
            { KEY_F(10), "\x1b[21~" },
            { KEY_F(11), "\x1b[23~" },
            { KEY_F(12), "\x1b[24~" },
        };
        int fi;

        for(fi = 0; fi < (int)(sizeof(fkeys) / sizeof(fkeys[0])); fi++)
        {
            if(keycode == fkeys[fi].key)
            {
                bytes = (int)strlen(fkeys[fi].seq);
                memcpy(buf, fkeys[fi].seq, (size_t)bytes);
                break;
            }
        }
    }

    // if bytes is > 0 we've arleady found something to write
    if(bytes > 0)
    {
        retval = _vtty_write_pty(vtty, buf, bytes);
        return retval;
    }

    /*
        ncurses KEY_* (KEY_MIN..KEY_MAX) are not packed kmio sequences.
        Emitting the low two bytes is never a valid keystroke.
    */
    if(keycode >= (uint32_t)KEY_MIN && keycode <= (uint32_t)KEY_MAX)
        return -1;

    /*
        Multi-byte keycodes from the host kmio layer pack wire bytes in
        chronological order into the low half of the word: first byte in
        bits 0-7 (ESC for Alt combos), second in 8-15, third in 16-23.
        Emit low-to-high so ESC-V leaves as ESC then V, not V then ESC.
        The old high-to-low unpack inverted every packed sequence and
        mangled host bracketed-paste wrappers (ESC[200~ / ESC[201~).
    */
    if(keycode > 0xFFFF)
    {
        buf[0] = (unsigned char)(keycode & 0x000000FF);
        buf[1] = (unsigned char)((keycode & 0x0000FF00) >> 8);
        buf[2] = (unsigned char)((keycode & 0x00FF0000) >> 16);
        bytes = 3;
    }
    else if(keycode > 0xFF)
    {
        buf[0] = (unsigned char)(keycode & 0x000000FF);
        buf[1] = (unsigned char)((keycode & 0x0000FF00) >> 8);
        bytes = 2;
    }
    else
    {
        buf[0] = (unsigned char)(keycode & 0x000000FF);
        bytes = 1;
    }

    retval = _vtty_write_pty(vtty, buf, bytes);

    return retval;
}

/*
    Deliver the full buffer to the pty master.  Retries short writes and
    EINTR; on EAGAIN waits for POLLOUT (the master is nonblocking).  A
    single write() used to drop the remainder on short write or EAGAIN --
    silent character loss under paste load.
*/
int
_vtty_write_pty(vtty_t *vtty, const unsigned char *buf, ssize_t bytes)
{
    ssize_t             total = 0;
    ssize_t             n;
    struct pollfd       pfd;

    if(buf == NULL || bytes < 0) return -1;
    if(bytes == 0) return 0;
    if(vtty == NULL || vtty->pty_fd < 0) return -1;

    while(total < bytes)
    {
        n = write(vtty->pty_fd, buf + total, (size_t)(bytes - total));
        if(n > 0)
        {
            total += n;
            continue;
        }

        if(n < 0 && errno == EINTR)
            continue;

        if(n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
        {
            memset(&pfd, 0, sizeof(pfd));
            pfd.fd = vtty->pty_fd;
            pfd.events = POLLOUT;
            if(poll(&pfd, 1, -1) < 0)
            {
                if(errno == EINTR)
                    continue;
                vtty_error(vtty, VTTY_ECODE_PTY_WRITE_ERR, NULL);
                return -1;
            }
            continue;
        }

        vtty_error(vtty, VTTY_ECODE_PTY_WRITE_ERR, NULL);
        return -1;
    }

    return (int)total;
}

ssize_t
vtty_write_data(vtty_t *vtty, const void *data, size_t len)
{
    static const unsigned char bp_start[] = { 0x1b, '[', '2', '0', '0', '~' };
    static const unsigned char bp_end[]   = { 0x1b, '[', '2', '0', '1', '~' };
    const unsigned char        *buf;
    int                         bracket;

    if(vtty == NULL) return -1;
    if(len == 0) return 0;
    if(data == NULL) return -1;

    /* size_t -> ssize_t: reject absurd lengths rather than truncate */
    if(len > (size_t)SSIZE_MAX) return -1;

    buf = (const unsigned char *)data;
    bracket = (vtty->internal_state & STATE_BRACKETED_PASTE) ? 1 : 0;

    if(bracket)
    {
        if(_vtty_write_pty(vtty, bp_start, (ssize_t)sizeof(bp_start)) < 0)
            return -1;
    }

    if(_vtty_write_pty(vtty, buf, (ssize_t)len) < 0)
        return -1;

    if(bracket)
    {
        if(_vtty_write_pty(vtty, bp_end, (ssize_t)sizeof(bp_end)) < 0)
            return -1;
    }

    if(vtty->event_mask & VTTY_MASK_PIPE_WRITTEN)
    {
        if(vtty->event_hook != NULL)
            vtty->event_hook(vtty, VTTY_MASK_PIPE_WRITTEN, NULL);
    }

    return (ssize_t)len;
}

int
vtty_write_mouse_event(vtty_t *vtty, MEVENT *mouse_event)
{
    unsigned char   buf[64];
    ssize_t         bytes;

    if(vtty == NULL || mouse_event == NULL) return -1;
    if(vtty->mouse_driver == NULL) return 0;

    vtty->pending_mouse = mouse_event;

    if(mouse_event->bstate & BUTTON1_CLICKED)
    {
        MEVENT synth = *mouse_event;

        synth.bstate = (synth.bstate & ~BUTTON1_CLICKED) | BUTTON1_PRESSED;
        vtty->pending_mouse = &synth;
        bytes = vtty->mouse_driver(vtty, buf);
        if(bytes > 0) _vtty_write_pty(vtty, buf, bytes);

        synth.bstate = (synth.bstate & ~BUTTON1_PRESSED) | BUTTON1_RELEASED;
        vtty->pending_mouse = &synth;
        bytes = vtty->mouse_driver(vtty, buf);
        if(bytes > 0) _vtty_write_pty(vtty, buf, bytes);

        vtty->pending_mouse = NULL;
        return 1;
    }

    bytes = vtty->mouse_driver(vtty, buf);
    vtty->pending_mouse = NULL;

    if(bytes > 0)
        return _vtty_write_pty(vtty, buf, bytes);

    return 0;
}
