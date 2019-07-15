
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>

#include "vterm.h"
#include "vterm_private.h"
#include "vterm_write.h"
#include "strings.h"
#include "stringv.h"
#include "macros.h"

// necessary for KEYMAP x-macro to work correctly
#define KEY_F1      KEY_F(1)
#define KEY_F2      KEY_F(2)
#define KEY_F3      KEY_F(3)
#define KEY_F4      KEY_F(4)
#define KEY_F5      KEY_F(5)
#define KEY_F6      KEY_F(6)
#define KEY_F7      KEY_F(7)
#define KEY_F8      KEY_F(8)
#define KEY_F9      KEY_F(9)
#define KEY_F10     KEY_F(10)
#define KEY_F11     KEY_F(11)
#define KEY_F12     KEY_F(12)


// load keymap table for xterm and xterm256
#define KEYMAP(k, s)    k,
uint32_t keymap_xterm_val[] = {
#include "keymap_xterm.def"
};
#undef KEYMAP

#define KEYMAP(k, s)    s,
char *keymap_xterm_str[] = {
#include "keymap_xterm.def"
};
#undef KEYMAP

// load keymap table for rxvt
#define KEYMAP(k, s)    k,
uint32_t keymap_rxvt_val[] = {
#include "keymap_rxvt.def"
};
#undef KEYMAP

#define KEYMAP(k, s)    s,
char *keymap_rxvt_str[] = {
#include "keymap_rxvt.def"
};
#undef KEYMAP

// load keymap table for linux
#define KEYMAP(k, s)    k,
uint32_t keymap_linux_val[] = {
#include "keymap_linux.def"
};
#undef KEYMAP

#define KEYMAP(k, s)    s,
char *keymap_linux_str[] = {
#include "keymap_linux.def"
};
#undef KEYMAP

// load keymap table for vt100
#define KEYMAP(k, s)    k,
uint32_t keymap_vt100_val[] = {
#include "keymap_vt100.def"
};
#undef KEYMAP

#define KEYMAP(k, s)    s,
char *keymap_vt100_str[] = {
#include "keymap_vt100.def"
};
#undef KEYMAP


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
    static struct termios   term_state;
    unsigned char           buf[64];
    ssize_t                 bytes = 0;
    int                     retval = 0;
    static char             **keymap_str;
    static uint32_t         *keymap_val;
    static int              array_sz = 0;
    int                     i;

    // if the array size is 0 then we need to setup our map pointers
    if(array_sz == 0)
    {
        if(vterm->flags & VTERM_FLAG_XTERM ||
            vterm->flags & VTERM_FLAG_XTERM_256)
        {
            array_sz = ARRAY_SZ(keymap_xterm_val);
            keymap_str = keymap_xterm_str;
            keymap_val = keymap_xterm_val;
        }

        if(vterm->flags & VTERM_FLAG_RXVT)
        {
            array_sz = ARRAY_SZ(keymap_rxvt_val);
            keymap_str = keymap_rxvt_str;
            keymap_val = keymap_rxvt_val;
        }

        if(vterm->flags & VTERM_FLAG_LINUX)
        {
            array_sz = ARRAY_SZ(keymap_linux_val);
            keymap_str = keymap_linux_str;
            keymap_val = keymap_linux_val;
        }
    }

    // look in KEYMAP x-macro table for a match
    for(i = 0; i < array_sz; i++)
    {
        // the key keycode is a match
        if(keycode == keymap_val[i])
        {
            bytes = strlen(keymap_xterm_str[i]);
            strncpy((char *)buf, keymap_str[i], bytes);
            break;
        }
    }

    if(keycode == KEY_BACKSPACE)
    {
        tcgetattr(vterm->pty_fd, &term_state);

        if(term_state.c_cc[VERASE] != '0')
            sprintf((char *)buf, "%c", term_state.c_cc[VERASE]);
        else
            sprintf((char *)buf, "\b");

        bytes = strlen((char *)buf);
    }


    if(keycode == KEY_MOUSE)
    {
        if(vterm->mouse_driver != NULL)
        {
            bytes = vterm->mouse_driver(vterm, buf);
        }
    }

    // if bytes is > 0 we've arleady found something to write
    if(bytes > 0)
    {
        retval = _vterm_write_pty(vterm, buf, bytes);
        return retval;
    }

    // if bytes == 0 then no special handling, just pass the key through
    bytes = sizeof(char);
    memcpy(buf, &keycode, bytes);
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
        fprintf(stderr, "WARNING: Failed to write buffer to pty\n");
        retval = -1;
    }

    return retval;
}
