#include "macros.h"
#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_buffer.h"
#include "vterm_cursor.h"

void
interpret_csi_SAVECUR(vterm_t *vterm, int param[], int pcount)
{
    vterm_desc_t    *v_desc = NULL;

    VAR_UNUSED(param);    //make compiler happy
    VAR_UNUSED(pcount);

    // set vterm description buffer selector
    v_desc = vterm->v_desc_active;

    v_desc->saved_x = v_desc->ccol;
    v_desc->saved_y = v_desc->crow;

    return;
}

