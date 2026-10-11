#include "macros.h"
#include "vtty.h"
#include "vtty_private.h"
#include "vtty_csi.h"
#include "vtty_buffer.h"
#include "vtty_cursor.h"

void
interpret_csi_SAVECUR(vtty_t *vtty, int param[], int pcount)
{
    vtty_desc_t     *v_desc = NULL;

    VAR_UNUSED(param);    //make compiler happy
    VAR_UNUSED(pcount);

    // set vtty description buffer selector
    v_desc = vtty->v_desc_active;

    v_desc->saved_x = v_desc->ccol;
    v_desc->saved_y = v_desc->crow;

    return;
}

