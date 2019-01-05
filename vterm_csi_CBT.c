#include "vterm_csi.h"
#include "vterm_private.h"
#include "vterm_buffer.h"

/*
    From Xterm guide

    ESC [ n Z

    Cursor Backward Tabulation n tab stops (default = 1)
*/
void
interpret_csi_CBT(vterm_t *vterm, int param[], int pcount)
{
    vterm_desc_t    *v_desc = NULL;
    int             tab_count = 1;          // default is one
    int             stride;
    int             idx;

    if(vterm == NULL) return;

    if(pcount > 0) tab_count = param[0];

    // set the vterm description buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    stride = v_desc->ccol % 8;

    if(stride == 0) stride = 8;

    do
    {
        v_desc->ccol -= stride;

        tab_count--;
        stride = 8;
    }
    while(tab_count > 0);

    return;
}
