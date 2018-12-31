
#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"

// catch all for unknown escape sequences
void
interpret_csi_UNKNOWN(vterm_t *vterm, int param[], int pcount)
{
    // STUB function

    (void)vterm;
    (void)param;
    (void)pcount;

    return;
}

