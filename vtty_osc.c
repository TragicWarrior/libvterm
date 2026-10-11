#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <strings.h>

#include "macros.h"
#include "stringv.h"
#include "vtty.h"
#include "vtty_private.h"
#include "vtty_ctrl_char.h"
#include "vtty_osc.h"

static int
vtty_osc_read_string(vtty_t *vtty, char *esbuf, char *buf, int buf_sz);

static void
vtty_osc_set_title(vtty_t *vtty, char *pos);

static void
vtty_osc_parse_xcolor(vtty_t *vtty, char *buf, int buf_sz);

static void
vtty_osc_parse_clipboard(vtty_t *vtty, char *pos);

static int
vtty_osc_is_term(const char *p);

static int
vtty_b64_val(unsigned char c);

static int
vtty_b64_decode(const char *src, size_t src_len, char **out, size_t *out_len);

static void
vtty_clipboard_store(vtty_t *vtty, char *data, size_t len, char selection);

/*  public function */

void
vtty_get_title(vtty_t *vtty, char *buf, int buf_sz)
{
    if(vtty == NULL) return;
    if(buf == NULL) return;
    if(buf_sz < 2) return;

    memset(buf, 0, buf_sz);

    if(vtty->title == NULL) return;

    strncpy(buf, vtty->title, buf_sz - 1);

    return;
}

/* private functions */

int
vtty_interpret_xterm_osc(vtty_t *vtty)
{
    char        buf[128];           // general purpose capture buffer
    int         verb = 0;
    int         digits = 0;
    char        *pos;
    int         count = 0;
    int         max_sz;

    // advance past OSC which is ESC ]
    pos = vtty->esbuf + 1;

    while(isdigit(*pos))
    {
        verb *= 10;
        verb += (*pos) - '0';
        pos++;
        digits++;
    }

    /*
        an OSC with no number is not one of ours -- the linux console's
        palette sequences (ESC ] P and ESC ] R) arrive this way.  this
        must count digits and not test the value:  0 is a real function,
        and the one most programs use to set the title.
    */
    if(digits == 0) return 0;

    switch(verb)
    {
        // Change Icon Name and Window Title
        case 0:

        // Change Window Title
        case 2:
        {
            vtty_osc_set_title(vtty, pos);
            break;
        }

        /*
            Change Icon Name.  a terminal in a window has no icon, and
            the icon name is not the title:  a program that sends both
            would otherwise show whichever came last.
        */
        case 1:
        {
            break;
        }

        // Define a custom RGB color)
        case 4:
        {
            max_sz = ARRAY_SZ(buf);
            count = vtty_osc_read_string(vtty, pos, buf, max_sz);

            // no string followed the number
            if(count < 0) break;

            vtty_osc_parse_xcolor(vtty, buf, count);

            break;
        }

        /*
            OSC 52 ; Pc ; Pd ST  -- clipboard SET from the child.

            this is the inner hop only: decode and hand the payload to
            the embedder.  query (Pd = '?') is refused.  empty Pd clears.
        */
        case 52:
        {
            vtty_osc_parse_clipboard(vtty, pos);
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
vtty_osc_read_string(vtty_t *vtty, char *esbuf, char *buf, int buf_sz)
{
    char    *pos;
    int     count = 0;

    if(vtty == NULL) return -1;
    if(buf_sz < 2) return -1;

    pos = buf;

    memset(buf, 0, buf_sz);

    /*
        strings begin with a semicolon, advance past it.  a sequence
        that ends before one is found has no string, and the search must
        stop there instead of running on through memory.
    */
    for(;; esbuf++)
    {
        if(*esbuf == ';')
        {
            esbuf++;
            break;
        }

        if(*esbuf == '\0' || vtty_osc_is_term(esbuf)) return -1;
    }

    // make room for null terminator
    buf_sz--;

    for(;;)
    {
        // the end of the sequence, however it was terminated
        if(*esbuf == '\0') break;

        // bell terminates an OSC string
        if(*esbuf == '\x07') break;

        /*
            so does the C1 string terminator 0x9c, but only in 8-bit
            mode.  in UTF-8 mode 0x9c is an ordinary continuation byte
            (U+2733 is E2 9C B3) and stopping on it cuts the string in
            the middle of a character.
        */
        if(*esbuf == '\x9c' && (vtty->flags & VTTY_FLAG_NOUTF8))
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

/*
    OSC 0 and OSC 2:  the child names its window.

    the string is stored for vtty_get_title() and, when it differs from
    the title already held, announced with VTTY_EVENT_TITLE.  a shell
    prompt re-sends the same title on every command, so an unchanged
    title is not announced.

    an empty string is a valid title:  it is how a program gives the
    title back, and the host is expected to fall back on its own name
    for the window.
*/
static void
vtty_osc_set_title(vtty_t *vtty, char *pos)
{
    char    buf[VTTY_TITLE_BUF_SZ];
    int     count;
    int     start;
    int     need;

    count = vtty_osc_read_string(vtty, pos, buf, VTTY_TITLE_BUF_SZ);

    // no string followed the number
    if(count < 0) return;

    /*
        a title that filled the buffer may have been cut in the middle
        of a multi-byte character.  find the last lead byte and, if the
        bytes it calls for are not all there, drop the fragment.
    */
    if(count == VTTY_TITLE_BUF_SZ - 1 && !(vtty->flags & VTTY_FLAG_NOUTF8))
    {
        start = count - 1;

        // step back over continuation bytes (10xxxxxx)
        while(start > 0 && ((unsigned char)buf[start] & 0xC0) == 0x80)
            start--;

        need = 1;
        if(((unsigned char)buf[start] & 0xE0) == 0xC0) need = 2;
        if(((unsigned char)buf[start] & 0xF0) == 0xE0) need = 3;
        if(((unsigned char)buf[start] & 0xF8) == 0xF0) need = 4;

        if(count - start < need)
        {
            memset(buf + start, 0, count - start);
            count = start;
        }
    }

    // nothing to do when the title is the one already held
    if(vtty->title == NULL && count == 0) return;
    if(vtty->title != NULL && strcmp(vtty->title, buf) == 0) return;

    if(vtty->title == NULL)
    {
        vtty->title = (char *)calloc(1, VTTY_TITLE_BUF_SZ);
        if(vtty->title == NULL) return;
    }

    memcpy(vtty->title, buf, VTTY_TITLE_BUF_SZ);

    if(vtty->event_hook == NULL) return;
    if(!(vtty->event_mask & VTTY_MASK_TITLE)) return;

    vtty->event_hook(vtty, VTTY_EVENT_TITLE, vtty->title);

    return;
}

void
vtty_osc_parse_xcolor(vtty_t *vtty, char *buf, int buf_sz)
{
    char    **params = NULL;
    char    *pos;
    short   new_color;
    short   r, g, b;

    VAR_UNUSED(vtty);     // make compiler happy

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
        accordingly and vtty_add_mapped_color() does that on its own.
    */
    vtty_add_mapped_color(vtty, new_color, (float)r, (float)g, (float)b);

    return;
}

int
vtty_clipboard_get(vtty_t *vtty, const char **data, size_t *len)
{
    if(vtty == NULL) return -1;

    if(data != NULL) *data = vtty->clipboard;
    if(len != NULL) *len = vtty->clipboard_len;

    return (vtty->clipboard != NULL) ? 1 : 0;
}

void
vtty_clipboard_clear(vtty_t *vtty)
{
    if(vtty == NULL) return;

    free(vtty->clipboard);
    vtty->clipboard = NULL;
    vtty->clipboard_len = 0;
    vtty->clipboard_sel = 0;

    return;
}

static int
vtty_osc_is_term(const char *p)
{
    if(p == NULL || *p == '\0') return 1;
    if(*p == '\x07') return 1;
    if((unsigned char)*p == 0x9c) return 1;
    if(*p == '\x1b' && p[1] == '\\') return 1;

    return 0;
}

static int
vtty_b64_val(unsigned char c)
{
    if(c >= 'A' && c <= 'Z') return c - 'A';
    if(c >= 'a' && c <= 'z') return c - 'a' + 26;
    if(c >= '0' && c <= '9') return c - '0' + 52;
    if(c == '+') return 62;
    if(c == '/') return 63;

    return -1;
}

static int
vtty_b64_decode(const char *src, size_t src_len, char **out, size_t *out_len)
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

            v = vtty_b64_val(c);
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
vtty_clipboard_store(vtty_t *vtty, char *data, size_t len, char selection)
{
    vtty_clipboard_t    ev;

    free(vtty->clipboard);
    vtty->clipboard = data;
    vtty->clipboard_len = len;
    vtty->clipboard_sel = selection;

    if(vtty->event_hook == NULL) return;
    if(!(vtty->event_mask & VTTY_MASK_CLIPBOARD)) return;

    ev.data = data;
    ev.len = len;
    ev.selection = selection;
    vtty->event_hook(vtty, VTTY_EVENT_CLIPBOARD, &ev);

    return;
}

static void
vtty_osc_parse_clipboard(vtty_t *vtty, char *pos)
{
    char        *pc;
    char        *pd;
    char        *decoded = NULL;
    size_t      pd_len;
    size_t      decoded_len = 0;
    char        selection;

    if(vtty == NULL || pos == NULL) return;

    /* skip the semicolon after the verb (and nothing else) */
    if(*pos == ';') pos++;
    else return;

    pc = pos;
    while(*pos != '\0' && *pos != ';' && !vtty_osc_is_term(pos))
        pos++;

    if(*pos != ';') return;

    selection = (pos > pc) ? pc[0] : 'c';
    pos++;
    pd = pos;

    while(*pos != '\0' && !vtty_osc_is_term(pos))
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
            vtty_clipboard_store(vtty, NULL, 0, selection);
            return;
        }

        if(vtty_b64_decode(s, (size_t)(e - s), &decoded, &decoded_len) != 0)
            return;
    }

    vtty_clipboard_store(vtty, decoded, decoded_len, selection);

    return;
}
