#include <string.h>

#include "vterm_private.h"
#include "mouse_driver.h"
#include "stringv.h"

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
        sprintf((char *)buf, "\033[M%c%c%c",
            32 + 0x0,
            32 + x,
            32 + y);

        retval = strlen((char *)buf);

        return retval;
    }

    if(mouse_event.bstate & BUTTON1_RELEASED)
    {
        sprintf((char *)buf, "\033[M%c%c%c",
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
        sprintf((char *)buf, "\033OA");

        retval = strlen((char *)buf);

        return retval;
    }

    if(mouse_event.bstate & BUTTON5_PRESSED)
    {
        sprintf((char *)buf, "\033OB");

        retval = strlen((char *)buf);

        return retval;
    }

#endif

    return 0;
}

