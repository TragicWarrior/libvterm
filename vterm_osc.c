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

static void
vterm_osc_parse_clipboard(vterm_t *vterm, char *pos);

static int
vterm_osc_is_term(const char *p);

static int
vterm_b64_val(unsigned char c);

static int
vterm_b64_decode(const char *src, size_t src_len, char **out, size_t *out_len);

static void
vterm_clipboard_store(vterm_t *vterm, char *data, size_t len, char selection);

/*  public function */

void
vterm_get_title(vterm_t *vterm, char *buf, int buf_sz)
{
    if(vterm == NULL) return;
    if(buf == NULL) return;
    if(buf_sz < 2) return;

    memset(buf, 0, buf_sz);

    if(vterm->title == NULL) return;

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

        // Change Icon Name
        case 1:

        // Change Window Title
        case 2:
        {
            /*
                todo:  for now we will simply copy the string and
                treat all OSC sequences the same (icon, name, both).
            */
            if(vterm->title == NULL)
            {
                vterm->title = (char *)calloc(1, VTERM_TITLE_BUF_SZ);
                if(vterm->title == NULL) break;
            }

            count = vterm_osc_read_string(vterm, pos, vterm->title,
                VTERM_TITLE_BUF_SZ);

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

        /*
            OSC 52 ; Pc ; Pd ST  -- clipboard SET from the child.

            this is the inner hop only: decode and hand the payload to
            the embedder.  query (Pd = '?') is refused.  empty Pd clears.
        */
        case 52:
        {
            vterm_osc_parse_clipboard(vterm, pos);
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

int
vterm_clipboard_get(vterm_t *vterm, const char **data, size_t *len)
{
    if(vterm == NULL) return -1;

    if(data != NULL) *data = vterm->clipboard;
    if(len != NULL) *len = vterm->clipboard_len;

    return (vterm->clipboard != NULL) ? 1 : 0;
}

void
vterm_clipboard_clear(vterm_t *vterm)
{
    if(vterm == NULL) return;

    free(vterm->clipboard);
    vterm->clipboard = NULL;
    vterm->clipboard_len = 0;
    vterm->clipboard_sel = 0;

    return;
}

static int
vterm_osc_is_term(const char *p)
{
    if(p == NULL || *p == '\0') return 1;
    if(*p == '\x07') return 1;
    if((unsigned char)*p == 0x9c) return 1;
    if(*p == '\x1b' && p[1] == '\\') return 1;

    return 0;
}

static int
vterm_b64_val(unsigned char c)
{
    if(c >= 'A' && c <= 'Z') return c - 'A';
    if(c >= 'a' && c <= 'z') return c - 'a' + 26;
    if(c >= '0' && c <= '9') return c - '0' + 52;
    if(c == '+') return 62;
    if(c == '/') return 63;

    return -1;
}

static int
vterm_b64_decode(const char *src, size_t src_len, char **out, size_t *out_len)
{
    int         val[4];
    int         nval = 0;
    int         pads = 0;
    size_t      cap;
    size_t      n = 0;
    size_t      i;
    char        *buf;

    *out = NULL;
    *out_len = 0;

    cap = (src_len / 4 + 1) * 3;
    buf = (char *)malloc(cap + 1);
    if(buf == NULL) return -1;

    for(i = 0; i < src_len; i++)
    {
        unsigned char   c = (unsigned char)src[i];
        int             v;

        if(c == ' ' || c == '\t' || c == '\n' || c == '\r')
            continue;

        if(c == '=')
        {
            v = 0;
            pads++;
        }
        else
        {
            if(pads > 0)
            {
                free(buf);
                return -1;
            }

            v = vterm_b64_val(c);
            if(v < 0)
            {
                free(buf);
                return -1;
            }
        }

        val[nval++] = v;
        if(nval < 4) continue;

        if(n + 3 > cap)
        {
            free(buf);
            return -1;
        }

        buf[n++] = (char)((val[0] << 2) | (val[1] >> 4));
        if(pads < 2)
            buf[n++] = (char)((val[1] << 4) | (val[2] >> 2));
        if(pads < 1)
            buf[n++] = (char)((val[2] << 6) | val[3]);

        nval = 0;
        if(pads > 0) break;
    }

    if(nval != 0)
    {
        free(buf);
        return -1;
    }

    *out = buf;
    *out_len = n;
    return 0;
}

static void
vterm_clipboard_store(vterm_t *vterm, char *data, size_t len, char selection)
{
    vterm_clipboard_t   ev;

    free(vterm->clipboard);
    vterm->clipboard = data;
    vterm->clipboard_len = len;
    vterm->clipboard_sel = selection;

    if(vterm->event_hook == NULL) return;
    if(!(vterm->event_mask & VTERM_MASK_CLIPBOARD)) return;

    ev.data = data;
    ev.len = len;
    ev.selection = selection;
    vterm->event_hook(vterm, VTERM_EVENT_CLIPBOARD, &ev);

    return;
}

static void
vterm_osc_parse_clipboard(vterm_t *vterm, char *pos)
{
    char        *pc;
    char        *pd;
    char        *decoded = NULL;
    size_t      pd_len;
    size_t      decoded_len = 0;
    char        selection;

    if(vterm == NULL || pos == NULL) return;

    /* skip the semicolon after the verb (and nothing else) */
    if(*pos == ';') pos++;
    else return;

    pc = pos;
    while(*pos != '\0' && *pos != ';' && !vterm_osc_is_term(pos))
        pos++;

    if(*pos != ';') return;

    selection = (pos > pc) ? pc[0] : 'c';
    pos++;
    pd = pos;

    while(*pos != '\0' && !vterm_osc_is_term(pos))
        pos++;

    pd_len = (size_t)(pos - pd);

    {
        const char  *s = pd;
        const char  *e = pd + pd_len;

        while(s < e && (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r'))
            s++;
        while(e > s && (e[-1] == ' ' || e[-1] == '\t' ||
            e[-1] == '\n' || e[-1] == '\r'))
            e--;

        /* query: refuse -- do not reply, do not touch the stored payload */
        if((e - s) == 1 && *s == '?')
            return;

        /* empty Pd: clear */
        if(s == e)
        {
            vterm_clipboard_store(vterm, NULL, 0, selection);
            return;
        }

        if(vterm_b64_decode(s, (size_t)(e - s), &decoded, &decoded_len) != 0)
            return;
    }

    vterm_clipboard_store(vterm, decoded, decoded_len, selection);

    return;
}
