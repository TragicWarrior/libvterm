
#include "vtty.h"
#include "vtty_private.h"

void
vtty_install_hook(vtty_t *vtty, VttyEventHook hook)
{
    if(vtty == NULL) return;

    vtty->event_hook = hook;

    return;
}

uint32_t
vtty_get_event_mask(vtty_t *vtty)
{
    if(vtty == NULL) return 0;

    return vtty->event_mask;
}

void
vtty_set_event_mask(vtty_t *vtty, uint32_t mask)
{
    if(vtty == NULL) return;

    vtty->event_mask = mask;

    return;
}
