#include "macros.h"
#include "vtty.h"
#include "vtty_private.h"
#include "vtty_csi.h"
#include "vtty_buffer.h"
#include "vtty_cursor.h"

void
interpret_csi_RESTORECUR(vtty_t *vtty, int param[], int pcount)
{
    vtty_desc_t     *v_desc = NULL;

    VAR_UNUSED(param);
    VAR_UNUSED(pcount);

    // set the vtty description buffer selector
    v_desc = vtty->v_desc_active;

    v_desc->ccol = v_desc->saved_x;
    v_desc->crow = v_desc->saved_y;

    return;
}

