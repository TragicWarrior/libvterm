
#include <string.h>

#include "vterm.h"
#include "vterm_cursor.h"
#include "vterm_csi.h"
#include "vterm_private.h"
#include "vterm_buffer.h"


int
interpret_csi_RS1_rxvt(vterm_t *vterm, char *byte)
{
    static int      end = sizeof(RXVT_RS1) - 2;

    if(byte[0] != RXVT_RS1[vterm->rs1_off])
    {
        vterm->rs1_off = 0;
        return 0;
    }

    if(vterm->rs1_off == end)
    {
        vterm_buffer_set_active(vterm, VTERM_BUF_STANDARD);
        vterm_cursor_show(vterm, VTERM_BUF_STANDARD);

        vterm->rs1_off = 0;

        return 0;
    }

    vterm->rs1_off++;

    return 0;
}

int
interpret_csi_RS1_xterm(vterm_t *vterm, char *data)
{
    if(vterm == NULL) return -1;

    // safety check
    if(strncmp(data, XTERM_RS1, strlen(XTERM_RS1)) != 0) return -1;

    /*
        although the RXVT sequence is a bizarre signal for resetting the
        terminal, the sequence itself is very effective.  push it through
        the renderering engine to affect a reset.
    */
    vterm_render(vterm, RXVT_RS1, sizeof(RXVT_RS1) - 1);

    // reset to standard buffer (and add other stuff if ncessary)
    vterm_buffer_set_active(vterm, VTERM_BUF_STANDARD);

    // make cursor always visible after a reset
    vterm_cursor_show(vterm, VTERM_BUF_STANDARD);

    return 0;
}
