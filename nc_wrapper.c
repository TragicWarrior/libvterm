#include <stdlib.h>
#include <dlfcn.h>

#ifdef __linux__
#include <ncursesw/curses.h>
#endif

#ifdef __FreeBSD__
#include <ncurses/ncurses.h>
#endif

#if defined(__APPLE__) && defined(__MACH__)
#include <ncursesw/ncurses.h>
#endif

#include "nc_wrapper.h"
#include "macros.h"

struct _nc_wrapper_s
{
    void    *dh;
    int     (*init_pair)        (int, int, int);
    int     (*init_color)       (int, int, int, int);
    int     (*pair_content)     (int, int *, int *);
    int     (*color_content)    (int, int *, int *, int *);
};

typedef struct _nc_wrapper_s    nc_wrapper_t;


nc_wrapper_t*   ncw_get_handle(void);


nc_wrapper_t *
ncw_get_handle(void)
{
    static nc_wrapper_t     *ncw = NULL;
    char                    *lib_path[] =
                                {
                                    "libncursesw.so.6.1",
                                    "libncursesw.so.6",
                                    "libncursesw.so.5",
                                };
    int                     array_sz = 0;
    int                     i;

    if(ncw != NULL) return ncw;

    ncw = CALLOC_PTR(ncw);

    array_sz = ARRAY_SZ(lib_path);

    for(i = 0; i < array_sz; i++)
    {
        ncw->dh = dlopen(lib_path[i], RTLD_NOW);
        if(ncw->dh != NULL) break;
    }

    // unable to load ncurses extensions
    if(ncw->dh == NULL) return ncw;

    // clear any vestigial errors
    dlerror();

    *(void **)(&ncw->init_pair) = dlsym(ncw->dh, "init_extended_pair");
    // ncw->init_pair = dlsym(ncw->dh, "init_extended_pair");
    if(dlerror() != NULL) ncw->init_pair = NULL;

    *(void **)(&ncw->init_color) = dlsym(ncw->dh, "init_extended_color");
    if(dlerror() != NULL) ncw->init_color = NULL;

    *(void **)(&ncw->pair_content) = dlsym(ncw->dh, "extended_pair_content");
    if(dlerror() != NULL) ncw->pair_content = NULL;

    *(void **)(&ncw->color_content) = dlsym(ncw->dh, "extended_color_content");
    if(dlerror() != NULL) ncw->color_content = NULL;

    return ncw;
}

int
ncw_init_pair(int pair, int f, int b)
{
    nc_wrapper_t    *ncw;
    int             retval;

    ncw = ncw_get_handle();

    if(ncw->init_pair == NULL)
    {
        retval = init_pair(pair, (short)f, (short)b);
        return retval;
    }

    retval = ncw->init_pair(pair, f, b);

    return retval;
}

int
ncw_init_color(int color, int r, int g, int b)
{
    nc_wrapper_t    *ncw;
    int             retval;

    ncw = ncw_get_handle();

    if(ncw->init_color == NULL)
    {
        retval = init_color(color, r, g, b);
        return retval;
    }

    retval = ncw->init_color(color, r, g, b);

    return retval;
}

int
ncw_pair_content(int pair, int *f, int *b)
{
    nc_wrapper_t    *ncw;
    int             retval;

    ncw = ncw_get_handle();

#ifndef __FreeBSD__
    if(ncw->pair_content == NULL)
#endif
    {
        retval = pair_content((short)pair, (short *)f, (short *)b);

        return retval;
    }

    retval = ncw->pair_content(pair, f, b);

    return retval;
}

int
ncw_color_content(int color, int *r, int *g, int *b)
{
    nc_wrapper_t    *ncw;
    short           sred;
    short           sgreen;
    short           sblue;
    int             retval;

    ncw = ncw_get_handle();

#ifndef __FreeBSD__
    if(ncw->color_content == NULL)
#endif
    {
        retval = color_content((short)color, &sred, &sgreen, &sblue);

        *r = sred;
        *g = sgreen;
        *b = sblue;

        return retval;
    }

    retval = ncw->color_content(color, r, g, b);

    return retval;
}
