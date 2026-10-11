/*
    Window title tests: OSC 0 / 1 / 2 and VTTY_EVENT_TITLE (headless,
    NOPTY | NOCURSES).

    Pins:
      - OSC 0 and OSC 2 set the title, with BEL and with ST (ESC \)
      - OSC 1 (icon name) does not change the title
      - VTTY_EVENT_TITLE carries the new title, fires once per change,
        does not fire for an unchanged title, and does not fire when
        the mask bit is clear
      - a second title replaces the first
      - an empty title is stored as "" and announced
      - multi-byte UTF-8 survives, including a character holding the
        byte 0x9C (U+2733), which is not a terminator in UTF-8 mode
      - in 8-bit mode 0x9C does terminate the string
      - an over-long title is cut at a character boundary
      - an OSC with no string, or with no number, changes nothing
      - OSC 4 (colour) and OSC 52 (clipboard), which share the parser,
        still work

    Build: cmake -DBUILD_TESTS=ON . && make test_osc_title
*/

#ifndef _XOPEN_SOURCE_EXTENDED
#define _XOPEN_SOURCE_EXTENDED
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vtty.h"

static int  failures = 0;
static int  hook_calls = 0;
static char last_title[256];

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

/* render a NUL-terminated escape string */
static void
R(vtty_t *vt, const char *s)
{
    vtty_render(vt, (char *)s, (int)strlen(s));
}

/* the event hook: count title events and keep the last payload */
static void
on_event(vtty_t *vtty, int event, void *anything)
{
    (void)vtty;

    if(event != VTTY_EVENT_TITLE) return;

    hook_calls++;
    snprintf(last_title, sizeof(last_title), "%s", (const char *)anything);
}

/* forget what the hook has seen so far */
static void
reset_hook(void)
{
    hook_calls = 0;
    memset(last_title, 0, sizeof(last_title));
}

/* the current title, as vtty_get_title() reports it */
static const char *
title(vtty_t *vt)
{
    static char     buf[256];

    memset(buf, 'x', sizeof(buf));
    vtty_get_title(vt, buf, sizeof(buf));

    return buf;
}

/*
    send one sequence and check the title, how many events it caused,
    and (when it caused one) the payload.
*/
static void
expect(vtty_t *vt, const char *name, const char *seq, const char *want,
    int want_calls)
{
    reset_hook();
    R(vt, seq);

    CHECK(strcmp(title(vt), want) == 0, "%s: title [%s] want [%s]",
        name, title(vt), want);
    CHECK(hook_calls == want_calls, "%s: %d event(s), want %d",
        name, hook_calls, want_calls);

    if(want_calls > 0 && hook_calls > 0)
    {
        CHECK(strcmp(last_title, want) == 0, "%s: payload [%s] want [%s]",
            name, last_title, want);
    }
}

int
main(void)
{
    vtty_t          *vt;
    const char      *clip;
    size_t          clip_len;
    char            seq[512];
    char            want[256];
    int             i;

    vt = vtty_create(80, 24, VTTY_FLAG_NOPTY | VTTY_FLAG_NOCURSES);
    if(vt == NULL)
    {
        fprintf(stderr, "vtty_create failed\n");
        return 1;
    }

    vtty_install_hook(vt, on_event);
    vtty_set_event_mask(vt, VTTY_MASK_TITLE);

    CHECK(strcmp(title(vt), "") == 0, "no title before one is set");

    /* ---- OSC 0 and 2, both terminators ---- */
    expect(vt, "osc 0 bel", "\033]0;zero-bel\007", "zero-bel", 1);
    expect(vt, "osc 0 st", "\033]0;zero-st\033\\", "zero-st", 1);
    expect(vt, "osc 2 bel", "\033]2;two-bel\007", "two-bel", 1);
    expect(vt, "osc 2 st", "\033]2;two-st\033\\", "two-st", 1);

    /* ---- OSC 1 is the icon name, not the title ---- */
    expect(vt, "osc 1 bel", "\033]1;icon-bel\007", "two-st", 0);
    expect(vt, "osc 1 st", "\033]1;icon-st\033\\", "two-st", 0);

    /* ---- a second title replaces the first; the same one is quiet ---- */
    expect(vt, "replace", "\033]2;second\007", "second", 1);
    expect(vt, "unchanged", "\033]2;second\007", "second", 0);
    expect(vt, "unchanged via osc 0", "\033]0;second\007", "second", 0);

    /* ---- a title with a semicolon in it ---- */
    expect(vt, "semicolon", "\033]2;a;b\007", "a;b", 1);

    /* ---- an empty title ---- */
    expect(vt, "empty", "\033]2;\007", "", 1);
    expect(vt, "empty again", "\033]0;\007", "", 0);

    /* ---- nothing after the number, and no number at all ---- */
    R(vt, "\033]2;keep\007");
    expect(vt, "no string", "\033]2\007", "keep", 0);
    expect(vt, "no number", "\033];oops\007", "keep", 0);
    expect(vt, "letter", "\033]Zoops\007", "keep", 0);

    /* ---- multi-byte UTF-8 ---- */
    expect(vt, "utf8 e-acute", "\033]2;caf\xc3\xa9\007", "caf\xc3\xa9", 1);
    expect(vt, "utf8 cjk", "\033]2;\xe4\xb8\x80x\007", "\xe4\xb8\x80x", 1);

    /* U+2733 is E2 9C B3:  0x9C must not end the string here */
    expect(vt, "utf8 holding 0x9c", "\033]0;\xe2\x9c\xb3 Claude\007",
        "\xe2\x9c\xb3 Claude", 1);

    /* ---- an over-long title:  cut, and on a character boundary ---- */
    memset(seq, 0, sizeof(seq));
    memset(want, 0, sizeof(want));
    strcpy(seq, "\033]2;");
    for(i = 0; i < 300; i++) strcat(seq, "a");
    strcat(seq, "\007");
    memset(want, 'a', 127);
    expect(vt, "long ascii", seq, want, 1);

    /* 126 bytes, then a 3-byte character that cannot fit in the 127th */
    memset(seq, 0, sizeof(seq));
    memset(want, 0, sizeof(want));
    strcpy(seq, "\033]2;");
    for(i = 0; i < 126; i++) strcat(seq, "b");
    strcat(seq, "\xe2\x82\xac\xe2\x82\xac\007");
    memset(want, 'b', 126);
    expect(vt, "long, cut inside a character", seq, want, 1);

    /* 124 bytes, then a 3-byte character that fits exactly */
    memset(seq, 0, sizeof(seq));
    memset(want, 0, sizeof(want));
    strcpy(seq, "\033]2;");
    for(i = 0; i < 124; i++) strcat(seq, "c");
    strcat(seq, "\xe2\x82\xac\xe2\x82\xac\007");
    memset(want, 'c', 124);
    strcat(want, "\xe2\x82\xac");
    expect(vt, "long, whole character kept", seq, want, 1);

    /* ---- the mask bit decides whether the host is told ---- */
    vtty_set_event_mask(vt, 0);
    expect(vt, "mask clear", "\033]2;quiet\007", "quiet", 0);
    vtty_set_event_mask(vt, VTTY_MASK_TITLE);

    /* ---- the parser's other functions still work ---- */
    R(vt, "\033]2;before-others\007");

    /* OSC 4 (colour), well formed and with nothing after the number */
    expect(vt, "osc 4", "\033]4;1;rgb:ff/00/00\007", "before-others", 0);
    expect(vt, "osc 4, no string", "\033]4\007", "before-others", 0);

    /* OSC 52 (clipboard):  "hello" */
    expect(vt, "osc 52", "\033]52;c;aGVsbG8=\007", "before-others", 0);
    clip = NULL;
    clip_len = 0;
    CHECK(vtty_clipboard_get(vt, &clip, &clip_len) == 1,
        "osc 52: payload stored");
    CHECK(clip_len == 5 && clip != NULL && memcmp(clip, "hello", 5) == 0,
        "osc 52: payload is hello");

    /* text after an OSC still lands on the screen, not in the title */
    expect(vt, "trailing text", "\033]2;with-text\007visible", "with-text", 1);

    vtty_install_hook(vt, NULL);
    vtty_destroy(vt);

    /* ---- 8-bit mode:  0x9C is the string terminator ---- */
    vt = vtty_create(80, 24,
        VTTY_FLAG_NOPTY | VTTY_FLAG_NOCURSES | VTTY_FLAG_NOUTF8);
    if(vt == NULL)
    {
        fprintf(stderr, "vtty_create (8-bit) failed\n");
        return 1;
    }

    vtty_install_hook(vt, on_event);
    vtty_set_event_mask(vt, VTTY_MASK_TITLE);

    expect(vt, "8-bit, 0x9c terminates", "\033]2;eight\x9c", "eight", 1);

    vtty_install_hook(vt, NULL);
    vtty_destroy(vt);

    if(failures > 0)
    {
        fprintf(stderr, "FAILED: %d check(s)\n", failures);
        return 1;
    }

    printf("OK: osc title test completed\n");
    return 0;
}
