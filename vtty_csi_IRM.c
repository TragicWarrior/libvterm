#include "macros.h"
#include "vtty_csi.h"
#include "vtty_private.h"

void
interpret_csi_IRM(vtty_t *vtty, bool replace_mode)
{
    VAR_UNUSED(replace_mode);     // silence complier warning

    if(vtty == NULL) return;

    // todo:    this is a temporary hack to make sure replace mode does
    //          not get turned on until the real effects of IRM mode on
    //          other control is well understood.
    vtty->internal_state &= ~STATE_REPLACE_MODE;

    /*

    if(replace_mode == TRUE)
        vtty->internal_state |= STATE_REPLACE_MODE;
    else
        vtty->internal_state &= ~STATE_REPLACE_MODE;

    */

    return;
}
