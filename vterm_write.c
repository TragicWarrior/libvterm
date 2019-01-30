
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>

#include "vterm.h"
#include "vterm_private.h"
#include "vterm_write.h"
#include "strings.h"

int
vterm_write_pipe(vterm_t *vterm, uint32_t keycode)
{
    int     retval;

    if(vterm == NULL) return -1;

    retval = vterm->write(vterm, keycode);

    if(vterm->event_hook != NULL)
    {
        vterm->event_hook(vterm, VTERM_HOOK_PIPE_WRITTEN, NULL);
    }

    return retval;
}

int
vterm_write_rxvt(vterm_t *vterm,uint32_t keycode)
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
        case KEY_MOUSE:     buffer = "\e[M";    break;
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
vterm_write_xterm(vterm_t *vterm, uint32_t keycode)
{
    ssize_t                 bytes_written = 0;
    char                    *buffer = NULL;
    // char                    *mouse = NULL;
    static struct termios   term_state;
    static char             backspace[8] = "\b";
    int                     retval = 0;
    // MEVENT                  mouse_event;

    tcgetattr(vterm->pty_fd,&term_state);

    if(term_state.c_cc[VERASE] != '0')
        sprintf(backspace, "%c", term_state.c_cc[VERASE]);

    switch(keycode)
    {
        case '\n':          buffer = "\r";      break;
        case KEY_UP:        buffer = "\eOA";    break;
        case KEY_DOWN:      buffer = "\eOB";    break;
        case KEY_RIGHT:     buffer = "\eOC";    break;
        case KEY_LEFT:      buffer = "\eOD";    break;
        case KEY_BACKSPACE: buffer = backspace; break;
        case KEY_IC:        buffer = "\e[2~";   break;
        case KEY_DC:        buffer = "\e[3~";   break;
        case KEY_HOME:      buffer = "\eOH";    break;
        case KEY_END:       buffer = "\eOF";    break;
        case KEY_PPAGE:     buffer = "\e[5~";   break;
        case KEY_NPAGE:     buffer = "\e[6~";   break;
        case KEY_SUSPEND:   buffer = "\x1A";    break;      // ctrl-z
        case KEY_F(1):      buffer = "\eOP";    break;
        case KEY_F(2):      buffer = "\eOQ";    break;
        case KEY_F(3):      buffer = "\eOR";    break;
        case KEY_F(4):      buffer = "\eOS";    break;
        case KEY_F(5):      buffer = "\e[15~";  break;
        case KEY_F(6):      buffer = "\e[17~";  break;
        case KEY_F(7):      buffer = "\e[18~";  break;
        case KEY_F(8):      buffer = "\e[19~";  break;
        case KEY_F(9):      buffer = "\e[20~";  break;
        case KEY_F(10):     buffer = "\e[21~";  break;
        case KEY_F(11):     buffer = "\e[23~";  break;
        case KEY_F(12):     buffer = "\e[24~";  break;
        case KEY_MOUSE:     buffer = "\e[M";    break;
    }

    // send a special sequence
    if(buffer != NULL)
    {
        bytes_written = write(vterm->pty_fd, buffer, strlen(buffer));
        if( bytes_written != (ssize_t)strlen(buffer) )
        {
            fprintf(stderr, "WARNING: Failed to write buffer to pty\n");
            retval = -1;
        }

        return retval;
    }

    // default... send keystroke
    bytes_written = write(vterm->pty_fd, &keycode, sizeof(char));
    if( bytes_written != sizeof(char) )
    {
        fprintf(stderr, "WARNING: Failed to write buffer to pty\n");
        retval = -1;
    }

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

