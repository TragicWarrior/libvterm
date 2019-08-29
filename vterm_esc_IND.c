
#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_misc.h"
#include "vterm_buffer.h"

/*
    interprets a 'move cursor down' (IND) escape sequence.
*/
inline void
interpret_esc_IND(vterm_t *vterm)
{
    vterm_desc_t    *v_desc = NULL;
    int             idx;

    // set active vterm description selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    vterm_scroll_up(vterm, FALSE);

    return;
}

