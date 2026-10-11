#include "macros.h"
#include "vtty.h"
#include "vtty_private.h"
#include "vtty_csi.h"

// catch all for unknown escape sequences
void
interpret_csi_UNKNOWN(vtty_t *vtty, int param[], int pcount)
{
    // STUB function

    VAR_UNUSED(vtty);
    VAR_UNUSED(param);
    VAR_UNUSED(pcount);

    return;
}

