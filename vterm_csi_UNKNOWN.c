#include "macros.h"
#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"

// catch all for unknown escape sequences
void
interpret_csi_UNKNOWN(vterm_t *vterm, int param[], int pcount)
{
    // STUB function

    VAR_UNUSED(vterm);
    VAR_UNUSED(param);
    VAR_UNUSED(pcount);

    return;
}

