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


void
mouse_driver_unset(vterm_t *vterm)
{
    vterm->mouse = 0;
    vterm->mouse_driver = NULL;

    mousemask(0, NULL);

    return;
}
