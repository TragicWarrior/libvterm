
/*
    VT100 SGR documentation
    From http://vt100.net/docs/vt510-rm/SGR table 5-16
    0   All attributes off
    1   Bold
    4   Underline
    5   Blinking
    7   Negative image
    8   Invisible image
    10  The ASCII character set is the current 7-bit
        display character set (default) - SCO Console only.
    11  Map Hex 00-7F of the PC character set codes
        to the current 7-bit display character set
        - SCO Console only.
    12  Map Hex 80-FF of the current character set to
        the current 7-bit display character set - SCO
        Console only.
    22  Bold off
    24  Underline off
    25  Blinking off
    27  Negative image off
    28  Invisible image off
    38  Custom foreground color
    48  Custom background color
*/

#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_colors.h"
#include "vterm_buffer.h"

void
_vterm_set_color_pair_safe(vterm_t *vterm, short colors);

short
interpret_custom_color(vterm_t *vterm, int param[], int pcount);

/* interprets a 'set attribute' (SGR) CSI escape sequence */
void
interpret_csi_SGR(vterm_t *vterm, int param[], int pcount)
{
    vterm_desc_t    *v_desc = NULL;
    int             nested_params[MAX_CSI_ES_PARAMS];
    int             i;
    short           colors;
    static int      depth = 0;
    int             idx;
    short           fg, bg;
    int             retval;

    // this depth counter prevents a recursion bomb.  depth limit is arbitary.
    if (depth > 6) return;

    // set vterm_desc buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    if(pcount == 0)
    {
        v_desc->curattr = A_NORMAL;         // reset attributes
        return;
    }

    for(i = 0; i < pcount; i++)
    {
        // jump tables are always faster than conditionals
        switch(param[i])
        {
            case 0:
            {
                v_desc->curattr = A_NORMAL;     // reset attributes

                // attribute reset is an implicit color reset too so we'll
                // do a nested call to handle it.
                nested_params[0] = 39;
                nested_params[1] = 49;

                depth++;

                interpret_csi_SGR(vterm, nested_params, 2);
                depth--;

                break;
            }

            case 1:
            case 4:                              // bold on
            {
                v_desc->curattr |= A_BOLD;
                break;
            }

            case 2:
            {
                v_desc->curattr |= A_DIM;
                break;
            }

            // blink on
            case 5:
            {
                v_desc->curattr |= A_BLINK;
                break;
            }

            // reverse on
            case 7:
            {
                v_desc->curattr |= A_REVERSE;
                break;
            }

            // reverse off
            case 27:
            {
                v_desc->curattr &= ~(A_REVERSE);
                continue;
            }

            // invisible on
            case 8:
            {
                v_desc->curattr |= A_INVIS;
                break;
            }

            // rmacs
            case 10:
            {
                vterm->internal_state &= ~STATE_ALT_CHARSET;
                break;
            }

            // smacs
            case 11:
            {
                vterm->internal_state |= STATE_ALT_CHARSET;
                break;
            }

            // bold on
            case 22:
            case 24:
            {
                v_desc->curattr &= ~A_BOLD;
                break;
            }

            // blink off
            case 25:
            {
                v_desc->curattr &= ~A_BLINK;
                break;
            }

            // invisible off
            case 28:
            {
                v_desc->curattr &= ~A_INVIS;
                break;
            }

            // set fg color (case # - 30 = fg color)
            case 30:
            case 31:
            case 32:
            case 33:
            case 34:
            case 35:
            case 36:
            case 37:
            {
                v_desc->fg = param[i] - 30;

                // find the required pair in the cache
                colors = color_cache_find_pair(vterm->color_cache,
                    v_desc->fg, v_desc->bg);

                if(colors == -1)
                {
                    colors = color_cache_add_new_pair(vterm->color_cache,
                        v_desc->fg, v_desc->bg);
                }

                _vterm_set_color_pair_safe(vterm, colors);

                break;
            }

            // set custom foreground color
            case 38:
            {
                fg = interpret_custom_color(vterm, param, pcount);

                if(fg != -1)
                {
                    v_desc->fg = fg;

                    colors = color_cache_find_pair(vterm->color_cache,
                        v_desc->fg, v_desc->bg);

                    if(colors == -1)
                    {
                        colors = color_cache_add_new_pair(vterm->color_cache,
                            v_desc->fg, v_desc->bg);
                    }

                    _vterm_set_color_pair_safe(vterm, colors);

                    i += 2;
                }

            break;
            }

            // reset fg color
            case 39:
            {
                retval = color_cache_split_pair(vterm->color_cache,
                    v_desc->default_colors, &fg, &bg);

                if(retval != -1)
                {
                    v_desc->fg = fg;

                    colors = color_cache_find_pair(vterm->color_cache,
                        v_desc->fg, v_desc->bg);
                }
                else
                {
                    colors = 0;
                }

#ifdef NOCURSES
                // should not ever execute - bad combination of flags and
                // #define's.
                colors = 0;
#endif

                // one addtl safeguard
                if(colors == -1) colors = 0;

                _vterm_set_color_pair_safe(vterm, colors);

            break;
            }

            // set bg color (case # - 40 = fg color)
            case 40:
            case 41:
            case 42:
            case 43:
            case 44:
            case 45:
            case 46:
            case 47:
            {
                v_desc->bg = param[i] - 40;

                // find the required pair in the cache
                colors = color_cache_find_pair(vterm->color_cache,
                    v_desc->fg, v_desc->bg);

                // no color pair found so we'll try and add it
                if(colors == -1)
                {
                    colors = color_cache_add_new_pair(vterm->color_cache,
                        v_desc->fg, v_desc->bg);
                }

                _vterm_set_color_pair_safe(vterm, colors);

                break;
            }

            // set custom background color
            case 48:
            {
                bg = interpret_custom_color(vterm, param, pcount);

                if(bg != -1)
                {
                    v_desc->bg = bg;

                    colors = color_cache_find_pair(vterm->color_cache,
                        v_desc->fg, v_desc->bg);

                    if(colors == -1)
                    {
                        colors = color_cache_add_new_pair(vterm->color_cache,
                            v_desc->fg, v_desc->bg);
                    }

                    _vterm_set_color_pair_safe(vterm, colors);

                    i += 2;
                }

            break;
            }

            // reset bg color
            case 49:
            {
                retval = color_cache_split_pair(vterm->color_cache,
                    v_desc->default_colors, &fg, &bg);

                if(retval != -1)
                {
                    v_desc->bg = bg;

                    colors = color_cache_find_pair(vterm->color_cache,
                        v_desc->fg, v_desc->bg);
                }
                else
                {
                    colors = 0;
                }

#ifdef NOCURSES
                // should not ever execute - bad combination of flags and
                // #define's.
                colors = 0;
#endif

                // one addtl safeguard
                if(colors == -1) colors = 0;

                _vterm_set_color_pair_safe(vterm, colors);

                break;
            }
        }
    }
}

inline void
_vterm_set_color_pair_safe(vterm_t *vterm, short colors)
{
    vterm_desc_t    *v_desc = NULL;
    attr_t          attr_saved = 0;
    int             idx;

    // set vterm_desc buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    /*
        the COLOR_PAIR macros seems to trample attributes.
        save them before making changes and OR them back in.
    */

    if(v_desc->curattr & A_REVERSE) attr_saved |= A_REVERSE;
    if(v_desc->curattr & A_BOLD) attr_saved |= A_BOLD;
    if(v_desc->curattr & A_DIM) attr_saved |= A_DIM;

    v_desc->curattr = 0;
    v_desc->curattr |= COLOR_PAIR(colors);
    v_desc->curattr |= attr_saved;

    return;
}

/*
    ISO-8613-6 sequenences are as follows:

    Set foreground by RGB:
    ESC [ 38 ; 2 ; i ; r ; g ; b m

    Set foreground by color
    ESC [ 38 ; 5 ; n m

    Set bagkground by RGB:
    ESC [ 48 ; 2 ; i ; r ; g ; b m

    Set background by color
    ESC [ 48 ; 5 ; n m

    i = color space (always ignored)
    r = red value
    g = green value
    b = blue value
    n = a defined color
*/
inline short
interpret_custom_color(vterm_t *vterm, int param[], int pcount)
{
    int     method = 0;
    // short   red;
    // short   green;
    // short   blue;

    if(vterm == NULL) return -1;
    if(pcount < 2) return -1;

    method = param[1];

    // set to color pair
    if(method == 5)
    {
        /*
            without having initmate knowledge of the client application,
            it's not possible to determine what color is being
            requested.
        */
        if(pcount < 3) return -1;

        return (short)param[2];
    }

    // set to nearest rgb value
    if(method == 2)
    {
        if(pcount < 6) return -1;

        // red = param[3];
        // green = param[4];
        // blue = param[5];

        // endwin();
        // printf("r: %d, g: %d, b: %d\n\r", red, green, blue);
        // exit(0);

        return -1;
    }

/*
SGR_DEBUG:

    endwin();

    printf("params %d\n\r", pcount);
    int i;
    for(i = 0; i < pcount; i++)
    {
        printf("%d ", param[i]);
    }
    printf("\n\r");

    exit(0);
*/

    return -1;
}
