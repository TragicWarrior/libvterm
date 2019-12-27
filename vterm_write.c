
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>

#include "vterm.h"
#include "vterm_private.h"
#include "vterm_write.h"
#include "vterm_error.h"
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
_vterm_write_pty(vterm_t *vterm, unsigned char *buf, ssize_t bytes);

int
vterm_write_pipe(vterm_t *vterm, uint32_t keycode)
{
    int     retval;

    if(vterm == NULL) return -1;

    retval = vterm->write(vterm, keycode);

    if(vterm->event_mask & VTERM_MASK_PIPE_WRITTEN)
    {
        if(vterm->event_hook != NULL)
        {
            vterm->event_hook(vterm, VTERM_MASK_PIPE_WRITTEN, NULL);
        }
    }

    return retval;
}


int
vterm_write_keymap(vterm_t *vterm, uint32_t keycode)
{
    unsigned char           buf[KEY_BUFFER_SZ];
    int                     mapped_key = 0;
    ssize_t                 bytes = 0;
    int                     retval = 0;
    int                     i;

    // zero out buf for safe measure
    memset(buf, 0, sizeof(buf));

    // set the default to Xterm handler
    if(vterm->keymap_str == NULL)
    {
        // default keymap is xterm unless set differently via below
        vterm->keymap_size = ARRAY_SZ(keymap_xterm_val);
        vterm->keymap_str = keymap_xterm_str;
        vterm->keymap_val = keymap_xterm_val;

        if(vterm->flags & VTERM_FLAG_RXVT)
        {
            vterm->keymap_size = ARRAY_SZ(keymap_rxvt_val);
            vterm->keymap_str = keymap_rxvt_str;
            vterm->keymap_val = keymap_rxvt_val;
        }

        if(vterm->flags & VTERM_FLAG_LINUX)
        {
            vterm->keymap_size = ARRAY_SZ(keymap_linux_val);
            vterm->keymap_str = keymap_linux_str;
            vterm->keymap_val = keymap_linux_val;
        }

        if(vterm->flags & VTERM_FLAG_VT100)
        {
            vterm->keymap_size = ARRAY_SZ(keymap_vt100_val);
            vterm->keymap_str = keymap_vt100_str;
            vterm->keymap_val = keymap_vt100_val;
        }
    }

    // look in KEYMAP x-macro table for a match
    for(i = 0; i < vterm->keymap_size; i++)
    {
        // check for end of list
        if(vterm->keymap_val[i] == -1)
        {
            break;
        }

        if(vterm->keymap_val[i] == KEY_DYNAMIC)
        {
            mapped_key = key_defined(vterm->keymap_str[i]);
            if(mapped_key > 0)
            {
                vterm->keymap_val[i] = mapped_key;
            }

            // the key cannot be mapped.  move on.
            if(mapped_key < 1)
            {
                continue;
            }
        }

        // the key keycode is a match
        if(keycode == (uint32_t)vterm->keymap_val[i])
        {
            // cant imagine a sequence longer than 16 bytes
            bytes = strlen(vterm->keymap_str[i]);
            if(bytes > 16) bytes = 16;

            strcpy((char *)buf, vterm->keymap_str[i]);
            break;
        }
    }

    if(keycode == KEY_MOUSE)
    {
        if(vterm->mouse_driver != NULL)
        {
            bytes = vterm->mouse_driver(vterm, buf);

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
        tcgetattr(vterm->pty_fd, &vterm->term_state);

        if(vterm->term_state.c_cc[VERASE] != '\b')
            sprintf((char *)buf, "%c", vterm->term_state.c_cc[VERASE]);
        else
            sprintf((char *)buf, "\b");

        bytes = strlen((char *)buf);
    }

    // if bytes is > 0 we've arleady found something to write
    if(bytes > 0)
    {
        retval = _vterm_write_pty(vterm, buf, bytes);
        return retval;
    }

    // there's probably a more elegant way to do this but
    if(keycode > 0xFFFF)
    {
        buf[0] = (keycode & 0x00FF0000) >> 16;
        buf[1] = (keycode & 0x0000FF00) >> 8;
        buf[2] = (keycode & 0x000000FF);
        bytes = 3;
    }
    else if(keycode > 0xFF)
    {
        buf[0] = (keycode & 0x0000FF00) >> 8;
        buf[1] = (keycode & 0x000000FF);
        bytes = 2;
    }
    else
    {
        buf[0] = (keycode & 0x000000FF);
        bytes = 1;
    }

    retval = _vterm_write_pty(vterm, buf, bytes);

    return retval;
}

int
_vterm_write_pty(vterm_t *vterm, unsigned char *buf, ssize_t bytes)
{
    ssize_t bytes_written = 0;
    int     retval = 0;

    if(buf == NULL) return -1;

    bytes_written = write(vterm->pty_fd, buf, bytes);
    if(bytes_written != bytes)
    {
        vterm_error(vterm, VTERM_ECODE_PTY_WRITE_ERR, NULL);
    }

    return retval;
}
