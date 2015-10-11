/******************************************************************
 *
 * @(#)std_port_unix.c:
 *
 * Purpose: 
 *  To support some intertask-synchronize function.
 *  The function for VM use only.
 *
 * Functions:
 *
 * History:
 *  2002.1.2        Initial version
 *                  doing
 *
 ***** Copyright 2001, doing reserved *****************************/

#include "std_port/std_port_platform.h"

#ifdef _UNIX
/* No MULTI_tHREAD */

#include "std_port/std_port.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

/* Internal routines */
static void std_sigAlrmHandler(int sig);

#ifdef ARCH_INTEL
#define DO_PREPARE \
    static unsigned long long cachedTSC = 0; \
\
    union \
    { \
        unsigned int r_eax_edx[2]; \
        unsigned long long currentTSC; \
    } i64;
#else
#define DO_PREPARE
#endif

/* Don't use push, pop here, since r_eax/r_edx is in
 * stack, the compiler may optimize it to [esp+offset].
 */
#ifdef __APPLE__
#define DO_RDTSC \
     asm volatile("rdtsc" : "=a" (i64.r_eax_edx[0]), "=d" (i64.r_eax_edx[1]));
#else
#define DO_RDTSC \
     asm volatile("rdtsc" : "=a" (i64.r_eax_edx[0]), "=d" (i64.r_eax_edx[1]));
#endif

/* Like time() */
extern std_time_t std_getOSTime()
{
    DO_PREPARE

    static time_t lastTime = 0;

#ifdef ARCH_INTEL
    DO_RDTSC;
    if ((i64.currentTSC - cachedTSC) < 10000000) // Too near 
        return (std_time_t) lastTime;

    /* Expired, update cache */
    cachedTSC = i64.currentTSC;
#endif

    time(&lastTime);
    return (std_time_t) lastTime;
}

/* Return ms counter */
extern std_tick_t std_getOSTick()
{
    DO_PREPARE

    static std_tick_t lastTick = 0;
    struct timeval tv;

#ifdef ARCH_INTEL
    DO_RDTSC;
    if ((i64.currentTSC - cachedTSC) < 1000000) // Too near 
        return lastTick;

    /* Expired, update cache */
    cachedTSC = i64.currentTSC;
#endif

    gettimeofday(&tv, NULL);
    lastTick = (std_tick_t) (tv.tv_sec * 1000 + tv.tv_usec / 1000);
    return lastTick;
}

/* Return us counter */
extern std_freq_t std_getOSUsCounter()
{
#ifdef IA_RDTSC
    /* Use RDTSC */
    static unsigned long long timeFreq = 0;
    union
    {
        unsigned long r_eax_edx[2];
        unsigned long long timestamp;
    } u_now, u_prev;
    static volatile unsigned long r_eax, r_edx;

    if (timeFreq == 0)
    {
        /* First invoking, stat for frequency */
        std_tick_t c_now, c_prev;

        /* Wait clock changed */
        c_prev = std_getOSTick();
        while (c_prev == std_getOSTick());
        c_prev = std_getOSTick();

        /* RDTSC - save time stamp counter */
        DO_RDTSC;

        /* Wait clock changed */
        u_prev.r_eax_edx[0] = r_eax;
        u_prev.r_eax_edx[1] = r_edx;

        /* At least 1 tick passed */
        while (c_prev + 10 > (c_now = std_getOSTick()));

        /* RDTSC - get new time stamp counter after sleep */
        DO_RDTSC;

        /* Get frequency per second */
        u_now.r_eax_edx[0] = r_eax;
        u_now.r_eax_edx[1] = r_edx;
        timeFreq = (u_now.timestamp - u_prev.timestamp) * 1000 / (c_now - c_prev);
    }

    /* Get current counter */
    DO_RDTSC;
    u_now.r_eax_edx[0] = r_eax;
    u_now.r_eax_edx[1] = r_edx;

    /* Convert to us, return in 32bits */
    return (std_freq_t) (u_now.timestamp * 1000000 / timeFreq);
#else

    DO_PREPARE

    /* BSD4.3 get time */
    static std_freq_t lastFreq;
    struct timeval  tv;

#ifdef ARCH_INTEL
    DO_RDTSC;
    if ((i64.currentTSC - cachedTSC) < 1000) // Too near 
        return lastFreq;

    /* Expired, update cache */
    cachedTSC = i64.currentTSC;
#endif

    gettimeofday(&tv, NULL);
    lastFreq = (std_freq_t) tv.tv_sec * 1000000 + tv.tv_usec;
    return lastFreq;
#endif
}

/* Sleep nms */
extern void std_sleep(int msec)
{
    struct timespec req, rem;

    req.tv_sec  = msec / 1000;
    req.tv_nsec = (msec % 1000) * 1000000;
    nanosleep(&req, &rem);
}

/* Fork process */
extern int std_fork()
{
    return fork();
}

/* Start/restart a timer
 * index: which timer (currently, only 0 is supoorted)
 * msec - millisecond
 * If a previous timer is existed, auto cleared */
/* Return 1 means OK, 0 means failed */
static int std_timerFuncInstalled = 0;
static int std_timerTimeoutFlag = 1; /* Default is timeout */
extern int std_start_timer(int index, int msec)
{
    struct itimerval val;
    if (! std_timerFuncInstalled)
    {
        signal(SIGALRM, std_sigAlrmHandler);
        std_timerFuncInstalled = 1;
    }

    if (index != 0)
        /* Bad index, failed */
        return 0;

    if (msec > STD_MAX_tIMER_tIMEOUT)
        /* Bad timeout time(too long), failed */
        return 0;

    /* Set current tick, reset timeout flag */
    val.it_interval.tv_sec = 0;
    val.it_interval.tv_usec = 0;
    val.it_value.tv_sec = msec / 1000;
    val.it_value.tv_usec = (msec % 1000) * 1000;
    setitimer(ITIMER_REAL, &val, NULL);

    /* Clear flag */
    std_timerTimeoutFlag = 0;
    return 1;
}

/* Is the timer timeout?
 * This function can be deteced more then once
 * see: _vmStartTimer */
/* Return 1 means timeout, 0 means no yet */
extern int std_is_timer_timeout(int index)
{
    if (index != 0)
        /* Bad timer, return timeout */
        return 1;

    return std_timerTimeoutFlag;
}

/* Stop the timer */
extern void std_clear_timer(int index)
{
    struct itimerval val;

    if (index != 0)
        /* Bad timer, return */
        return;

    /* Set timeout */
    std_timerTimeoutFlag = 1;

    /* Stop timer */
    memset(&val, 0, sizeof(val));
    setitimer(ITIMER_REAL, &val, NULL);
}

/* Get current working directory */
extern void std_get_cwd(char *path, size_t size)
{
    getcwd(path, size);
}

/* Get temporary directory */
extern const char *std_get_temp_dir()
{
    const char *p;

    if ((p = getenv("TMPDIR")) == NULL &&
        (p = getenv("TEMP")) == NULL &&
        (p = getenv("TMP")) == NULL)
        p = "/tmp/";

    return p;
}

/* Get error */
extern int std_get_error()
{
    return errno;
}

/* Exit current process */
extern void std_end_process(int r)
{
    printf("Exception on thread: %p\n", (void *) std_get_current_task_id());
    /* Raise exception to make core dump */
    __builtin_trap();

    exit(r);
}

/* Internal routine */
/* To receive alarm signal (for timeout), set timeout flag */
static void std_sigAlrmHandler(int sig)
{
    std_timerTimeoutFlag = 1;
}

#endif  /* End of _UNIX */
