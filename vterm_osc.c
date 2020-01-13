#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <strings.h>

#include "macros.h"
#include "stringv.h"
#include "vterm.h"
#include "vterm_private.h"
#include "vterm_ctrl_char.h"
#include "vterm_osc.h"

static int
vterm_osc_read_string(vterm_t *vterm, char *esbuf, char *buf, int buf_sz);

static void
vterm_osc_parse_xcolor(vterm_t *vterm, char *buf, int buf_sz);

/*  public function */

void
vterm_get_title(vterm_t *vterm, char *buf, int buf_sz)
{
    if(vterm == NULL) return;
    if(buf == NULL) return;
    if(buf_sz < 2) return;

    memset(buf, 0, buf_sz);
    strncpy(buf, vterm->title, buf_sz - 1);

    return;
}

/* private functions */

int
vterm_interpret_xterm_osc(vterm_t *vterm)
{
    char        buf[128];           // general purpose capture buffer
    int         verb = 0;
    char        *pos;
    int         count = 0;
    int         max_sz;

    // advance past OSC which is ESC ]
    pos = vterm->esbuf + 1;

    while(isdigit(*pos))
    {
        verb *= 10;
        verb += (*pos) - '0';
        pos++;
    }

    if(verb == 0) return 0;

    switch(verb)
    {
        // Change Icon Name and Window Title
        case 0:
        {
            max_sz = ARRAY_SZ(vterm->title);
            count = vterm_osc_read_string(vterm, pos, vterm->title, max_sz);

            break;
        }

        // Change Icon Name
        case 1:

        // Change Window Title
        case 2:
        {
            /*
                todo:  for now we will simply copy the string and
                treat all OSC sequences the same (icon, name, both).
            */
            max_sz = ARRAY_SZ(vterm->title);
            count = vterm_osc_read_string(vterm, pos, vterm->title, max_sz);

            break;
        }

        // Define a custom RGB color)
        case 4:
        {
            max_sz = ARRAY_SZ(buf);
            count = vterm_osc_read_string(vterm, pos, buf, max_sz);

            vterm_osc_parse_xcolor(vterm, buf, count);

            break;
        }

        // Unknown purpose.  Part of xterm u8 (user defined string #8)
        case 7:
        // Also unknown purpose.
        case 777:
        {
            break;
        }

        default:
            break;
    }

    return count;
}

int
vterm_osc_read_string(vterm_t *vterm, char *esbuf, char *buf, int buf_sz)
{
    char    *pos;
    int     count = 0;

    if(vterm == NULL) return -1;
    if(buf_sz < 2) return -1;

    pos = buf;

    // strings begin with a semicolon, advance past it
    for(;; esbuf++)
    {
        if(*esbuf == ';')
        {
            esbuf++;
            break;
        }
    }

    memset(buf, 0, buf_sz);

    // make room for null terminator
    buf_sz--;

    for(;;)
    {
        // both bell and 0x9c can terminate a OSC string
        if(*esbuf == '\x07' || *esbuf == '\x9c')
        {
            break;
        }

        // the seqencue ESC \ (0x5C) can also terminate a OSC string
        if(*esbuf == '\x1b' && esbuf[1] == '\x5c')
        {
            break;
        }

        // limit hit
        if(count == buf_sz) break;

        // control chars in an OSC string are ignored
        if(!IS_CTRL_CHAR(*esbuf))
        {
            // copy a character from the escape buffer into user buffer
            *pos = *esbuf;

            count++;
            pos++;
        }

        esbuf++;
    }

    return count;
}

void
vterm_osc_parse_xcolor(vterm_t *vterm, char *buf, int buf_sz)
{
    char    **params = NULL;
    char    *pos;
    short   new_color;
    short   r, g, b;

    VAR_UNUSED(vterm);    // make compiler happy

    pos = buf;

    // replace any slashes or colons with semicolons
    while(buf_sz > 0)
    {
        switch(*pos)
        {
            case ':':
            case '/':   *pos = ';';     break;

            case '\0':  break;
        }

        pos++;
        buf_sz--;
    }

    // explode string by semicolon
    params = strsplitv(buf, ";");

    if(params == NULL) return;

    if(strncasecmp(params[1], "rgb", sizeof("rgb") - 1) != 0)
    {
        strfreev(params);
        return;
    }

    new_color = (short)atoi(params[0]);

    /*
        XParseColor RGB values are specifed in base-16 and range from
        0x00 to 0xFF.
    */
    r = (short)(strtol(params[2], NULL, 16));
    g = (short)(strtol(params[3], NULL, 16));
    b = (short)(strtol(params[4], NULL, 16));

    strfreev(params);

    /*
        The ncurses RGB values run from 0 - 1000.  We need to scale
        accordingly and vterm_add_mapped_color() does that on its own.
    */
    vterm_add_mapped_color(vterm, new_color, (float)r, (float)g, (float)b);

    return;
}
