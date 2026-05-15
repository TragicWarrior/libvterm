#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "vterm.h"
#include "vterm_private.h"
#include "vterm_escape.h"
#include "vterm_escape_suffixes.h"
#include "vterm_csi.h"
#include "vterm_osc.h"
#include "vterm_osc_DA.h"
#include "vterm_render.h"
#include "vterm_misc.h"
#include "vterm_buffer.h"
#include "macros.h"


static int
vterm_interpret_esc_xterm_dsc(vterm_t *vterm);

static int
vterm_interpret_esc_scs(vterm_t *vterm);

static int
vterm_interpret_dec_private(vterm_t *vterm);

static bool
check_suffix_csi(vterm_t *vterm)
{
    return validate_csi_escape_suffix(&vterm->esbuf[vterm->esbuf_len - 1]);
}

static bool
check_suffix_osc(vterm_t *vterm)
{
    char *lastchar = &vterm->esbuf[vterm->esbuf_len - 1];

    if(vterm->flags & VTERM_FLAG_LINUX)
    {
        if(*lastchar == 'R' && *(lastchar - 1) == ']')
        {
            vterm_escape_cancel(vterm);
            return false;
        }
    }

    return validate_xterm_escape_suffix(lastchar);
}

static bool
check_suffix_scs(vterm_t *vterm)
{
    return validate_scs_escape_suffix(&vterm->esbuf[vterm->esbuf_len - 1]);
}

static bool
check_suffix_dcs(vterm_t *vterm)
{
    return (vterm->esbuf_len > 2
        && vterm->esbuf[vterm->esbuf_len - 2] == '\x1B'
        && vterm->esbuf[vterm->esbuf_len - 1] == '\\');
}

static bool
check_suffix_dec(vterm_t *vterm)
{
    return validate_dec_private_suffix(&vterm->esbuf[vterm->esbuf_len - 1]);
}

void
vterm_escape_start(vterm_t *vterm)
{
    vterm->internal_state |= STATE_ESCAPE_MODE;

    // zero out the escape buffer just in case
    vterm->esbuf_len = 0;
    vterm->esbuf[0] = '\0';

    vterm->esc_handler = NULL;
    vterm->esc_suffix_check = NULL;

    return;
}

void
vterm_escape_cancel(vterm_t *vterm)
{
    vterm->internal_state &= ~STATE_ESCAPE_MODE;

    // make sure OSC mode is off too
    vterm->internal_state &= ~STATE_OSC_MODE;

    // zero out the escape buffer for the next run
    vterm->esbuf_len = 0;
    vterm->esbuf[0] = '\0';

    vterm->esc_handler = NULL;
    vterm->esc_suffix_check = NULL;

    return;
}

void
vterm_interpret_escapes(vterm_t *vterm)
{
    char                firstchar;

    static void         *interim_table[] =
                            {
                                [0] = &&interim_char_none,
                                [']'] = &&interim_OSC,
                                ['['] = &&interim_CSI,
                                [')'] = &&interim_rparth,
                                ['('] = &&interim_lparth,
                                ['P'] = &&interim_char_P,
                                ['#'] = &&interim_pound,
                                // C1 Control Characters
                                [0x90] = &&interim_char_P,
                                [0x9b] = &&interim_CSI,
                                [0x9d] = &&interim_OSC,
                            };

    int                 retval = 0;


    firstchar = vterm->esbuf[0];

    // too early to do anything
    if(!firstchar) return;

    retval = vterm_interpret_escapes_simple(vterm, firstchar);
    if(retval > 0) return;

    SWITCH(interim_table, (unsigned char)firstchar, 0);

    interim_OSC:
        vterm->internal_state |= STATE_OSC_MODE;
        vterm->esc_handler = vterm_interpret_xterm_osc;
        vterm->esc_suffix_check = check_suffix_osc;
        return;

    interim_CSI:
        vterm->esc_handler = vterm_interpret_csi;
        vterm->esc_suffix_check = check_suffix_csi;
        return;

    interim_lparth:
        vterm->esc_handler = vterm_interpret_esc_scs;
        vterm->esc_suffix_check = check_suffix_scs;
        return;

    interim_rparth:
        vterm->esc_handler = vterm_interpret_esc_scs;
        vterm->esc_suffix_check = check_suffix_scs;
        return;

    interim_char_P:
        vterm->esc_handler = vterm_interpret_esc_xterm_dsc;
        vterm->esc_suffix_check = check_suffix_dcs;
        return;

    interim_pound:
        vterm->esc_handler = vterm_interpret_dec_private;
        vterm->esc_suffix_check = check_suffix_dec;
        return;

    interim_char_none:
        vterm_escape_cancel(vterm);
        return;
}

int
vterm_interpret_esc_xterm_dsc(vterm_t *vterm)
{
    /*
        TODO

        accept DSC (ESC-P) sequences from xterm but don't do anything
        with them - just toss them to /dev/null for now.
    */

    const char  *p;

    p = vterm->esbuf + 1;

    if( *p=='+'
        && *(p+1)=='q'
        && isxdigit( *(p+2) )
        && isxdigit( *(p+3) )
        && isxdigit( *(p+4) )
        && isxdigit( *(p+5) ) )
        {
        /* we have a valid code, and we'll promptly ignore it */
        }

    return 0;
}

static int
vterm_interpret_esc_scs(vterm_t *vterm)
{
    const char  *p;

    p = vterm->esbuf;

    // not the most elegant way to handle these.  todo: improve later.
    if(*p == '(' && p[1] == '0')
    {
        vterm->internal_state |= STATE_ALT_CHARSET;
    }

    if(*p == '(' && p[1] == 'B')
    {
        vterm->internal_state &= ~STATE_ALT_CHARSET;
    }

    // G1 sequence - unused
    if(*p == ')') {}

    p++;
    // could do something with the codes

    // return the number of bytes handled
    return 2;
}

static int
vterm_interpret_dec_private(vterm_t *vterm)
{
    const char  *p;

    p = vterm->esbuf;
    p++;

    if(*p == '\0') return 0;

    if(*p == '8')
    {
        interpret_dec_DECALN(vterm);
        return 1;
    }

    return 0;
}

