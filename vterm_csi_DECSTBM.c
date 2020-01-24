
#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_buffer.h"


/*
    Interpret a 'set scrolling region' (DECSTBM) sequence
    ESC [ t ; b r

    t = tob row
    b = bottmo row
*/
void
interpret_csi_DECSTBM(vterm_t *vterm,int param[],int pcount)
{
    vterm_desc_t    *v_desc = NULL;
    int             newtop, newbottom;
    int             idx;

    // set the vterm description selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    if(!pcount)
    {
        newtop = 0;
        newbottom = v_desc->rows - 1;
    }
    else if(pcount < 2) return; /* malformed */
    else
    {
        newtop = param[0] - 1;
        newbottom = param[1] - 1;
    }

    /* clamp to bounds */
    if(newtop < 0) newtop = 0;
    if(newtop >= v_desc->rows) newtop = v_desc->rows - 1;
    if(newbottom < 0) newbottom = 0;
    if(newbottom >= v_desc->rows) newbottom = v_desc->rows - 1;

    // not valid
    if(newtop > newbottom) return;

    /* check for range validity */
    if(newtop > newbottom) return;
    v_desc->scroll_min = newtop;
    v_desc->scroll_max = newbottom;

    if(v_desc->scroll_min != 0)
        v_desc->buffer_state |= STATE_SCROLL_SHORT;

    if(v_desc->scroll_max != v_desc->rows - 1)
        v_desc->buffer_state |= STATE_SCROLL_SHORT;

    if((v_desc->scroll_min == 0) && (v_desc->scroll_max == v_desc->rows - 1))
        v_desc->buffer_state &= ~STATE_SCROLL_SHORT;

    return;
}
