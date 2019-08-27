#include <string.h>

#include "vterm_private.h"
#include "mouse_driver.h"
#include "stringv.h"

/*
    CSI < Ps ; Ps ; Ps M   <- mouse button down or move event

    CSI < Ps ; Ps ; Ps m   <- mouse button release event
*/
ssize_t
mouse_driver_SGR(vterm_t *vterm, unsigned char *buf)
{
    MEVENT          mouse_event;
    unsigned int    button = 0;
    int             retval = 0;
    int             x, y;

    getmouse(&mouse_event);
    x = mouse_event.x;
    y = mouse_event.y;

    if(vterm->window != NULL)
    {
        // see notes above on coordinate transformations
        if(wmouse_trafo(vterm->window, &y, &x, FALSE) == FALSE)
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
        if(vterm->mouse_mode & VTERM_MOUSE_ALTSCROLL)
            sprintf((char *)buf, "\033OA");
        else
            sprintf((char *)buf, "\033[<%d;%d;%dM", button + 64 + 4, x, y);

        retval = strlen((char *)buf);

        return retval;
    }

    if(mouse_event.bstate & BUTTON5_PRESSED)
    {
        if(vterm->mouse_mode & VTERM_MOUSE_ALTSCROLL)
            sprintf((char *)buf, "\033OB");
        else
            sprintf((char *)buf, "\033[<%d;%d;%dM", button + 64 + 5, x, y);

        retval = strlen((char *)buf);

        return retval;
    }

#endif

    return 0;
}

