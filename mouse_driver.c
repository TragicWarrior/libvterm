#include <string.h>

#include "vterm_private.h"
#include "mouse_driver.h"
#include "stringv.h"

void
mouse_driver_init(void)
{
    mmask_t     mouse_mask = ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION;

    mousemask(mouse_mask, NULL);
    mouseinterval(0);

    return;
}

ssize_t
mouse_driver_default(vterm_t *vterm, unsigned char *buf)
{
    ssize_t     mbytes = 0;

    // SGR is a special variant of VT200
    if(vterm->mouse & VTERM_MOUSE_SGR)
    {
        mbytes = mouse_driver_SGR(vterm, buf);
        return mbytes;
    }

    if(vterm->mouse & VTERM_MOUSE_VT200)
    {
        mbytes = mouse_driver_vt200(vterm, buf);
        return mbytes;
    }


    return 0;
}

ssize_t
mouse_driver_vt200(vterm_t *vterm, unsigned char *buf)
{
    MEVENT      mouse_event;
    int         retval = 0;
    int         x, y;

    getmouse(&mouse_event);
    x = mouse_event.x;
    y = mouse_event.y;

    if(vterm->window != NULL)
    {
        /*
            get mouse returns screen relative coords and we need to transform
            those into window relative coords.  wmouse_trafo() does exactly
            that and returns false if the conversion is not possilbe--for
            exmpale if the coord are out of bounds.
        */
        if(wmouse_trafo(vterm->window, &y, &x, FALSE) == FALSE)
        {
            return 0;
        }

        // ncurses top left is is 0,0 whereas for X10 it is 1,1
        x++;
        y++;
    }

    if(mouse_event.bstate & BUTTON1_PRESSED)
    {
        sprintf((char *)buf, "\e[M%c%c%c",
            32 + 0x0,
            32 + x,
            32 + y);

        retval = strlen((char *)buf);

        return retval;
    }

    if(mouse_event.bstate & BUTTON1_RELEASED)
    {
        sprintf((char *)buf, "\e[M%c%c%c",
            32 + 0x3,
            32 + x,
            32 + y);

        retval = strlen((char *)buf);

        return retval;
    }

// only the newer ABI supports the wheel mous properly
#if NCURSES_MOUSE_VERSION > 1

    if(mouse_event.bstate & BUTTON4_PRESSED)
    {
        sprintf((char *)buf, "\eOA");

        retval = strlen((char *)buf);

        return retval;
    }

    if(mouse_event.bstate & BUTTON5_PRESSED)
    {
        sprintf((char *)buf, "\eOB");

        retval = strlen((char *)buf);

        return retval;
    }

#endif

    return 0;
}

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
        sprintf((char *)buf, "\e[<%d;%d;%dM", 0, x, y);

        retval = strlen((char *)buf);

        return retval;
    }

    if(mouse_event.bstate & BUTTON1_RELEASED)
    {
        sprintf((char *)buf, "\e[<%d;%d;%dm", 0, x, y);

        retval = strlen((char *)buf);

        return retval;
    }

// only the newer ABI supports the wheel mous properly
#if NCURSES_MOUSE_VERSION > 1

    if(mouse_event.bstate & BUTTON4_PRESSED)
    {
        if(vterm->mouse & VTERM_MOUSE_ALTSCROLL)
            sprintf((char *)buf, "\eOA");
        else
            sprintf((char *)buf, "\e[<%d;%d;%dM", button + 64 + 4, x, y);

        retval = strlen((char *)buf);

        return retval;
    }

    if(mouse_event.bstate & BUTTON5_PRESSED)
    {
        if(vterm->mouse & VTERM_MOUSE_ALTSCROLL)
            sprintf((char *)buf, "\eOB");
        else
            sprintf((char *)buf, "\e[<%d;%d;%dM", button + 64 + 5, x, y);

        retval = strlen((char *)buf);

        return retval;
    }

#endif

    return 0;
}

void
mouse_driver_unset(vterm_t *vterm)
{
    vterm->mouse = 0;
    vterm->mouse_driver = NULL;

    mousemask(0, NULL);

    return;
}
