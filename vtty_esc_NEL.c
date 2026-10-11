
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
    ESC - E (Next Line)

    This sequence causes the active position to move to the first position
    on the next line downward. If the active position is at the
    bottom margin, a scroll up is performed.
*/

inline void
interpret_esc_NEL(vtty_t *vtty)
{
    vtty_desc_t     *v_desc = NULL;

    // set vtty desc buffer selector
    v_desc = vtty->v_desc_active;

    // move to column 1;
    v_desc->ccol = 0;

    // check to see if we're at the bottom already
    if(v_desc->crow >= v_desc->scroll_max)
    {
        vtty_scroll_up(vtty, TRUE);
        return;
    }

    v_desc->crow++;

    return;
}
