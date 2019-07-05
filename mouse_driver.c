#include "vterm_private.h"
#include "stringv.h"

char *
mouse_driver_vt200(vterm_t *vterm)
{
    MEVENT  mouse_event;
    char    *buffer = NULL;

    if(vterm->mouse == VTERM_MOUSE_VT200)
    {
        getmouse(&mouse_event);

        if(mouse_event.bstate & BUTTON1_CLICKED)
        {
            buffer = strdup_printf("\e[M%c%c%c\e[M%c%c%c",
                (char)(32 + 0 + 0),
                (char)(32 + mouse_event.x),
                (char)(32 + mouse_event.y),
                (char)(32 + 0 + 0x40),
                (char)(32 + mouse_event.x),
                (char)(32 + mouse_event.y));
        }
    }

    return buffer;
}

char *
mouse_driver_SGR(vterm_t *vterm)
{
    MEVENT  mouse_event;
    char    *buffer = NULL;

    if(vterm->mouse & VTERM_MOUSE_SGR)
    {
        getmouse(&mouse_event);

        if(mouse_event.bstate & BUTTON1_CLICKED)
        {
            buffer = strdup_printf("\e[<%c;%c;%cM\e[<%c;%c;%cm",
                (char)(0),
                (char)(mouse_event.x),
                (char)(mouse_event.y),
                (char)(0),
                (char)(mouse_event.x),
                (char)(mouse_event.y));
        }
    }

    return buffer;
}
