
#include <string.h>
#include <fcntl.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_buffer.h"

/*
    ESC - M (Reverse Index, RI)

    Move the active position to the same horizontal position on the
    preceding line. If the active position is at the top margin, a
    scroll down is performed.
*/

inline void
interpret_esc_RI(vterm_t *vterm)
{
    vterm_desc_t    *v_desc = NULL;
    int             idx;

    // set vterm desc buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    // check to see if we're at the top already
    if(v_desc->crow <= v_desc->scroll_min)
    {
        /*
            setting reset_colors param to TRUE makes bastet happy but
            breaks pspg when scrolling.
        */
        vterm_scroll_down(vterm, TRUE);
        return;
    }

    v_desc->crow--;

    return;
}
