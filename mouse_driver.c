#include <string.h>

#include "vterm_private.h"
#include "mouse_driver.h"
#include "stringv.h"

void
mouse_driver_init(void)
{
    mmask_t     mouse_mask = ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION;

    mousemask(mouse_mask, NULL);

    return;
}

ssize_t
mouse_driver_default(vterm_t *vterm, unsigned char *buf)
{
    ssize_t     mbytes = 0;

    if(vterm->mouse & VTERM_MOUSE_VT200)
    {
        vterm->mouse_driver = mouse_driver_vt200;
    }

    if(vterm->mouse & VTERM_MOUSE_SGR)
    {
        vterm->mouse_driver = mouse_driver_SGR;
    }

    mbytes = vterm->mouse_driver(vterm, buf);

    return mbytes;
}

ssize_t
mouse_driver_vt200(vterm_t *vterm, unsigned char *buf)
{
    MEVENT      mouse_event;
    int         b_event = 0;

    getmouse(&mouse_event);

    if(mouse_event.bstate & BUTTON1_CLICKED)
    {
        sprintf((char *)buf, "\e[M%c%c%c\e[M%c%c%c",
            32 + 0,
            32 + mouse_event.x,
            32 + mouse_event.y,
            32 + 0x3,
            32 + mouse_event.x,
            32 + mouse_event.y);

        return 12;
    }

// only the newer ABI supports the wheel mous properly
#if NCURSES_MOUSE_VERSION > 1

    if(mouse_event.bstate & BUTTON4_PRESSED)
    {
        sprintf((char *)buf, "\eOA");

        return 3;
    }

    if(mouse_event.bstate & BUTTON5_PRESSED)
    {
        sprintf((char *)buf, "\eOB");

        return 3;
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
    MEVENT  mouse_event;

    getmouse(&mouse_event);

    if(mouse_event.bstate & BUTTON1_CLICKED)
    {
        sprintf((char *)buf, "\e[<%d;%d;%dM\e[<%d;%d;%dm",
            32 + 0,
            mouse_event.x,
            mouse_event.y,
            32 + 0,
            mouse_event.x,
            mouse_event.y);

        return strlen((char *)buf);
    }

// only the newer ABI supports the wheel mous properly
#if NCURSES_MOUSE_VERSION > 1

    if(mouse_event.bstate & BUTTON4_PRESSED)
    {
        sprintf((char *)buf, "\eOA");

        return 3;
    }

    if(mouse_event.bstate & BUTTON5_PRESSED)
    {
        sprintf((char *)buf, "\eOB");

        return 3;
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
