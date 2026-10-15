/*
    Characters for the Linux console.

    The console draws text from a single kernel font of at most 512
    glyphs.  A character the font lacks shows as the font's fallback
    glyph, so a modern program's prompt arrows, status dots, spinners and
    braille art turn into a row of boxes.  Here such a character is swapped,
    at draw time only, for the nearest one the console can show.  The
    cell itself keeps what the child sent.

    Each entry lists up to three stand-ins, best first; the last is
    always plain ASCII, which every font has.  When the host has told
    us which characters its font holds (vtty_set_host_glyphs), a
    character the font has is left alone and the first stand-in it has
    is used.  Without that list, a character in the table is always
    replaced, and a stand-in missing from some of the stock console
    fonts is passed over for the next one.
*/

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "vtty.h"
#include "glyph_fit.h"

#define GLYPH_CANDIDATES    3

typedef struct
{
    wchar_t     from;
    wchar_t     to[GLYPH_CANDIDATES];       // best first, 0 ends the list
}
glyph_sub_t;

/*
    keep this sorted by .from: it is searched with bsearch().  Only
    single-width characters belong here -- swapping a double-width one
    for a single-width one would shift the rest of the line.
*/
static const glyph_sub_t glyph_subs[] =
{
    { 0x203B,   { L'*' } },                         // ※
    { 0x2055,   { L'*' } },                         // ⁕
    { 0x2139,   { L'i' } },                         // ℹ
    { 0x2196,   { 0x2572, L'\\' } },                // ↖  ╲
    { 0x2197,   { 0x2571, L'/' } },                 // ↗  ╱
    { 0x2198,   { 0x2572, L'\\' } },                // ↘  ╲
    { 0x2199,   { 0x2571, L'/' } },                 // ↙  ╱
    { 0x21A9,   { 0x21B5, L'<' } },                 // ↩  ↵
    { 0x21AA,   { 0x2192, L'>' } },                 // ↪  →
    { 0x21BA,   { L'@' } },                         // ↺
    { 0x21BB,   { L'@' } },                         // ↻
    { 0x21C4,   { 0x2194, L'-' } },                 // ⇄  ↔
    { 0x21C5,   { 0x2195, L'|' } },                 // ⇅  ↕
    { 0x21E7,   { 0x21D1, 0x2191, L'^' } },         // ⇧  ⇑ ↑
    { 0x22EE,   { L':' } },                         // ⋮
    { 0x22EF,   { 0x2026, L'.' } },                 // ⋯  …
    { 0x2318,   { L'#' } },                         // ⌘
    { 0x2325,   { L'+' } },                         // ⌥
    { 0x2326,   { 0x2192, L'>' } },                 // ⌦  →
    { 0x232B,   { 0x2190, L'<' } },                 // ⌫  ←
    { 0x2398,   { 0x25A0, L'#' } },                 // ⎘  ■
    { 0x23BE,   { 0x250C, L'+' } },                 // ⎾  ┌
    { 0x23BF,   { 0x2514, L'+' } },                 // ⎿  └
    { 0x23CE,   { 0x21B5, L'<' } },                 // ⏎  ↵
    { 0x23CF,   { 0x25B2, L'^' } },                 // ⏏  ▲
    { 0x23D0,   { 0x2502, L'|' } },                 // ⏐  │
    { 0x23F4,   { 0x25C0, L'<' } },                 // ⏴  ◀
    { 0x23F5,   { 0x25B6, L'>' } },                 // ⏵  ▶
    { 0x23F6,   { 0x25B2, L'^' } },                 // ⏶  ▲
    { 0x23F7,   { 0x25BC, L'v' } },                 // ⏷  ▼
    { 0x23F8,   { 0x2016, L'|' } },                 // ⏸  ‖
    { 0x23F9,   { 0x25A0, L'#' } },                 // ⏹  ■
    { 0x23FA,   { 0x25CF, L'*' } },                 // ⏺  ●
    { 0x2580,   { 0x2592, L'#' } },                 // ▀  ▒
    { 0x2581,   { L'_' } },                         // ▁
    { 0x2582,   { L'_' } },                         // ▂
    { 0x2583,   { 0x2592, L'#' } },                 // ▃  ▒
    { 0x2584,   { 0x2592, L'#' } },                 // ▄  ▒
    { 0x2585,   { 0x2592, L'#' } },                 // ▅  ▒
    { 0x2586,   { 0x2588, L'#' } },                 // ▆  █
    { 0x2587,   { 0x2588, L'#' } },                 // ▇  █
    { 0x2589,   { 0x2588, L'#' } },                 // ▉  █
    { 0x258A,   { 0x2588, L'#' } },                 // ▊  █
    { 0x258B,   { 0x2592, L'#' } },                 // ▋  ▒
    { 0x258C,   { 0x2592, L'#' } },                 // ▌  ▒
    { 0x258D,   { 0x2592, L'#' } },                 // ▍  ▒
    { 0x258E,   { 0x2502, L'|' } },                 // ▎  │
    { 0x258F,   { 0x2502, L'|' } },                 // ▏  │
    { 0x2590,   { 0x2592, L'#' } },                 // ▐  ▒
    { 0x2593,   { 0x2588, L'#' } },                 // ▓  █
    { 0x2594,   { L'-' } },                         // ▔
    { 0x2595,   { 0x2502, L'|' } },                 // ▕  │
    { 0x2596,   { L'.' } },                         // ▖
    { 0x2597,   { L'.' } },                         // ▗
    { 0x2598,   { L'\'' } },                        // ▘
    { 0x2599,   { 0x2592, L'#' } },                 // ▙  ▒
    { 0x259A,   { 0x2592, L'#' } },                 // ▚  ▒
    { 0x259B,   { 0x2592, L'#' } },                 // ▛  ▒
    { 0x259C,   { 0x2592, L'#' } },                 // ▜  ▒
    { 0x259D,   { L'\'' } },                        // ▝
    { 0x259E,   { 0x2592, L'#' } },                 // ▞  ▒
    { 0x259F,   { 0x2592, L'#' } },                 // ▟  ▒
    { 0x25A1,   { 0x2610, L'#' } },                 // □  ☐
    { 0x25AA,   { 0x25A0, L'#' } },                 // ▪  ■
    { 0x25AB,   { 0x2610, L'.' } },                 // ▫  ☐
    { 0x25B3,   { 0x25B2, L'^' } },                 // △  ▲
    { 0x25B5,   { 0x25B4, L'^' } },                 // ▵  ▴
    { 0x25B7,   { 0x25B6, L'>' } },                 // ▷  ▶
    { 0x25B9,   { 0x25B8, L'>' } },                 // ▹  ▸
    { 0x25BA,   { 0x25B6, L'>' } },                 // ►  ▶
    { 0x25BB,   { 0x25B6, L'>' } },                 // ▻  ▶
    { 0x25BD,   { 0x25BC, L'v' } },                 // ▽  ▼
    { 0x25BF,   { 0x25BE, L'v' } },                 // ▿  ▾
    { 0x25C1,   { 0x25C0, L'<' } },                 // ◁  ◀
    { 0x25C3,   { 0x25C2, L'<' } },                 // ◃  ◂
    { 0x25C4,   { 0x25C0, L'<' } },                 // ◄  ◀
    { 0x25C5,   { 0x25C0, L'<' } },                 // ◅  ◀
    { 0x25C6,   { 0x2666, L'*' } },                 // ◆  ♦
    { 0x25C7,   { 0x25C8, L'o' } },                 // ◇  ◈
    { 0x25C9,   { 0x25CF, L'*' } },                 // ◉  ●
    { 0x25CB,   { L'o' } },                         // ○
    { 0x25CC,   { L'o' } },                         // ◌
    { 0x25CE,   { L'o' } },                         // ◎
    { 0x25E6,   { 0x2219, L'.' } },                 // ◦  ∙
    { 0x25EF,   { L'o' } },                         // ◯
    { 0x2605,   { L'*' } },                         // ★
    { 0x2606,   { L'*' } },                         // ☆
    { 0x26A0,   { L'!' } },                         // ⚠
    { 0x270E,   { L'/' } },                         // ✎
    { 0x270F,   { L'/' } },                         // ✏
    { 0x2713,   { 0x221A, L'v' } },                 // ✓  √
    { 0x2714,   { 0x221A, L'v' } },                 // ✔  √
    { 0x2715,   { 0x00D7, L'x' } },                 // ✕  ×
    { 0x2716,   { 0x2718, L'x' } },                 // ✖  ✘
    { 0x2722,   { L'+' } },                         // ✢
    { 0x2726,   { L'*' } },                         // ✦
    { 0x2727,   { L'*' } },                         // ✧
    { 0x2733,   { L'*' } },                         // ✳
    { 0x2736,   { L'*' } },                         // ✶
    { 0x273B,   { L'*' } },                         // ✻
    { 0x273D,   { L'*' } },                         // ✽
    { 0x274B,   { L'*' } },                         // ❋
    { 0x276E,   { 0x2039, L'<' } },                 // ❮  ‹
    { 0x276F,   { 0x203A, L'>' } },                 // ❯  ›
    { 0x2794,   { 0x2192, L'>' } },                 // ➔  →
    { 0x279C,   { 0x2192, L'>' } },                 // ➜  →
    { 0x27A4,   { 0x2192, L'>' } },                 // ➤  →
    { 0x27F2,   { L'@' } },                         // ⟲
    { 0x27F3,   { L'@' } },                         // ⟳
    { 0x2981,   { 0x2219, L'.' } },                 // ⦁  ∙
    { 0x29C9,   { 0x25A0, L'#' } },                 // ⧉  ■
    { 0x2B21,   { L'o' } },                         // ⬡
    { 0x2B22,   { 0x25CF, L'*' } },                 // ⬢  ●
};

#define GLYPH_SUB_COUNT     (sizeof(glyph_subs) / sizeof(glyph_subs[0]))

/*
    stand-ins that some stock console fonts lack (the DejaVu, VGA and
    Fixed sets each miss a few).  Without the host's own list they are
    passed over for the next stand-in.  Keep sorted.
*/
static const wchar_t glyph_unsure[] =
{
    0x2016,     // ‖
    0x2026,     // …
    0x2194,     // ↔
    0x2195,     // ↕
    0x21B5,     // ↵
    0x21D1,     // ⇑
    0x2219,     // ∙
    0x221A,     // √
};

#define GLYPH_UNSURE_COUNT  (sizeof(glyph_unsure) / sizeof(glyph_unsure[0]))

// the characters the host's font holds, sorted; NULL when not known
static wchar_t      *host_glyphs = NULL;
static size_t       host_glyph_count = 0;

/* bsearch() comparison for a wchar_t key against a wchar_t element */
static int
_glyph_cmp_wchar(const void *key, const void *elem)
{
    wchar_t a = *(const wchar_t *)key;
    wchar_t b = *(const wchar_t *)elem;

    return (a > b) - (a < b);
}

/* bsearch() comparison for a wchar_t key against a table entry */
static int
_glyph_cmp_sub(const void *key, const void *elem)
{
    wchar_t a = *(const wchar_t *)key;
    wchar_t b = ((const glyph_sub_t *)elem)->from;

    return (a > b) - (a < b);
}

/*
    whether the console can be trusted to draw wch: anything in the
    host's list when there is one, otherwise anything not known to be
    missing from some stock font.  ASCII is always drawable.
*/
static bool
_glyph_drawable(wchar_t wch)
{
    if(wch < 0x80) return true;

    if(host_glyphs != NULL)
    {
        return bsearch(&wch, host_glyphs, host_glyph_count,
            sizeof(wchar_t), _glyph_cmp_wchar) != NULL;
    }

    return bsearch(&wch, glyph_unsure, GLYPH_UNSURE_COUNT,
        sizeof(wchar_t), _glyph_cmp_wchar) == NULL;
}

/*
    braille is a 2 x 4 grid of dots, used for spinners and for pictures
    such as logos.  Keep the picture's shape: a sparse cell becomes a
    mark at the height its dots sit, a fuller one a shade as dense.
*/
static wchar_t
_glyph_fit_braille(wchar_t wch)
{
    unsigned int    dots = (unsigned int)(wch - 0x2800);
    unsigned int    top = dots & ((1 << 0) | (1 << 3));      // dots 1, 4
    unsigned int    upper = dots & ((1 << 1) | (1 << 4));    // dots 2, 5
    unsigned int    lower = dots & ((1 << 2) | (1 << 5));    // dots 3, 6
    unsigned int    bottom = dots & ((1 << 6) | (1 << 7));   // dots 7, 8
    int             count = 0;
    unsigned int    d;

    // how many of the eight dots are raised
    for(d = dots; d != 0; d >>= 1) count += (d & 1);

    if(count == 0) return L' ';

    // a light cell: a mark where its dots sit
    if(count <= 3)
    {
        if((lower | bottom) == 0) return L'\'';
        if((top | upper) == 0) return L'.';
        return L':';
    }

    // a heavy cell: a shade of matching weight
    if(count <= 5) return _glyph_drawable(0x2591) ? 0x2591 : L':';  // ░
    if(count <= 7) return _glyph_drawable(0x2592) ? 0x2592 : L'#';  // ▒
    return _glyph_drawable(0x2588) ? 0x2588 : L'#';                  // █
}

bool
vtty_glyph_fit_wanted(void)
{
#ifndef NOCURSES
    const char  *name = termname();

    // linux, linux-16color, linux-c, ... are all the console
    return (name != NULL && strncmp(name, "linux", 5) == 0);
#else
    return false;
#endif
}

wchar_t
vtty_fit_glyph(wchar_t wch)
{
    const glyph_sub_t   *sub;
    int                 i;

    // nothing in the table is below U+2000: keep ASCII and Latin fast
    if(wch < 0x2000) return wch;

    // the font has it: nothing to do
    if(host_glyphs != NULL && _glyph_drawable(wch)) return wch;

    // the circled and dingbat digits one to nine
    if(wch >= 0x2460 && wch <= 0x2468) return L'1' + (wch - 0x2460);  // ①
    if(wch >= 0x24F5 && wch <= 0x24FD) return L'1' + (wch - 0x24F5);  // ⓵
    if(wch >= 0x2776 && wch <= 0x277E) return L'1' + (wch - 0x2776);  // ❶

    if(wch >= 0x2800 && wch <= 0x28FF) return _glyph_fit_braille(wch);

    sub = bsearch(&wch, glyph_subs, GLYPH_SUB_COUNT, sizeof(glyph_sub_t),
        _glyph_cmp_sub);
    if(sub == NULL) return wch;

    // the first stand-in the console can draw; the last one always is
    for(i = 0; i < GLYPH_CANDIDATES && sub->to[i] != 0; i++)
    {
        if(_glyph_drawable(sub->to[i])) return sub->to[i];
    }

    return wch;
}

int
vtty_set_host_glyphs(const wchar_t *glyphs, int count)
{
    wchar_t     *copy;

    // forget the old list; NULL or 0 goes back to the built-in rules
    free(host_glyphs);
    host_glyphs = NULL;
    host_glyph_count = 0;

    if(glyphs == NULL || count <= 0) return 0;

    copy = malloc(sizeof(wchar_t) * (size_t)count);
    if(copy == NULL) return -1;

    memcpy(copy, glyphs, sizeof(wchar_t) * (size_t)count);
    qsort(copy, (size_t)count, sizeof(wchar_t), _glyph_cmp_wchar);

    host_glyphs = copy;
    host_glyph_count = (size_t)count;

    return 0;
}
