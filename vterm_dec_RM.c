
#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_buffer.h"

/* Interpret the DEC RM (reset mode) */
void
interpret_dec_RM(vterm_t *vterm, int param[], int pcount)
{
    vterm_desc_t    *v_desc = NULL;
    int             i;
    int             idx;

    if(pcount == 0) return;

    // set vterm desc buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    for(i = 0; i < pcount; i++)
    {
        /* civis is actually the "normal" vibility for rxvt   */
        if(param[i] == 25) v_desc->buffer_state |= STATE_CURSOR_INVIS;

        // restore standard buffer
        // CSI hanlder:  ESC [ ? 47 l
        if(param[i] == 47)
        {
            // check to see if we're already using the ALT buffer
            if(idx == VTERM_BUFFER_STD) continue;

            vterm_buffer_set_active(vterm, VTERM_BUFFER_STD);
            continue;
        }
    }
}
