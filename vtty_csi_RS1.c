
#include <string.h>

#include "vtty.h"
#include "vtty_cursor.h"
#include "vtty_csi.h"
#include "vtty_private.h"
#include "vtty_buffer.h"


int
interpret_csi_RS1_rxvt(vtty_t *vtty, char *byte)
{
    static int      end = sizeof(RXVT_RS1) - 2;

    if(byte[0] != RXVT_RS1[vtty->rs1_off])
    {
        vtty->rs1_off = 0;
        return 0;
    }

    if(vtty->rs1_off == end)
    {
        vtty_buffer_set_active(vtty, VTTY_BUF_STANDARD);
        vtty_cursor_show(vtty, VTTY_BUF_STANDARD);

        vtty->rs1_off = 0;

        return 0;
    }

    vtty->rs1_off++;

    return 0;
}

int
interpret_csi_RS1_xterm(vtty_t *vtty, char *data)
{
    if(vtty == NULL) return -1;

    // safety check
    if(strncmp(data, XTERM_RS1, strlen(XTERM_RS1)) != 0) return -1;

    /*
        although the RXVT sequence is a bizarre signal for resetting the
        terminal, the sequence itself is very effective.  push it through
        the renderering engine to affect a reset.
    */
    vtty_render(vtty, RXVT_RS1, sizeof(RXVT_RS1) - 1);

    // reset to standard buffer (and add other stuff if ncessary)
    vtty_buffer_set_active(vtty, VTTY_BUF_STANDARD);

    // make cursor always visible after a reset
    vtty_cursor_show(vtty, VTTY_BUF_STANDARD);

    return 0;
}
