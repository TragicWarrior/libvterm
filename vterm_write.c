
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
vterm_write_rxvt(vterm_t *vterm, uint32_t keycode)
{
    char                    *buffer = NULL;
    static struct termios   term_state;
    static char             backspace[8] = "\b";
    unsigned char           buf[64];
    int                     bytes = 0;
    int                     retval = 0;

    tcgetattr(vterm->pty_fd, &term_state);

    if(term_state.c_cc[VERASE] != '0')
        sprintf(backspace, "%c", term_state.c_cc[VERASE]);

    switch(keycode)
    {
        case '\n':          buffer = "\r";      break;
        case KEY_UP:        buffer = "\e[A";    break;
        case KEY_DOWN:      buffer = "\e[B";    break;
        case KEY_RIGHT:     buffer = "\e[C";    break;
        case KEY_LEFT:      buffer = "\e[D";    break;
        case KEY_BACKSPACE: buffer = backspace; break;
        case KEY_IC:        buffer = "\e[2~";   break;
        case KEY_DC:        buffer = "\e[3~";   break;
        case KEY_HOME:      buffer = "\e[7~";   break;
        case KEY_END:       buffer = "\e[8~";   break;
        case KEY_PPAGE:     buffer = "\e[5~";   break;
        case KEY_NPAGE:     buffer = "\e[6~";   break;
        case KEY_SUSPEND:   buffer = "\x1A";    break;      // ctrl-z
        case KEY_F1:        buffer = "\e[11~";  break;
        case KEY_F2:        buffer = "\e[12~";  break;
        case KEY_F3:        buffer = "\e[13~";  break;
        case KEY_F4:        buffer = "\e[14~";  break;
        case KEY_F5:        buffer = "\e[15~";  break;
        case KEY_F6:        buffer = "\e[17~";  break;
        case KEY_F7:        buffer = "\e[18~";  break;
        case KEY_F8:        buffer = "\e[19~";  break;
        case KEY_F9:        buffer = "\e[20~";  break;
        case KEY_F10:       buffer = "\e[21~";  break;
        case KEY_F11:       buffer = "\e[23~";  break;
        case KEY_F12:       buffer = "\e[24~";  break;
    }

    if(keycode == KEY_MOUSE)
    {
        if(vterm->mouse_driver != NULL)
        {
            bytes = vterm->mouse_driver(vterm, buf);
        }
    }

    if(buffer != NULL)
    {
        bytes = strlen(buffer);
        memcpy(buf, buffer, bytes);
    }

    if(keycode != KEY_MOUSE && buffer == NULL)
    {
        bytes = sizeof(char);
        memcpy(buf, &keycode, bytes);
    }

    if(bytes > 0)
    {
        retval = _vterm_write_pty(vterm, buf, bytes);
    }

   return retval;


/*
    if(buffer == NULL)
    {
        bytes_written = write(vterm->pty_fd,&keycode,sizeof(char));
        if( bytes_written != sizeof(char) )
        {
            fprintf(stderr, "WARNING: Failed to write buffer to pty\n");
            retval = -1;
        }
    }
    else
    {
        bytes_written = write(vterm->pty_fd,buffer,strlen(buffer));
        if( bytes_written != (ssize_t)strlen(buffer) )
        {
            fprintf(stderr, "WARNING: Failed to write buffer to pty\n");
            retval = -1;
        }
    }

    return reval;
*/

}

int
vterm_write_linux(vterm_t *vterm, uint32_t keycode)
{
    char                    *buffer = NULL;
    static struct termios   term_state;
    static char             backspace[8] = "\b";
    unsigned char           buf[64];
    int                     bytes = 0;
    int                     retval = 0;

    tcgetattr(vterm->pty_fd, &term_state);

    if(term_state.c_cc[VERASE] != '0')
        sprintf(backspace, "%c", term_state.c_cc[VERASE]);

    switch(keycode)
    {
        case '\n':          buffer = "\r";      break;
        case KEY_UP:        buffer = "\e[A";    break;
        case KEY_DOWN:      buffer = "\e[B";    break;
        case KEY_RIGHT:     buffer = "\e[C";    break;
        case KEY_LEFT:      buffer = "\e[D";    break;
        case KEY_BACKSPACE: buffer = backspace; break;
        case KEY_IC:        buffer = "\e[2~";   break;
        case KEY_DC:        buffer = "\e[3~";   break;
        case KEY_HOME:      buffer = "\e[7~";   break;
        case KEY_END:       buffer = "\e[8~";   break;
        case KEY_PPAGE:     buffer = "\e[5~";   break;
        case KEY_NPAGE:     buffer = "\e[6~";   break;
        case KEY_SUSPEND:   buffer = "\x1A";    break;      // ctrl-z
        case KEY_F(1):      buffer = "\e[11~";  break;
        case KEY_F(2):      buffer = "\e[12~";  break;
        case KEY_F(3):      buffer = "\e[13~";  break;
        case KEY_F(4):      buffer = "\e[14~";  break;
        case KEY_F(5):      buffer = "\e[15~";  break;
        case KEY_F(6):      buffer = "\e[17~";  break;
        case KEY_F(7):      buffer = "\e[18~";  break;
        case KEY_F(8):      buffer = "\e[19~";  break;
        case KEY_F(9):      buffer = "\e[20~";  break;
        case KEY_F(10):     buffer = "\e[21~";  break;
        case KEY_F(11):     buffer = "\e[23~";  break;
        case KEY_F(12):     buffer = "\e[24~";  break;
    }

    if(keycode == KEY_MOUSE)
    {
        if(vterm->mouse_driver != NULL)
        {
            bytes = vterm->mouse_driver(vterm, buf);
        }
    }

    if(buffer != NULL)
    {
        bytes = strlen(buffer);
        memcpy(buf, buffer, bytes);
    }

    if(keycode != KEY_MOUSE && buffer == NULL)
    {
        bytes = sizeof(char);
        memcpy(buf, &keycode, bytes);
    }

    if(bytes > 0)
    {
        retval = _vterm_write_pty(vterm, buf, bytes);
    }

   return retval;
}

#define KEYMAP(k, s)    k,
uint32_t xterm_keymap_val[] = {
#include "xterm_keymap.def"
};
#undef KEYMAP

#define KEYMAP(k, s)    s,
char *xterm_keymap_str[] = {
#include "xterm_keymap.def"
};
#undef KEYMAP


int
vterm_write_xterm(vterm_t *vterm, uint32_t keycode)
{
    static struct termios   term_state;
    unsigned char           buf[64];
    ssize_t                 bytes = 0;
    int                     retval = 0;
    static int              array_sz = ARRAY_SZ(xterm_keymap_val);
    int                     i;

    // look in KEYMAP x-macro table for a match
    for(i = 0; i < array_sz; i++)
    {
        // the key keycode is a match
        if(keycode == xterm_keymap_val[i])
        {
            bytes = strlen(xterm_keymap_str[i]);
            strncpy((char *)buf, xterm_keymap_str[i], bytes);
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

    if(bytes > 0)
    {
        retval = _vterm_write_pty(vterm, buf, bytes);
        return retval;
    }

    bytes = sizeof(char);
    memcpy(buf, &keycode, bytes);
    retval = _vterm_write_pty(vterm, buf, bytes);

    return retval;
}


int
vterm_write_vt100(vterm_t *vterm,uint32_t keycode)
{
    ssize_t                 bytes_written = 0;
    char                    *buffer = NULL;
    static struct termios   term_state;
    static char             backspace[8] = "\b";
    int                     retval = 0;

    tcgetattr(vterm->pty_fd,&term_state);

    if(term_state.c_cc[VERASE] != '0')
        sprintf(backspace,"%c",term_state.c_cc[VERASE]);

    switch(keycode)
    {
        case '\n':          buffer = "\r";      break;
        case KEY_UP:        buffer = "\e[A";    break;
        case KEY_DOWN:      buffer = "\e[B";    break;
        case KEY_RIGHT:     buffer = "\e[C";    break;
        case KEY_LEFT:      buffer = "\e[D";    break;
        case KEY_BACKSPACE: buffer = backspace; break;
        case KEY_IC:        buffer = "\e[2~";   break;
        case KEY_DC:        buffer = "\e[3~";   break;
        case KEY_HOME:      buffer = "\e[7~";   break;
        case KEY_END:       buffer = "\e[8~";   break;
        case KEY_PPAGE:     buffer = "\e[5~";   break;
        case KEY_NPAGE:     buffer = "\e[6~";   break;
        case KEY_SUSPEND:   buffer = "\x1A";    break;      // ctrl-z
        case KEY_F(1):      buffer = "\e[[A";   break;
        case KEY_F(2):      buffer = "\e[[B";   break;
        case KEY_F(3):      buffer = "\e[[C";   break;
        case KEY_F(4):      buffer = "\e[[D";   break;
        case KEY_F(5):      buffer = "\e[[E";   break;
        case KEY_F(6):      buffer = "\e[17~";  break;
        case KEY_F(7):      buffer = "\e[18~";  break;
        case KEY_F(8):      buffer = "\e[19~";  break;
        case KEY_F(9):      buffer = "\e[20~";  break;
        case KEY_F(10):     buffer = "\e[21~";  break;
    }

    if(buffer == NULL)
    {
        bytes_written = write(vterm->pty_fd,&keycode,sizeof(char));
        if( bytes_written != sizeof(char) )
        {
            fprintf(stderr, "WARNING: Failed to write buffer to pty\n");
            retval = -1;
        }
    }
    else
    {
        bytes_written = write(vterm->pty_fd,buffer,strlen(buffer));
        if( bytes_written != (ssize_t)strlen(buffer) )
        {
            fprintf(stderr, "WARNING: Failed to write buffer to pty\n");
            retval = -1;
        }
    }

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
