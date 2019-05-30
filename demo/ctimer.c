#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../macros.h"
#include "ctimer.h"

struct _ctimer_s
{
    struct timeval  epoch;
};

ctimer_t*
ctimer_create(void)
{
    ctimer_t            *ctimer;
    struct timespec     epoch;

    CALLOC_PTR(ctimer);

    clock_gettime(CLOCK_MONOTONIC, &epoch);

    // 1 microsecond equals 1000 nanoseconds
    ctimer->epoch.tv_sec = epoch.tv_sec;
    ctimer->epoch.tv_usec = epoch.tv_nsec / 1000;

    return ctimer;
}

void
ctimer_reset(ctimer_t *ctimer)
{
    struct timespec     epoch;

    if(ctimer == NULL) return;

    clock_gettime(CLOCK_MONOTONIC, &epoch);

    // 1 microsecond equals 1000 nanoseconds
    ctimer->epoch.tv_sec = epoch.tv_sec;
    ctimer->epoch.tv_usec = epoch.tv_nsec / 1000;

    return;
}

void
ctimer_elapsed(ctimer_t *ctimer, struct timeval *elapsed)
{
    struct timespec     now_ts;
    struct timeval      now_tv;

    clock_gettime(CLOCK_MONOTONIC, &now_ts);
    memset(elapsed, 0, sizeof(struct timeval));

    // convert timespec to timeval
    now_tv.tv_sec = now_ts.tv_sec;
    now_tv.tv_usec = now_ts.tv_nsec / 1000;

    elapsed->tv_sec = now_tv.tv_sec - ctimer->epoch.tv_sec;
    elapsed->tv_usec = now_tv.tv_usec - ctimer->epoch.tv_usec;
    if(elapsed->tv_usec < 0)
    {
     	elapsed->tv_sec--;
        elapsed->tv_usec += 1000000;
    }

    return;
}

int
ctimer_compare(ctimer_t *ctimer, struct timeval *threshold)
{
    struct timeval	elapsed;

    ctimer_elapsed(ctimer, &elapsed);

    if(elapsed.tv_sec > threshold->tv_sec) return 1;

    if(elapsed.tv_sec == threshold->tv_sec)
    {
        if(elapsed.tv_usec > threshold->tv_usec) return 1;

        if(elapsed.tv_usec == threshold->tv_usec) return 0;
    }

    return -1;
}

void
ctimer_destroy(ctimer_t *ctimer)
{
    if(ctimer == NULL) return;

    free(ctimer);
}

