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
    mouse_report_t  report;
    int             code;

    if(mouse_driver_decode(vtty, &report) == 0) return 0;

    // the wheel
    if(report.kind == MOUSE_REPORT_WHEEL_UP
        || report.kind == MOUSE_REPORT_WHEEL_DOWN)
    {
        if(vtty->mouse_mode & VTTY_MOUSE_ALTSCROLL)
        {
            // alternate scroll mode sends cursor keys instead
            if(report.kind == MOUSE_REPORT_WHEEL_UP)
                sprintf((char *)buf, "\033OA");
            else
                sprintf((char *)buf, "\033OB");

            return strlen((char *)buf);
        }

        // SGR wheel-up is button 64 and wheel-down 65 (+ modifier bits)
        code = MOUSE_CODE_WHEEL | report.mods;
        if(report.kind == MOUSE_REPORT_WHEEL_DOWN) code |= 1;

        sprintf((char *)buf, "\033[<%d;%d;%dM", code, report.x, report.y);

        return strlen((char *)buf);
    }

    // buttons and motion:  the button number plus modifier bits
    code = report.button | report.mods;
    if(report.kind == MOUSE_REPORT_MOTION) code |= MOUSE_CODE_MOTION;

    // SGR tells a release apart by the final character alone
    sprintf((char *)buf, "\033[<%d;%d;%d%c", code, report.x, report.y,
        (report.kind == MOUSE_REPORT_RELEASE) ? 'm' : 'M');

    return strlen((char *)buf);
}
