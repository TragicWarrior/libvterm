/*
    Deterministic resize / scroll / copy stress harness for libvterm.

    Built to corner the failure mode that killed the May 2026 contiguous-
    buffer attempt: a hang on first resize that could not be reproduced
    from the CLI.  This drives vterm_resize across extreme sizes, the
    max_cols width ratchet, scrolling, DECSTBM regions, and
    vterm_copy_buffer -- all headless (NOPTY|NOCURSES), fully
    deterministic (fixed-seed LCG, no rand()/time), under an alarm()
    watchdog so a hang aborts loudly instead of spinning forever.

    Build (plain):
      gcc -O0 -g -I. test/test_resize.c <lib .c files> -o /tmp/trez \
          -lncursesw -lutil -lm -lrt
    Build (ASan):
      gcc -fsanitize=address -O1 -g -I. test/test_resize.c <lib .c> ...
*/

#ifndef _XOPEN_SOURCE_EXTENDED
#define _XOPEN_SOURCE_EXTENDED
#endif
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "vterm.h"

static unsigned long lcg = 0x9e3779b9UL;

static unsigned
nrand(void)
{
    lcg = lcg * 1103515245UL + 12345UL;
    return (unsigned)((lcg >> 16) & 0x7fff);
}

static volatile sig_atomic_t g_iter = 0;

static void
on_alarm(int sig)
{
    (void)sig;
    /* async-signal-safe-ish: write a fixed message + the iteration */
    char msg[64];
    int  n = snprintf(msg, sizeof(msg),
        "\nHANG DETECTED at iteration %d\n", (int)g_iter);
    if(n > 0) { ssize_t w = write(2, msg, (size_t)n); (void)w; }
    _exit(99);
}

static void
feed(vterm_t *vt, const char *s)
{
    vterm_render(vt, (char *)s, (int)strlen(s));
}

static void
feed_content(vterm_t *vt, int lines)
{
    int i;
    char buf[128];

    for(i = 0; i < lines; i++)
    {
        /* vary SGR color so the color cache + per-cell colors churn */
        int fg = 30 + (int)(nrand() % 8);
        int bg = 40 + (int)(nrand() % 8);
        snprintf(buf, sizeof(buf),
            "\033[%d;%dmline %d the quick brown fox 0123456789\033[0m\r\n",
            fg, bg, i);
        feed(vt, buf);
    }
}

static void
exercise_decstbm(vterm_t *vt, int height)
{
    char buf[32];
    int  top, bot;

    if(height < 4) return;

    top = 1 + (int)(nrand() % (unsigned)(height / 2));
    bot = top + 1 + (int)(nrand() % (unsigned)(height - top));
    if(bot > height) bot = height;

    snprintf(buf, sizeof(buf), "\033[%d;%dr", top, bot);
    feed(vt, buf);

    /* scroll inside the region */
    feed_content(vt, height + 3);

    /* reset region to full screen */
    feed(vt, "\033[r");
}

static void
exercise_copy(vterm_t *vt)
{
    int rows = 0, cols = 0;
    int r;
    vterm_cell_t **cells;

    cells = vterm_copy_buffer(vt, &rows, &cols);
    if(cells == NULL) return;

    /* touch every returned cell so ASan flags any under-allocation */
    {
        volatile wchar_t sink = 0;
        for(r = 0; r < rows; r++)
        {
            int c;
            for(c = 0; c < cols; c++) sink ^= cells[r][c].wch[0];
        }
        (void)sink;
    }

    /* free exactly the way vwm's vwmterm_copy_selection does */
    for(r = 0; r < rows; r++) free(cells[r]);
    free(cells);
}

int
main(int argc, char **argv)
{
    vterm_t *vt;
    int      i;
    const int ITERS = 8000;

    /* optional seed arg lets a caller sweep many size sequences */
    if(argc > 1) lcg = strtoul(argv[1], NULL, 0);

    signal(SIGALRM, on_alarm);
    alarm(30);                  /* a correct run finishes in well under 1s */

    vt = vterm_create(80, 24, VTERM_FLAG_NOPTY | VTERM_FLAG_NOCURSES);
    if(vt == NULL)
    {
        fprintf(stderr, "vterm_create failed\n");
        return 1;
    }

    feed_content(vt, 60);       /* fill + scroll into history */

    for(i = 0; i < ITERS; i++)
    {
        int w, h;
        int pick;

        g_iter = i;

        /* size mix: extremes + mid-range, both grow and shrink so the
           max_cols ratchet and the shrink-free path both fire */
        pick = (int)(nrand() % 10);
        switch(pick)
        {
            case 0:  w = 1;   h = 1;   break;   /* degenerate */
            case 1:  w = 2;   h = 2;   break;
            case 2:  w = 200; h = 80;  break;   /* large (ratchets width) */
            case 3:  w = 250; h = 20;  break;   /* very wide, short */
            case 4:  w = 40;  h = 120; break;   /* narrow, tall */
            default:
                w = 2 + (int)(nrand() % 198);
                h = 2 + (int)(nrand() % 78);
                break;
        }

        vterm_resize(vt, w, h);

        /* feed content sized to the new geometry to force scrolling */
        feed_content(vt, h / 2 + 2);

        if((i & 3) == 0)  exercise_decstbm(vt, h);
        if((i & 7) == 0)  exercise_copy(vt);
    }

    exercise_copy(vt);
    vterm_destroy(vt);

    alarm(0);
    printf("OK: %d iterations completed\n", ITERS);
    return 0;
}
