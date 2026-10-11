#include <string.h>

#include "vtty_private.h"
#include "mouse_driver.h"
#include "stringv.h"

/*
    CSI < Ps ; Ps ; Ps M   <- mouse button down or move event

    CSI < Ps ; Ps ; Ps m   <- mouse button release event
*/
ssize_t
mouse_driver_SGR(vtty_t *vtty, unsigned char *buf)
{
    MEVENT          mouse_event;
    unsigned int    button = 0;
    int             retval = 0;
    int             x, y;

    if(vtty->pending_mouse != NULL)
        memcpy(&mouse_event, vtty->pending_mouse, sizeof(MEVENT));
    else
        getmouse(&mouse_event);
    x = mouse_event.x;
    y = mouse_event.y;

    if(vtty->window != NULL)
    {
        // see notes above on coordinate transformations
        if(wmouse_trafo(vtty->window, &y, &x, FALSE) == FALSE)
        {
            return 0;
        }

        // ncurses top left is is 0,0 whereas for X10 it is 1,1
        x++;
        y++;
    }

    if(mouse_event.bstate & BUTTON_SHIFT) button |= 4;
    if(mouse_event.bstate & BUTTON_CTRL) button |= 8;
    if(mouse_event.bstate & BUTTON_ALT) button |= 16;

    if(mouse_event.bstate & BUTTON1_PRESSED)
    {
        sprintf((char *)buf, "\033[<%d;%d;%dM", 0, x, y);

        retval = strlen((char *)buf);

        return retval;
    }

    if(mouse_event.bstate & BUTTON1_RELEASED)
    {
        sprintf((char *)buf, "\033[<%d;%d;%dm", 0, x, y);

        retval = strlen((char *)buf);

        return retval;
    }

// only the newer ABI supports the wheel mouse properly
#if NCURSES_MOUSE_VERSION > 1

    if(mouse_event.bstate & BUTTON4_PRESSED)
    {
        if(vtty->mouse_mode & VTTY_MOUSE_ALTSCROLL)
            sprintf((char *)buf, "\033OA");
        else
            // SGR wheel-up is button 64 (+ modifier bits)
            sprintf((char *)buf, "\033[<%d;%d;%dM", button + 64, x, y);

        retval = strlen((char *)buf);

        return retval;
    }

    if(mouse_event.bstate & BUTTON5_PRESSED)
    {
        if(vtty->mouse_mode & VTTY_MOUSE_ALTSCROLL)
            sprintf((char *)buf, "\033OB");
        else
            // SGR wheel-down is button 65 (+ modifier bits)
            sprintf((char *)buf, "\033[<%d;%d;%dM", button + 65, x, y);

        retval = strlen((char *)buf);

        return retval;
    }

#endif

    return 0;
}

