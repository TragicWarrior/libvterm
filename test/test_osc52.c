/*
    OSC 52 inner-hop tests (headless, NOPTY | NOCURSES).

    Pins:
      - SET decodes base64 into vterm_clipboard_get
      - hook fires with VTERM_MASK_CLIPBOARD
      - query (Pd = '?') is refused (no reply, store unchanged)
      - empty Pd clears
      - payloads larger than ESEQ_BUF_SIZE (128) still arrive
      - ESC \ terminator works as well as BEL
      - bad base64 leaves the previous payload alone
      - default Pc is CLIPBOARD ('c')

    Build: cmake -DBUILD_TESTS=ON . && make test_osc52
*/

#ifndef _XOPEN_SOURCE_EXTENDED
#define _XOPEN_SOURCE_EXTENDED
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vterm.h"

static int failures = 0;
static int hook_calls = 0;
static vterm_clipboard_t last_ev;

#define CHECK(_cond, ...)                                   \
            do                                              \
            {                                               \
                if(!(_cond))                                \
                {                                           \
                    fprintf(stderr, "FAIL: " __VA_ARGS__);  \
                    fprintf(stderr, "\n");                  \
                    failures++;                             \
                }                                           \
            }                                               \
            while(0)

static void
R(vterm_t *vt, const char *s)
{
    vterm_render(vt, (char *)s, (int)strlen(s));
}

static void
on_clip(vterm_t *vterm, int event, void *anything)
{
    (void)vterm;

    if(event != VTERM_EVENT_CLIPBOARD) return;

    hook_calls++;
    last_ev = *(vterm_clipboard_t *)anything;
}

static void
reset_hook(void)
{
    hook_calls = 0;
    memset(&last_ev, 0, sizeof(last_ev));
}

static void
b64_encode(const unsigned char *src, size_t len, char *dst, size_t dst_sz)
{
    static const char tbl[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t  i = 0;
    size_t  o = 0;

    while(i < len)
    {
        unsigned int grp = (unsigned int)src[i] << 16;
        int n = 1;

        if(i + 1 < len)
        {
            grp |= (unsigned int)src[i + 1] << 8;
            n++;
        }
        if(i + 2 < len)
        {
            grp |= (unsigned int)src[i + 2];
            n++;
        }
        i += (size_t)n;

        if(o + 4 >= dst_sz) break;
        dst[o++] = tbl[(grp >> 18) & 63];
        dst[o++] = tbl[(grp >> 12) & 63];
        dst[o++] = (n > 1) ? tbl[(grp >> 6) & 63] : '=';
        dst[o++] = (n > 2) ? tbl[grp & 63] : '=';
    }

    dst[o] = '\0';
}

int
main(void)
{
    vterm_t     *vt;
    const char  *data = NULL;
    size_t      len = 0;
    char        seq[1024];
    char        encoded[512];
    char        raw[200];

    vt = vterm_create(80, 24, VTERM_FLAG_NOPTY | VTERM_FLAG_NOCURSES);
    if(vt == NULL)
    {
        fprintf(stderr, "vterm_create failed\n");
        return 1;
    }

    vterm_install_hook(vt, on_clip);
    vterm_set_event_mask(vt, VTERM_MASK_CLIPBOARD);

    /* SET "Hello" via BEL.  SGVsbG8= is the canonical encoding. */
    reset_hook();
    R(vt, "\033]52;c;SGVsbG8=\007");
    CHECK(vterm_clipboard_get(vt, &data, &len) == 1,
        "SET should store a payload");
    CHECK(len == 5 && data != NULL && memcmp(data, "Hello", 5) == 0,
        "SET decoded Hello, got len=%zu", len);
    CHECK(hook_calls == 1, "hook should fire once, got %d", hook_calls);
    CHECK(last_ev.len == 5 && last_ev.selection == 'c',
        "hook payload len=%zu sel=%c", last_ev.len, last_ev.selection);

    /* query must not reply and must not change the store */
    reset_hook();
    R(vt, "\033]52;c;?\007");
    CHECK(vterm_clipboard_get(vt, &data, &len) == 1,
        "query must leave the payload");
    CHECK(len == 5 && memcmp(data, "Hello", 5) == 0,
        "query must not mutate Hello");
    CHECK(hook_calls == 0, "query must not fire the hook, got %d", hook_calls);

    /* empty Pd clears and notifies */
    reset_hook();
    R(vt, "\033]52;c;\007");
    CHECK(vterm_clipboard_get(vt, &data, &len) == 0,
        "empty Pd should clear");
    CHECK(hook_calls == 1, "clear should fire the hook");
    CHECK(last_ev.data == NULL && last_ev.len == 0,
        "clear hook should pass NULL/0");

    /* default Pc (empty) is CLIPBOARD; ESC \\ terminator */
    reset_hook();
    R(vt, "\033]52;;SGk=\033\\");
    CHECK(vterm_clipboard_get(vt, &data, &len) == 1, "default Pc SET");
    CHECK(len == 2 && memcmp(data, "Hi", 2) == 0, "ESC\\ decoded Hi");
    CHECK(last_ev.selection == 'c', "empty Pc should default to 'c'");

    /* PRIMARY selection char is passed through */
    reset_hook();
    R(vt, "\033]52;p;T0s=\007");
    CHECK(vterm_clipboard_get(vt, &data, &len) == 1, "PRIMARY SET");
    CHECK(len == 2 && memcmp(data, "OK", 2) == 0, "PRIMARY decoded OK");
    CHECK(last_ev.selection == 'p', "selection should be 'p'");

    /* bad base64 leaves the previous payload alone */
    reset_hook();
    R(vt, "\033]52;c;????\007");
    CHECK(vterm_clipboard_get(vt, &data, &len) == 1,
        "bad base64 must not clear");
    CHECK(len == 2 && memcmp(data, "OK", 2) == 0,
        "bad base64 must keep OK");
    CHECK(hook_calls == 0, "bad base64 must not fire the hook");

    /* payload larger than the 128-byte CSI cap */
    memset(raw, 'A', sizeof(raw));
    b64_encode((unsigned char *)raw, sizeof(raw), encoded, sizeof(encoded));
    snprintf(seq, sizeof(seq), "\033]52;c;%s\007", encoded);
    CHECK(strlen(seq) > 128, "test payload must exceed ESEQ_BUF_SIZE");

    reset_hook();
    R(vt, seq);
    CHECK(vterm_clipboard_get(vt, &data, &len) == 1, "large SET stored");
    CHECK(len == sizeof(raw), "large SET len=%zu want %zu", len, sizeof(raw));
    CHECK(data != NULL && memcmp(data, raw, sizeof(raw)) == 0,
        "large SET payload mismatch");
    CHECK(hook_calls == 1, "large SET should fire the hook");

    /* leftover OSC 52 bytes must not spill as glyphs: cursor at home,
       first cell should still be the erase blank, not a base64 char.
       (a truncated-and-cancelled sequence used to print the tail.) */
    {
        int col = -1, row = -1;

        vterm_get_cursor_position(vt, &col, &row);
        CHECK(row == 0 && col == 0,
            "large OSC 52 must not advance the cursor (row=%d col=%d)",
            row, col);
    }

    /* poll API still works with the hook uninstalled */
    vterm_install_hook(vt, NULL);
    R(vt, "\033]52;c;Qg==\007");
    CHECK(vterm_clipboard_get(vt, &data, &len) == 1, "no-hook SET");
    CHECK(len == 1 && data[0] == 'B', "no-hook decoded B");

    vterm_clipboard_clear(vt);
    CHECK(vterm_clipboard_get(vt, &data, &len) == 0, "explicit clear");

    /* binary payload with an embedded NUL */
    {
        const unsigned char bin[] = { 'x', 0x00, 'y' };

        b64_encode(bin, sizeof(bin), encoded, sizeof(encoded));
        snprintf(seq, sizeof(seq), "\033]52;c;%s\007", encoded);
        R(vt, seq);
        CHECK(vterm_clipboard_get(vt, &data, &len) == 1, "NUL SET");
        CHECK(len == 3 && (unsigned char)data[0] == 'x' &&
            (unsigned char)data[1] == 0x00 &&
            (unsigned char)data[2] == 'y',
            "embedded NUL must survive decode");
    }

    vterm_destroy(vt);

    if(failures)
    {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }

    printf("test_osc52: ok\n");
    return 0;
}
