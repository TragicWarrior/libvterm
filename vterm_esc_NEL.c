
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
    ESC - E (Next Line)

    This sequence causes the active position to move to the first position
    on the next line downward. If the active position is at the
    bottom margin, a scroll up is performed.
*/

inline void
interpret_esc_NEL(vterm_t *vterm)
{
    vterm_desc_t    *v_desc = NULL;
    int             idx;

    // set vterm desc buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    // move to column 1;
    v_desc->ccol = 0;

    // check to see if we're at the bottom already
    if(v_desc->crow >= v_desc->scroll_max)
    {
        vterm_scroll_up(vterm, TRUE);
        return;
    }

    v_desc->crow++;

    return;
}
