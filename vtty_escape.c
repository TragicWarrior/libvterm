#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "vtty.h"
#include "vtty_private.h"
#include "vtty_escape.h"
#include "vtty_escape_suffixes.h"
#include "vtty_csi.h"
#include "vtty_osc.h"
#include "vtty_osc_DA.h"
#include "vtty_render.h"
#include "vtty_misc.h"
#include "vtty_buffer.h"
#include "macros.h"


static int
vtty_interpret_esc_xterm_dsc(vtty_t *vtty);

static int
vtty_interpret_esc_scs(vtty_t *vtty);

static int
vtty_interpret_dec_private(vtty_t *vtty);

static bool
check_suffix_csi(vtty_t *vtty)
{
    return validate_csi_escape_suffix(&vtty->esbuf[vtty->esbuf_len - 1]);
}

static bool
check_suffix_osc(vtty_t *vtty)
{
    char *lastchar = &vtty->esbuf[vtty->esbuf_len - 1];

    if(vtty->flags & VTTY_FLAG_LINUX)
    {
        if(*lastchar == 'R' && *(lastchar - 1) == ']')
        {
            vtty_escape_cancel(vtty);
            return false;
        }
    }

    /*
        the bare C1 String Terminator (0x9C) is also a UTF-8 continuation
        byte, so in UTF-8 mode an OSC payload carrying a multibyte glyph --
        e.g. a window title beginning with U+2733 (E2 9C B3) -- would
        terminate mid-character and spill the remainder onto the screen at
        the cursor.  honor 0x9C as ST only in 8-bit mode; UTF-8 apps close
        OSC with BEL (0x07) or ESC backslash.
    */
    if((unsigned char)*lastchar == 0x9c && !(vtty->flags & VTTY_FLAG_NOUTF8))
        return FALSE;

    return validate_xterm_escape_suffix(lastchar);
}

static bool
check_suffix_scs(vtty_t *vtty)
{
    return validate_scs_escape_suffix(&vtty->esbuf[vtty->esbuf_len - 1]);
}

static bool
check_suffix_dcs(vtty_t *vtty)
{
    return (vtty->esbuf_len > 2
        && vtty->esbuf[vtty->esbuf_len - 2] == '\x1B'
        && vtty->esbuf[vtty->esbuf_len - 1] == '\\');
}

static bool
check_suffix_dec(vtty_t *vtty)
{
    return validate_dec_private_suffix(&vtty->esbuf[vtty->esbuf_len - 1]);
}

void
vtty_esbuf_init(vtty_t *vtty)
{
    if(vtty == NULL) return;

    vtty->esbuf = vtty->esbuf_small;
    vtty->esbuf_cap = ESEQ_BUF_SIZE;
    vtty->esbuf_len = 0;
    vtty->esbuf_small[0] = '\0';

    return;
}

void
vtty_esbuf_reset(vtty_t *vtty)
{
    if(vtty == NULL) return;

    if(vtty->esbuf != NULL && vtty->esbuf != vtty->esbuf_small)
        free(vtty->esbuf);

    vtty_esbuf_init(vtty);

    return;
}

static int
vtty_esbuf_grow(vtty_t *vtty)
{
    char    *neu;
    int     cap;

    if(!IS_OSC_MODE(vtty)) return -1;

    cap = vtty->esbuf_cap * 2;
    if(cap < ESEQ_BUF_SIZE * 2) cap = ESEQ_BUF_SIZE * 2;
    if(cap > ESEQ_BUF_OSC_MAX) cap = ESEQ_BUF_OSC_MAX;
    if(vtty->esbuf_len >= cap) return -1;

    neu = (char *)malloc((size_t)cap + 1);
    if(neu == NULL) return -1;

    memcpy(neu, vtty->esbuf, (size_t)vtty->esbuf_len + 1);

    if(vtty->esbuf != vtty->esbuf_small)
        free(vtty->esbuf);

    vtty->esbuf = neu;
    vtty->esbuf_cap = cap;

    return 0;
}

int
vtty_esbuf_putc(vtty_t *vtty, char c)
{
    if(vtty == NULL) return -1;

    if(vtty->esbuf == NULL)
        vtty_esbuf_init(vtty);

    if(vtty->esbuf_len >= vtty->esbuf_cap)
    {
        if(vtty_esbuf_grow(vtty) == -1)
            return -1;
    }

    vtty->esbuf[vtty->esbuf_len] = c;
    vtty->esbuf_len++;
    vtty->esbuf[vtty->esbuf_len] = '\0';

    return 0;
}

void
vtty_escape_start(vtty_t *vtty)
{
    vtty->internal_state |= STATE_ESCAPE_MODE;

    vtty_esbuf_reset(vtty);

    vtty->esc_handler = NULL;
    vtty->esc_suffix_check = NULL;

    return;
}

void
vtty_escape_cancel(vtty_t *vtty)
{
    vtty->internal_state &= ~STATE_ESCAPE_MODE;

    // make sure OSC mode is off too
    vtty->internal_state &= ~STATE_OSC_MODE;

    vtty_esbuf_reset(vtty);

    vtty->esc_handler = NULL;
    vtty->esc_suffix_check = NULL;

    return;
}

void
vtty_interpret_escapes(vtty_t *vtty)
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


    firstchar = vtty->esbuf[0];

    // too early to do anything
    if(!firstchar) return;

    retval = vtty_interpret_escapes_simple(vtty, firstchar);
    if(retval > 0) return;

    SWITCH(interim_table, (unsigned char)firstchar, 0);

    interim_OSC:
        vtty->internal_state |= STATE_OSC_MODE;
        vtty->esc_handler = vtty_interpret_xterm_osc;
        vtty->esc_suffix_check = check_suffix_osc;
        return;

    interim_CSI:
        vtty->esc_handler = vtty_interpret_csi;
        vtty->esc_suffix_check = check_suffix_csi;
        return;

    interim_lparth:
        vtty->esc_handler = vtty_interpret_esc_scs;
        vtty->esc_suffix_check = check_suffix_scs;
        return;

    interim_rparth:
        vtty->esc_handler = vtty_interpret_esc_scs;
        vtty->esc_suffix_check = check_suffix_scs;
        return;

    interim_char_P:
        vtty->esc_handler = vtty_interpret_esc_xterm_dsc;
        vtty->esc_suffix_check = check_suffix_dcs;
        return;

    interim_pound:
        vtty->esc_handler = vtty_interpret_dec_private;
        vtty->esc_suffix_check = check_suffix_dec;
        return;

    interim_char_none:
        vtty_escape_cancel(vtty);
        return;
}

int
vtty_interpret_esc_xterm_dsc(vtty_t *vtty)
{
    /*
        TODO

        accept DSC (ESC-P) sequences from xterm but don't do anything
        with them - just toss them to /dev/null for now.
    */

    const char  *p;

    p = vtty->esbuf + 1;

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
vtty_interpret_esc_scs(vtty_t *vtty)
{
    const char  *p;

    p = vtty->esbuf;

    // not the most elegant way to handle these.  todo: improve later.
    if(*p == '(' && p[1] == '0')
    {
        vtty->internal_state |= STATE_ALT_CHARSET;
    }

    if(*p == '(' && p[1] == 'B')
    {
        vtty->internal_state &= ~STATE_ALT_CHARSET;
    }

    // G1 sequence - unused
    if(*p == ')') {}

    p++;
    // could do something with the codes

    // return the number of bytes handled
    return 2;
}

static int
vtty_interpret_dec_private(vtty_t *vtty)
{
    const char  *p;

    p = vtty->esbuf;
    p++;

    if(*p == '\0') return 0;

    if(*p == '8')
    {
        interpret_dec_DECALN(vtty);
        return 1;
    }

    return 0;
}

