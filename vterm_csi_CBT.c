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

    if(vterm == NULL) return;

    if(pcount > 0) tab_count = param[0];

    // set the vterm description buffer selector
    v_desc = vterm->v_desc_active;

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
