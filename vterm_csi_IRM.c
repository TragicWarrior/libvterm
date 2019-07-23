#include "vterm_csi.h"
#include "vterm_private.h"

void
interpret_csi_IRM(vterm_t *vterm, bool replace_mode)
{
    if(vterm == NULL) return;

    if(replace_mode == TRUE)
        vterm->internal_state |= STATE_REPLACE_MODE;
    else
        vterm->internal_state &= ~STATE_REPLACE_MODE;

    return;
}
