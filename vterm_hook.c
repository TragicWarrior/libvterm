
#include "vterm.h"
#include "vterm_private.h"

void
vterm_install_hook(vterm_t *vterm, VtermEventHook hook)
{
    if(vterm == NULL) return;

    vterm->event_hook = hook;

    return;
}
