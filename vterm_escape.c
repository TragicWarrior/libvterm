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
vterm_interpret_csi(vterm_t *vterm);

static int
vterm_interpret_esc_xterm_dsc(vterm_t *vterm);

static int
vterm_interpret_esc_scs(vterm_t *vterm);


void
vterm_escape_start(vterm_t *vterm)
{
    vterm->internal_state |= STATE_ESCAPE_MODE;

    // zero out the escape buffer just in case
    vterm->esbuf_len = 0;
    vterm->esbuf[0] = '\0';

    vterm->esc_handler = NULL;

    return;
}

void
vterm_escape_cancel(vterm_t *vterm)
{
    vterm->internal_state &= ~STATE_ESCAPE_MODE;

    // zero out the escape buffer for the next run
    vterm->esbuf_len = 0;
    vterm->esbuf[0] = '\0';

    vterm->esc_handler = NULL;

    return;
}

void
vterm_interpret_escapes(vterm_t *vterm)
{
    char                firstchar;
    char                *lastchar;

    static void         *interim_table[128] =
                            {
                                [0] = &&interim_char_none,
                                [']'] = &&interim_rbracket,
                                ['['] = &&interim_lbracket,
                                [')'] = &&interim_rparth,
                                ['('] = &&interim_lparth,
                                ['P'] = &&interim_char_P,
                            };

    int                 retval = 0;


    firstchar = vterm->esbuf[0];
    lastchar = &vterm->esbuf[vterm->esbuf_len - 1];

    // too early to do anything
    if(!firstchar) return;

    retval = vterm_interpret_escapes_simple(vterm);
    if(retval > 0) return;

    SWITCH(interim_table, (unsigned int)firstchar, 0);

    // looks like an complete xterm Operating System Command
    interim_rbracket:
        // term type linux sends this to reset FG and BG colors to default
        if(*lastchar == 'R')
        {
            vterm_escape_cancel(vterm);
            goto interim_run;
        }

        if(validate_xterm_escape_suffix(lastchar))
        {
            vterm->esc_handler = vterm_interpret_xterm_osc;
        }
        goto interim_run;

    // we have a complete csi escape sequence: interpret it
    interim_lbracket:
        if(validate_csi_escape_suffix(lastchar))
        {
            vterm->esc_handler = vterm_interpret_csi;
        }
        goto interim_run;

    // SCS G0 sequence - discards for now
    interim_lparth:
        if(validate_scs_escape_suffix(lastchar))
        {
            vterm->esc_handler = vterm_interpret_esc_scs;
        }
        goto interim_run;

    // SCS G1 sequence - discards for now
    interim_rparth:
        if(validate_scs_escape_suffix(lastchar))
        {
            vterm->esc_handler = vterm_interpret_esc_scs;
        }
        goto interim_run;


    // DCS sequence - starts in P and ends in Esc backslash
    interim_char_P:
        if( vterm->esbuf_len > 2
            && vterm->esbuf[vterm->esbuf_len - 2] == '\x1B'
            && *lastchar == '\\' )
        {
            vterm->esc_handler = vterm_interpret_esc_xterm_dsc;
        }
        goto interim_run;

    interim_char_none:
        vterm_escape_cancel(vterm);
        return;

    interim_run:

    // if an escape handler has been configured, run it.
    if(vterm->esc_handler != NULL)
    {
        vterm->esc_handler(vterm);
        vterm_escape_cancel(vterm);

        return;
    }

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

int
vterm_interpret_csi(vterm_t *vterm)
{
    static int  csiparam[MAX_CSI_ES_PARAMS];
    int         param_count = 0;
    const char  *p;
    char        verb;

    p = vterm->esbuf + 1;
    verb = vterm->esbuf[vterm->esbuf_len - 1];

    /*
        Parse numeric parameters

        The order of conditionals is intentional.  It's designed to
        favor the most likely to occur characters.  For example, in
        a standardized escape sequence the [ and ? will only occur
        once so put them at the bottom.
    */
    for(;;)
    {
        if(isdigit(*p))
        {
            if(param_count == 0)
            {
                csiparam[param_count] = 0;
                param_count++;
            }

            // increaase order of prev digit (10s column, 100s column, etc...)
            csiparam[param_count - 1] *= 10;
            csiparam[param_count - 1] += *p - '0';
            p++;
            continue;
        }

        if(*p == ';')
        {
            if(param_count >= MAX_CSI_ES_PARAMS) return -1;    // too long!

            csiparam[param_count] = 0;
            param_count++;
            p++;
            continue;
        }

        if(*p == '[')
        {
            p++;
            continue;
        }


        if(*p == '?')
        {
            p++;
            continue;
        }

        break;
    }

    // delegate handling depending on command character (verb)
    switch (verb)
    {
        case 'c':
        {
            // endwin(); exit(0);
            interpret_osc_DA(vterm);
            break;
        }

        case 'b':
        {
            interpret_csi_REP(vterm, csiparam, param_count);
            break;
        }

        case 'm':
        {
            interpret_csi_SGR(vterm, csiparam, param_count);
            break;
        }

        case 'l':
        {
            interpret_dec_RM(vterm, csiparam, param_count);
            break;
        }

        case 'h':
        {
            interpret_dec_SM(vterm, csiparam, param_count);
            break;
        }

        case 'J':
        {
            interpret_csi_ED(vterm, csiparam, param_count);
            break;
        }

        case 'H':
        {
            interpret_csi_CUx(vterm, verb, csiparam, param_count);
            break;
        }

        case 'f':
        {
            interpret_csi_CUP(vterm, csiparam, param_count);
            break;
        }

        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
        case 'G':
        case 'e':
        case 'a':
        case 'd':
        case '`':
        {
            interpret_csi_CUx(vterm, verb, csiparam, param_count);
            break;
        }

        case 'K':
        {
            interpret_csi_EL(vterm, csiparam, param_count);
            break;
        }

        case '@':
        {
            interpret_csi_ICH(vterm, csiparam, param_count);
            break;
        }

        case 'P':
        {
            interpret_csi_DCH(vterm, csiparam, param_count);
            break;
        }

        case 'L':
        {
            interpret_csi_IL(vterm, csiparam, param_count);
            break;
        }

        case 'M':
        {
            interpret_csi_DL(vterm, csiparam, param_count);
            break;
        }

        case 'X':
        {
            interpret_csi_ECH(vterm, csiparam, param_count);
            break;
        }

        case 'r':
        {
            interpret_csi_DECSTBM(vterm, csiparam, param_count);
            break;
        }

        case 's':
        {
            interpret_csi_SAVECUR(vterm, csiparam, param_count);
            break;
        }

        // S is defined as scroll up (SU) for both VT420 and ECMA-48
        case 'S':
        {
            interpret_csi_SU(vterm, csiparam, param_count);
            break;
        }

        case 'u':
        {
            interpret_csi_RESTORECUR(vterm, csiparam, param_count);
            break;
        }

        case 'Z':
        {
            interpret_csi_CBT(vterm, csiparam, param_count);
            break;
        }

        // T is defined for scroll down (SD) VT420, ^ is defined for ECMA-48
        case 'T':
        case '^':
        {
            interpret_csi_SD(vterm, csiparam, param_count);
            break;
        }

#ifdef _DEBUG
        default:
        {
            fprintf(stderr, "Unrecogized CSI: <%s>\n",
                vterm->esbuf);
            break;
        }
#endif
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


