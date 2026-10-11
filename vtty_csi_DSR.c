#include <stdio.h>
#include <string.h>

#include "vtty.h"
#include "vtty_private.h"
#include "vtty_csi.h"
#include "vtty_reply.h"

/*
    Interpret a Device Status Report (DSR) request and reply on the pty:

        CSI 5 n  ->  "\033[0n"            device OK
        CSI 6 n  ->  "\033[<row>;<col>R"  Cursor Position Report (1-based)

    Apps built on termenv / Charm (e.g. glow --tui) pair every OSC color
    query with a CSI 6 n and read until the CPR reply arrives -- the CPR is
    their terminator.  Without it they stall during capability detection
    and never enter the TUI.  Only the standard (non-DEC) form is answered;
    the DEC variant (CSI ? 6 n, DECXCPR) is intentionally left alone.
*/
void
interpret_csi_DSR(vtty_t *vtty, int param[], int pcount)
{
    vtty_desc_t     *v_desc;
    int             verb;

    verb = (pcount == 0) ? 0 : param[0];

    if(verb == 6)
    {
        // CPR -- report the active buffer's cursor, converted to 1-based
        v_desc = vtty->v_desc_active;

        memset(vtty->reply_buf, 0, sizeof(vtty->reply_buf));
        vtty->reply_buf_sz = snprintf(vtty->reply_buf,
            sizeof(vtty->reply_buf), "\033[%d;%dR",
            v_desc->crow + 1, v_desc->ccol + 1);

        vtty_reply_buffer(vtty);
    }
    else if(verb == 5)
    {
        // device status -- always report OK
        memset(vtty->reply_buf, 0, sizeof(vtty->reply_buf));
        vtty->reply_buf_sz = snprintf(vtty->reply_buf,
            sizeof(vtty->reply_buf), "\033[0n");

        vtty_reply_buffer(vtty);
    }

    return;
}
