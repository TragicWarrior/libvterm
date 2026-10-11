#include <string.h>

#include "vtty_private.h"
#include "mouse_driver.h"
#include "stringv.h"

/*
    CSI M Cb Cx Cy

    each of the three values is sent as a single byte, 32 + value.  a
    release does not say which button:  its button number is always 3.
*/
ssize_t
mouse_driver_vt200(vtty_t *vtty, unsigned char *buf)
{
    mouse_report_t  report;
    int             code;

    if(mouse_driver_decode(vtty, &report) == 0) return 0;

    // the wheel is sent as cursor keys in this mode
    if(report.kind == MOUSE_REPORT_WHEEL_UP)
    {
        sprintf((char *)buf, "\033OA");
        return strlen((char *)buf);
    }

    if(report.kind == MOUSE_REPORT_WHEEL_DOWN)
    {
        sprintf((char *)buf, "\033OB");
        return strlen((char *)buf);
    }

    // one byte per coordinate cannot express a cell beyond 223
    if(report.x > 223 || report.y > 223) return 0;

    code = report.button | report.mods;
    if(report.kind == MOUSE_REPORT_RELEASE) code = 3 | report.mods;
    if(report.kind == MOUSE_REPORT_MOTION) code |= MOUSE_CODE_MOTION;

    buf[0] = '\033';
    buf[1] = '[';
    buf[2] = 'M';
    buf[3] = (unsigned char)(32 + code);
    buf[4] = (unsigned char)(32 + report.x);
    buf[5] = (unsigned char)(32 + report.y);

    return 6;
}
