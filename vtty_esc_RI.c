
#include <string.h>
#include <fcntl.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "vtty.h"
#include "vtty_private.h"
#include "vtty_csi.h"
#include "vtty_buffer.h"

/*
    ESC - M (Reverse Index, RI)

    Move the active position to the same horizontal position on the
    preceding line. If the active position is at the top margin, a
    scroll down is performed.
*/

inline void
interpret_esc_RI(vtty_t *vtty)
{
    vtty_desc_t     *v_desc = NULL;

    // set vtty desc buffer selector
    v_desc = vtty->v_desc_active;

    // check to see if we're at the top already
    if(v_desc->crow <= v_desc->scroll_min)
    {
        /*
            setting reset_colors param to TRUE makes bastet happy but
            breaks pspg when scrolling.
        */
        vtty_scroll_down(vtty, TRUE);
        return;
    }

    v_desc->crow--;

    return;
}
