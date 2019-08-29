#ifndef _MOUSE_DRIVER_H_
#define _MOUSE_DRIVER_H_

#include <signal.h>

#ifdef __linux__
#include <ncursesw/curses.h>
#endif

#ifdef __FreeBSD__
#include <ncurses.h>
#endif

#if defined(__APPLE__) && defined(__MACH__)
// #include <ncurses.h>
#include <ncursesw/ncurses.h>
#endif

#include "vterm.h"
#include "vterm_private.h"

typedef struct _mouse_state_s   mouse_state_t;

struct _mouse_state_s
{
    vterm_t         *origin;
    sig_atomic_t    ref_count;
};

typedef struct _mouse_config_s  mouse_config_t;

struct _mouse_config_s
{
    mmask_t         mouse_mask;                     //  previous mouse mask
    int             mouse_interval;                 //  previous mouse interval
    unsigned int    state;
};

#define MOUSE_STATE_RUNNING     (1 << 0)


void    mouse_driver_init(vterm_t *vterm);

void    mouse_driver_start(vterm_t *vterm);

void    mouse_driver_stop(vterm_t *vterm);

void    mouse_driver_free(vterm_t *vterm);

ssize_t mouse_driver_default(vterm_t *vterm, unsigned char *buf);

void    mouse_driver_save_state(vterm_t *vterm);

void    mouse_driver_restore_state(vterm_t *vterm);

ssize_t mouse_driver_vt200(vterm_t *vterm, unsigned char *buf);

ssize_t mouse_driver_SGR(vterm_t *vterm, unsigned char *buf);


#endif
