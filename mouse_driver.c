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

    getmouse(&mouse_event);

    // endwin(); exit(0);

    // printf("x: %d, y: %d\n\r", mouse_event.x, mouse_event.y);

/*
    if(mouse_event.bstate & BUTTON1_CLICKED)
    {
        buf[0] = '\033';
        buf[1] = '[';
        buf[2] = 'M';
        buf[3] = ' ' + 0;
        buf[4] = ' ' + 6;
        buf[5] = ' ' + 6;
        buf[6] = '\033';
        buf[7] = '[';
        buf[8] = 'M';
        buf[9] = ' ' + 0x40;
        buf[10] = ' ' + 6;
        buf[11] = ' ' + 6;

        return 12;
    }
*/


    if(mouse_event.bstate & BUTTON1_CLICKED)
    {
        buf[0] = '\033';
        buf[1] = '[';
        buf[2] = 'M';
        buf[3] = ' ' + 0;
        buf[4] = ' ' + mouse_event.x;
        buf[5] = ' ' + mouse_event.y;
        buf[6] = '\033';
        buf[7] = '[';
        buf[8] = 'M';
        buf[9] = ' ' + 0x3;
        buf[10] = ' ' + mouse_event.x;
        buf[11] = ' ' + mouse_event.y;

        return 12;
    }

/*
    if(mouse_event.bstate & BUTTON1_PRESSED)
    {
        buffer = strdup_printf("\e[M%c%c%c",
            (char)(' ' + 0 + 0),
            (char)(' ' + mouse_event.x),
            (char)(' ' + mouse_event.y));

        return buffer;
    }

    if(mouse_event.bstate & BUTTON1_RELEASED)
    {
        buffer = strdup_printf("\e[M%c%c%c",
            (char)(' ' + 0 + 0x40),
            (char)(' ' + mouse_event.x),
            (char)(' ' + mouse_event.y));

        return buffer;
    }
*/

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

/*
    if(mouse_event.bstate & BUTTON1_CLICKED)
    {
        buffer = strdup_printf("\e[<%c;%c;%cM\e[<%c;%c;%cm",
            (char)(1),
            (char)(mouse_event.x),
            (char)(mouse_event.y),
            (char)(1),
            (char)(mouse_event.x),
            (char)(mouse_event.y));
    }
*/

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
