#include "macros.h"
#include "vtty_private.h"
#include "vtty_escape.h"
#include "vtty_csi.h"

// returns number of bytes processed (should always be 0 or 1)

int
vtty_interpret_escapes_simple(vtty_t *vtty, char verb)
{
    static void     *simple_table[] =
                        {
                            [0] = &&simple_char_default,
                            ['E'] = &&esc_NEL,
                            ['M'] = &&esc_RI,
                            ['A'] = &&simple_char_A,
                            ['B'] = &&simple_char_B,
                            ['C'] = &&simple_char_C,
                            ['D'] = &&simple_IND,
                            ['7'] = &&simple_char_vii,
                            ['8'] = &&simple_char_viii,
                            ['c'] = &&simple_char_c,
                            // C1 Control Characters
                            [0x84] = &&simple_IND,
                            [0x85] = &&esc_NEL,
                            [0x8d] = &&esc_RI,
                        };


    SWITCH(simple_table, (unsigned char)verb, 0);

    // interpert ESC-H a line-feed (NEL)
    esc_NEL:
        interpret_esc_NEL(vtty);
        vtty_escape_cancel(vtty);
        return 1;

    // interpret ESC-M as reverse line-feed (RI)
    esc_RI:
        interpret_esc_RI(vtty);
        vtty_escape_cancel(vtty);
        return 1;

    // VT52, move cursor up.  same as CUU which is ESC [ A
    simple_char_A:
        interpret_csi_CUx(vtty, 'A', (int *)NULL, 0);
        vtty_escape_cancel(vtty);
        return 1;

    // VT52, move cursor down.  same as CUD which is ESC [ B
    simple_char_B:
        interpret_csi_CUx(vtty, 'B', (int *)NULL, 0);
        vtty_escape_cancel(vtty);
        return 1;

    // VT52, move cursor right.  same as CUF which is ESC [ C
    simple_char_C:
        interpret_csi_CUx(vtty, 'C', (int *)NULL, 0);
        vtty_escape_cancel(vtty);
        return 1;

    // VT52, move cursor down.  same as ESC [ e
    simple_IND:
        // interpret_csi_CUx(vtty, 'e', (int *)NULL, 0);
        interpret_esc_IND(vtty);
        vtty_escape_cancel(vtty);
        return 1;

    simple_char_vii:
        interpret_csi_SAVECUR(vtty, 0, 0);
        vtty_escape_cancel(vtty);
        return 1;

    simple_char_viii:
        interpret_csi_RESTORECUR(vtty, 0, 0);
        vtty_escape_cancel(vtty);
        return 1;

    // The ESC c sequence is RS1 reset for most
    simple_char_c:
        // push in "\ec" as a safety check
        interpret_csi_RS1_xterm(vtty, XTERM_RS1);
        vtty_escape_cancel(vtty);
        return 1;

    // unhandled sequence
    simple_char_default:

    // zero means no sequences processed
    return 0;
}
