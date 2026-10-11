
#include "vtty.h"
#include "vtty_private.h"
#include "vtty_csi.h"
#include "vtty_misc.h"
#include "vtty_buffer.h"

/*
    interprets a 'move cursor down' (IND) escape sequence.
*/
inline void
interpret_esc_IND(vtty_t *vtty)
{
    vtty_scroll_up(vtty, FALSE);

    return;
}

