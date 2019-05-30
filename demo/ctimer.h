#ifndef _CTIMER_H_
#define _CTIMER_H_

#include <sys/time.h>

typedef struct _ctimer_s    ctimer_t;

ctimer_t*   ctimer_create(void);

void        ctimer_reset(ctimer_t *timer);

void        ctimer_elapsed(ctimer_t *timer, struct timeval *elapsed);

int         ctimer_compare(ctimer_t *timer, struct timeval *threshold);

void        ctimer_destroy(ctimer_t *timer);

#endif

