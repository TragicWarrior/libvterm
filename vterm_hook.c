
#include "vterm.h"
#include "vterm_private.h"

void
vterm_install_hook(vterm_t *vterm, VtermEventHook hook)
{
    if(vterm == NULL) return;

    vterm->event_hook = hook;

    return;
}

uint32_t
vterm_get_event_mask(vterm_t *vterm)
{
    if(vterm == NULL) return 0;

    return vterm->event_mask;
}

void
vterm_set_event_mask(vterm_t *vterm, uint32_t mask)
{
    if(vterm == NULL) return;

    vterm->event_mask = mask;

    return;
}
