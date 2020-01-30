#include <ctype.h>
#include <wchar.h>

#include "macros.h"
#include "vterm.h"
#include "vterm_error.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_osc_DA.h"

int
vterm_interpret_csi(vterm_t *vterm)
{
    static int      csiparam[MAX_CSI_ES_PARAMS];
    int             param_count = 0;
    const char      *p;
    char            verb;
    bool            dec_sequence = FALSE;

    static void     *csi_table[] =
                    {
                        [0] = &&csi_char_unknown,
                        ['c'] = &&csi_DA,
                        ['b'] = &&csi_char_b,
                        ['m'] = &&csi_char_m,
                        ['l'] = &&csi_char_l,
                        ['h'] = &&csi_char_h,
                        ['H'] = &&csi_char_H,
                        ['J'] = &&csi_char_J,
                        ['f'] = &&csi_HVP,
                        ['A'] = &&csi_CUx,
                        ['B'] = &&csi_CUx,
                        ['C'] = &&csi_CUx,
                        ['D'] = &&csi_CUx,
                        ['E'] = &&csi_CUx,
                        ['F'] = &&csi_CUx,
                        ['G'] = &&csi_CUx,
                        ['e'] = &&csi_CUx,
                        ['a'] = &&csi_CUx,
                        ['d'] = &&csi_CUx,
                        ['`'] = &&csi_CUx,
                        ['K'] = &&csi_EL,
                        ['@'] = &&csi_char_sym_at,
                        ['P'] = &&csi_char_P,
                        ['L'] = &&csi_char_L,
                        ['M'] = &&csi_char_M,
                        ['X'] = &&csi_ECH,
                        ['r'] = &&csi_char_r,
                        ['s'] = &&csi_char_s,
                        ['u'] = &&csi_char_u,
                        ['^'] = &&csi_SD,
                        ['T'] = &&csi_SD,
                        ['t'] = &&csi_EWMH,
                        ['Z'] = &&csi_char_Z,
                        ['S'] = &&csi_SU,
                        ['p'] = &&csi_char_p,
                    };

    p = vterm->esbuf + 1;
    verb = vterm->esbuf[vterm->esbuf_len - 1];

    /*
        Parse numeric parameters

        The order of conditionals is intentional.  It's designed to
        favor the most likely to occur characters.  For example, in
        a standardized escape sequence the [ and ? will only occur
        once so put them at the bottom.
    */
    csiparam[param_count] = 0;
    if(*p != verb) param_count = 1;
    for(;;)
    {
        if(isdigit(*p))
        {
            // increaase order of prev digit (10s column, 100s column, etc...)
            csiparam[param_count - 1] *= 10;
            csiparam[param_count - 1] += *p - '0';
            p++;
            continue;
        }

        if(*p == ';' || *p == ':')
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
            dec_sequence = TRUE;
            p++;
            continue;
        }

        if(*p == '!')
        {
            dec_sequence = TRUE;
            p++;
            continue;
        }

        break;
    }

    // jump table
    SWITCH(csi_table, (unsigned int)verb, 0);

    // delegate handling depending on command character (verb)
    csi_DA:
        interpret_osc_DA(vterm, csiparam, param_count);
        return 0;

    csi_char_b:
        interpret_csi_REP(vterm, csiparam, param_count);
        return 0;

    csi_char_m:
        interpret_csi_SGR(vterm, csiparam, param_count);
        return 0;

    csi_char_l:
        if(dec_sequence == TRUE)
            interpret_dec_RM(vterm, csiparam, param_count);
        else
            interpret_csi_IRM(vterm, TRUE);

        return 0;

    csi_char_h:
        if(dec_sequence == TRUE)
            interpret_dec_SM(vterm, csiparam, param_count);
        else
            interpret_csi_IRM(vterm, FALSE);

       return 0;

    csi_char_J:
        interpret_csi_ED(vterm, csiparam, param_count);
        return 0;

    csi_char_H:
        interpret_csi_CUx(vterm, verb, csiparam, param_count);
        return 0;

    csi_HVP:
        // HVP is the same as CUP (CUx)
        interpret_csi_CUx(vterm, verb, csiparam, param_count);
        return 0;

    csi_CUx:
        interpret_csi_CUx(vterm, verb, csiparam, param_count);
        return 0;

    csi_EL:
        interpret_csi_EL(vterm, csiparam, param_count);
        return 0;

    csi_char_sym_at:
        interpret_csi_ICH(vterm, csiparam, param_count);
        return 0;

    csi_char_P:
        interpret_csi_DCH(vterm, csiparam, param_count);
        return 0;

    csi_char_L:
        interpret_csi_IL(vterm, csiparam, param_count);
        return 0;

    csi_char_M:
        interpret_csi_DL(vterm, csiparam, param_count);
        return 0;

    csi_ECH:
        interpret_csi_ECH(vterm, csiparam, param_count);
        return 0;

    csi_char_r:
        interpret_csi_DECSTBM(vterm, csiparam, param_count);
        return 0;

    csi_char_s:
        interpret_csi_SAVECUR(vterm, csiparam, param_count);
        return 0;

    csi_char_u:
        interpret_csi_RESTORECUR(vterm, csiparam, param_count);
        return 0;

    csi_char_p:
        // if we catch a DECSTR then push RS1 into the system
        if(dec_sequence == TRUE)
        {
            vterm_render(vterm, RXVT_RS1, sizeof(RXVT_RS1) - 1);
            return 0;
        }

    csi_char_Z:
        interpret_csi_CBT(vterm, csiparam, param_count);
        return 0;

    csi_SD:
        interpret_csi_SD(vterm, csiparam, param_count);
        // interpret_csi_SU(vterm, csiparam, param_count);
        return 0;

    csi_SU:
        // interpret_csi_SD(vterm, csiparam, param_count);
        interpret_csi_SU(vterm, csiparam, param_count);
        return 0;

    csi_EWMH:
        // Extended Window Manager Hints
        // currently unsupported
        return 0;

    csi_char_unknown:
        vterm_error(vterm, VTERM_ECODE_UNHANDLED_CSI, (void *)vterm->esbuf); 

    return 0;
}

