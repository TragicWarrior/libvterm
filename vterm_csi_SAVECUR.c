
#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_buffer.h"

void
interpret_csi_SAVECUR(vterm_t *vterm,int param[],int pcount)
{
    vterm_desc_t    *v_desc = NULL;
    int             idx;

    // set vterm description buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    v_desc->saved_x = v_desc->ccol;
    v_desc->saved_y = v_desc->crow;

    (void)param;
    (void)pcount;

    return;
}

