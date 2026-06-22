#include <stdio.h>
#include <string.h>

#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_reply.h"

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
interpret_csi_DSR(vterm_t *vterm, int param[], int pcount)
{
    vterm_desc_t    *v_desc;
    int             verb;

    verb = (pcount == 0) ? 0 : param[0];

    if(verb == 6)
    {
        // CPR -- report the active buffer's cursor, converted to 1-based
        v_desc = vterm->v_desc_active;

        memset(vterm->reply_buf, 0, sizeof(vterm->reply_buf));
        vterm->reply_buf_sz = snprintf(vterm->reply_buf,
            sizeof(vterm->reply_buf), "\033[%d;%dR",
            v_desc->crow + 1, v_desc->ccol + 1);

        vterm_reply_buffer(vterm);
    }
    else if(verb == 5)
    {
        // device status -- always report OK
        memset(vterm->reply_buf, 0, sizeof(vterm->reply_buf));
        vterm->reply_buf_sz = snprintf(vterm->reply_buf,
            sizeof(vterm->reply_buf), "\033[0n");

        vterm_reply_buffer(vterm);
    }

    return;
}
