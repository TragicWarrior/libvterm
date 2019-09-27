#include "macros.h"
#include "vterm_csi.h"
#include "vterm_private.h"

void
interpret_csi_IRM(vterm_t *vterm, bool replace_mode)
{
    VAR_UNUSED(replace_mode);     // silence complier warning

    if(vterm == NULL) return;

    // todo:    this is a temporary hack to make sure replace mode does
    //          not get turned on until the real effects of IRM mode on
    //          other control is well understood.
    vterm->internal_state &= ~STATE_REPLACE_MODE;

    /*

    if(replace_mode == TRUE)
        vterm->internal_state |= STATE_REPLACE_MODE;
    else
        vterm->internal_state &= ~STATE_REPLACE_MODE;

    */

    return;
}
