/******************************************************************
 *
 * @(#)std_port_posix.c:
 *
 * Purpose: 
 *  To support some interproc-synchronize function.
 *
 * Functions:
 *
 * History:
 *  2002.1.2        Initial version
 *                  doing
 *
 ***** Copyright 2001, doing reserved *****************************/

#include "std_port/std_port_platform.h"

#ifdef _POSIX

#include "std_port/std_port.h"
#include "std_port/std_port_cs.h"
#include "std_port/std_port_internal.h"

#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__APPLE_CC__)
#define PTHREAD_MUTEX_RECURSIVE_NP PTHREAD_MUTEX_RECURSIVE
#endif

#ifndef STD_NO_MULTI_tHREAD

extern int std_create_mutex(std_mutex_id_t *pMutexId)
{
    std_mutex_id_t      mutexId;
    pthread_mutexattr_t attr;
    int                rc;

    mutexId = (std_mutex_id_t) std_malloc(sizeof(pthread_mutex_t));
    if (mutexId == NULL)
        return 0;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
    rc = pthread_mutex_init(mutexId, &attr);
    pthread_mutexattr_destroy(&attr);

    if (rc != 0)
        /* Failed to init mutex */
        return 0;

    *pMutexId = mutexId;
    return 1;
}

extern int std_delete_mutex(std_mutex_id_t mutexId)
{
    int rc;

    if (mutexId == STD_NO_MUTEX)
        /* Return failed for operating empty object */
        return 0;

    if ((rc = pthread_mutex_destroy(mutexId)) != 0 &&
        rc != EBUSY)
    {
        /* Failed to delete a mutex */
        return 0;
    }

    std_free(mutexId);
    return 1;
}

extern int std_get_mutex(std_mutex_id_t mutexId)
{
    int rc;

    if (mutexId == STD_NO_MUTEX)
        /* Return failed for operating empty object */
        return 0;

    for (;;)
    {
        if ((rc = pthread_mutex_lock(mutexId)) == 0)
            /* Got mutex */
            return 1;

        if (rc != EINTR)
            /* Failed to get mutex */
            return 0;
    }
}

extern int std_get_mutex_by_time(std_mutex_id_t mutexId, int timeout)
{
    int toTick;
    struct timespec tm;
    struct timeval  tv;
    struct timezone tz;
    int   rc;

    if (mutexId == STD_NO_MUTEX)
        /* Return failed for operating empty object */
        return 0;

    if (timeout == STD_WAIT_FOREVER)
        /* Wait forever */
        return std_get_mutex(mutexId);

    gettimeofday(&tv, &tz);
    tm.tv_sec = tv.tv_sec + timeout / 1000;
    tm.tv_nsec = tv.tv_usec * 1000 + (timeout % 1000) * 1000000;
    if (tm.tv_nsec >= 1000000000)
    {
        tm.tv_nsec -= 1000000000;
        tm.tv_sec++;
    }

    toTick = std_get_current_tick() + timeout;
    for (;;)
    {
#if 1
        if ((rc = pthread_mutex_trylock(mutexId)) == 0)
            return 1;

        std_sleep(1);
        if (std_get_current_tick() >= toTick)
            return 0;
#else
        if ((rc = pthread_mutex_timedlock(mutexId, &tm)) == 0)
            /* Got mutex */
            return 1;

        if (rc != EINTR)
            return 0;
#endif
    }
}

extern int std_release_mutex(std_mutex_id_t mutexId)
{
    if (mutexId == STD_NO_MUTEX)
        /* Return failed for operating empty object */
        return 0;

    return pthread_mutex_unlock(mutexId) == 0;
}

extern int std_forceReleaseMutex(std_mutex_id_t mutexId)
{
    if (mutexId == STD_NO_MUTEX)
        /* Return failed for operating empty object */
        return 0;

    /* Not support */
    STD_ASSERT(0);
    return 0;
}

/* Initialize a critical section */
extern void std_init_critical_section(std_critical_section_t *pSection)
{
    pthread_mutexattr_t attr;

    STD_ASSERT(pSection != NULL);

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&pSection->section, &attr);
    pthread_mutexattr_destroy(&attr);
}

/* Destroy the critical section */
extern void std_destroy_critical_section(std_critical_section_t *pSection)
{
    STD_ASSERT(pSection != NULL);
    pthread_mutex_destroy(&pSection->section);
}

/* Enter critical section */
extern void std_enter_critical_section(std_critical_section_t *pSection)
{
    int rc;

    STD_ASSERT(pSection != NULL);

    for (;;)
    {
        rc = pthread_mutex_lock(&pSection->section);
        if (rc == 0)
            return;
    }
}

/* Try enter critical section */
extern int std_try_enter_critical_section(std_critical_section_t *pSection)
{
    STD_ASSERT(pSection != NULL);
    return pthread_mutex_trylock(&pSection->section) == 0;
}

/* Leave critical section */
extern void std_leave_critical_section(std_critical_section_t *pSection)
{
    STD_ASSERT(pSection != NULL);
    pthread_mutex_unlock(&pSection->section);
}

extern int std_create_semaphore(std_semaphore_id_t *pSemId)
{
    std_posix_sem_t *semId;

    semId = (std_posix_sem_t *) std_malloc(sizeof(std_posix_sem_t));
    if (semId == NULL)
        /* Failed to allocate memory */
        return 0;

    if (pthread_mutex_init(&semId->mutex, NULL) != 0)
    {
        /* Failed to init mutex */
        std_free(semId);
        return 0;
    }

    if (pthread_cond_init(&semId->condition, NULL) != 0)
    {
        /* Failed to init condition */
        pthread_mutex_destroy(&semId->mutex);
        std_free(semId);
        return 0;
    }

    /* Initialize count */
    semId->count = 0;

    *pSemId = (std_semaphore_id_t) semId;
    return 1;
}

extern int std_delete_semaphore(std_semaphore_id_t semId)
{
    if (semId == STD_NO_SEMAPHORE)
        /* Return failed for operating empty object */
        return 0;

    pthread_mutex_destroy(&semId->mutex);
    pthread_cond_destroy(&semId->condition);
    std_free(semId);

    return 1;
}

extern int std_take_semaphore(std_semaphore_id_t semId)
{
    int rc;

    if (semId == STD_NO_SEMAPHORE)
        /* Return failed for operating empty object */
        return 0;

    if (pthread_mutex_lock(&(semId->mutex)) != 0)
    {
        /* Failed to lock mutex */
        return 0;
    }

    while (semId->count <= 0)
    {
        if ((rc = pthread_cond_wait(&(semId->condition), &(semId->mutex))) == 0)
            /* Got condition, check immediately */
            continue;

        if (rc != EINTR)
        {
            /* Failed to got condition */
            pthread_mutex_unlock(&(semId->mutex));
            return 0;
        }
    }

    /* Decrease counter */
    semId->count--;

    pthread_mutex_unlock(&(semId->mutex));

    /* OK */
    return 1;
}

extern int std_take_semaphore_by_time(std_semaphore_id_t semId, int timeout)
{
    struct timespec tm;
    struct timeval  tv;
    struct timezone tz;
    int   rc;

    if (semId == STD_NO_SEMAPHORE)
        /* Return failed for operating empty object */
        return 0;

    if (timeout == STD_WAIT_FOREVER)
        /* Wait forever */
        return std_take_semaphore(semId);

    if (pthread_mutex_lock(&(semId->mutex)) != 0)
        /* Failed to lock mutex */
        return 0;

    gettimeofday(&tv, &tz);
    tm.tv_sec = tv.tv_sec + timeout / 1000;
    tm.tv_nsec = tv.tv_usec * 1000 + (timeout % 1000) * 1000000;
    if (tm.tv_nsec >= 1000000000)
    {
        tm.tv_nsec -= 1000000000;
        tm.tv_sec++;
    }

    while (semId->count <= 0)
    {
        if ((rc = pthread_cond_timedwait(&(semId->condition),
                                         &(semId->mutex), &tm)) == 0)
            /* Got signal, check immediately */
            continue;

        /* Unlock it */
        if (rc != EINTR)
        {
            /* Failed to wait condition (error or timeout) */
            pthread_mutex_unlock(&(semId->mutex));
            return 0;
        }
    }

    /* Decrease counter */
    semId->count--;

    pthread_mutex_unlock(&(semId->mutex));

    /* OK */
    return 1;
}

extern int std_give_semaphore(std_semaphore_id_t semId)
{
    if (semId == STD_NO_SEMAPHORE)
        /* Return failed for operating empty object */
        return 0;

    if (pthread_mutex_lock(&(semId->mutex)) != 0)
        /* Failed to lock */
        return 0;

    if (semId->count >= 0x7FFFFFFF)
        /* count is over maximum */
        return 0;

    /* Increase counter */
    semId->count++;

    pthread_mutex_unlock(&(semId->mutex));

    if (pthread_cond_signal(&(semId->condition)) != 0)
    {
        /* Failed to send signal */
        return 0;
    }

    return 1;
}

extern int std_create_event(std_event_id_t *pEventId)
{
    std_posix_event_t *eventId;

    eventId = (std_posix_event_t *) std_malloc(sizeof(std_posix_event_t));
    if (eventId == NULL)
        /* Failed to allocate memory */
        return 0;

    if (pthread_mutex_init(&eventId->mutex, NULL) != 0)
    {
        /* Failed to init mutex */
        std_free(eventId);
        return 0;
    }

    if (pthread_cond_init(&eventId->condition, NULL) != 0)
    {
        /* Failed to init condition */
        pthread_mutex_destroy(&eventId->mutex);
        std_free(eventId);
        return 0;
    }

    *pEventId = eventId;
    return 1;
}

extern int std_delete_event(std_event_id_t eventId)
{
    if (eventId == STD_NO_EVENT)
        /* Return failed for operating empty object */
        return 0;

    pthread_mutex_destroy(&eventId->mutex);
    pthread_cond_destroy(&eventId->condition);
    std_free(eventId);

    return 1;
}

extern int std_wait_event(std_event_id_t eventId)
{
    int rc;

    if (eventId == STD_NO_EVENT)
        /* Return failed for operating empty object */
        return 0;

    if (pthread_mutex_lock(&(eventId->mutex)) != 0)
    {
        /* Failed to lock mutex */
        return 0;
    }

    while ((rc = pthread_cond_wait(&(eventId->condition), &(eventId->mutex))) == EINTR)
        /* Again */
        continue;

    pthread_mutex_unlock(&(eventId->mutex));

    /* OK */
    return rc == 0;
}

extern int std_wait_event_by_time(std_event_id_t eventId, int timeout)
{
    struct timespec tm;
    struct timeval  tv;
    struct timezone tz;
    int   rc;

    if (eventId == STD_NO_EVENT)
        /* Return failed for operating empty object */
        return 0;

    if (timeout == STD_WAIT_FOREVER)
        /* Wait forever */
        return std_wait_event(eventId);

    if (pthread_mutex_lock(&(eventId->mutex)) != 0)
        /* Failed to lock mutex */
        return 0;

    gettimeofday(&tv, &tz);
    tm.tv_sec = tv.tv_sec + timeout / 1000;
    tm.tv_nsec = tv.tv_usec * 1000 + (timeout % 1000) * 1000000;
    if (tm.tv_nsec >= 1000000000)
    {
        tm.tv_nsec -= 1000000000;
        tm.tv_sec++;
    }

    while ((rc = pthread_cond_timedwait(&(eventId->condition),
                                        &(eventId->mutex), &tm)) == EINTR)
        /* Again */
        continue;

    pthread_mutex_unlock(&(eventId->mutex));

    /* OK */
    return rc == 0;
}

extern int std_raise_event(std_event_id_t eventId)
{
    if (eventId == STD_NO_EVENT)
        /* Return failed for operating empty object */
        return 0;

    if (pthread_cond_broadcast(&(eventId->condition)) != 0)
    {
        /* Failed to send signal */
        return 0;
    }

    return 1;
}

extern int std_create_proc_sem(std_proc_sem_id_t *pProcSemId, const char *name)
{
    sem_t *sem;
    sem_unlink(name);
    sem = sem_open(name, O_CREAT, 0666, 0);
    if (sem == SEM_FAILED)
    {
        *pProcSemId = NULL;
        return 0;
    }

    *pProcSemId = (std_proc_sem_id_t) sem;
    return 1;
}

extern int std_open_proc_sem(std_proc_sem_id_t *pProcSemId, const char *name)
{
    sem_t *sem;

    sem = sem_open(name, 0, 0666, 0);
    if (sem == SEM_FAILED)
    {
        *pProcSemId = NULL;
        return 0;
    }

    *pProcSemId = (std_proc_sem_id_t) sem;
    return 1;
}

extern int std_close_proc_sem(std_proc_sem_id_t semId)
{
    return sem_close((sem_t *) semId) == 0;
}

extern int std_unlink_proc_sem(const char *name)
{
    return sem_unlink(name) == 0;
}

extern int std_take_proc_sem(std_proc_sem_id_t semId)
{
    do
    {
        if (sem_wait((sem_t *) semId) == 0)
            return 1;

        if (std_get_error() != EINTR)
            return 0;
    } while (1);
}

/* The implementation is not exactly "waitByTime" since the mac os
 * doesn't support sem_timedWait currently */
extern int std_take_proc_sem_by_time(std_proc_sem_id_t semId, int timeout)
{
#if (_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600)
    struct timespec tm;
    struct timeval  tv;
    struct timezone tz;

    gettimeofday(&tv, &tz);
    tm.tv_sec = tv.tv_sec + timeout / 1000;
    tm.tv_nsec = tv.tv_usec * 1000 + (timeout % 1000) * 1000000;
    if (tm.tv_nsec >= 1000000000)
    {
        tm.tv_nsec -= 1000000000;
        tm.tv_sec++;
    }

    do
    {
        if (sem_timedwait((sem_t *) semId, &tm) == 0)
            return 1;

        switch (std_get_error())
        {
        case EINTR:
            /* Try again */
            break;

        default:
        case ETIMEDOUT:
            return 0;
        }
    } while (1);
#else

#warning sem_timedwait is not supported, use sem_trywait + sleep

    std_tick_t tick = std_get_current_tick();
    for (;;)
    {
        int period = (int) (std_get_current_tick() - tick);
        if (period < 0 || period >= timeout)
            // timeout
            return 0;

        if (sem_trywait((sem_t *) semId) == 0)
            return 1;

        std_sleep(1);
    }
#endif
}

extern int std_try_take_proc_sem(std_proc_sem_id_t semId)
{
    do
    {
        if (sem_trywait((sem_t *) semId) == 0)
            return 1;

        if (std_get_error() != EINTR)
            return 0;
    } while (1);
}

extern int std_give_proc_sem(std_proc_sem_id_t semId)
{
    return sem_post((sem_t *) semId) == 0;
}

extern int std_create_shrd_mem(std_shrd_mem_id_t *pMemId, size_t size, const char *name)
{
    int shmid;
#if 0
    key_t key;

    char path[256];
    STD_SNPRINTF(path, sizeof(path), "%s%s", std_get_temp_dir(), name);
    key = ftok(path, 0);
    if (key == -1)
        // Failed to create key from name
        return 0;

    size = (size + (STD_MEM_PAGE_SIZE - 1)) & ~(STD_MEM_PAGE_SIZE - 1);
    shmid = shmget(key, size, 0666 | IPC_CREAT);
#else
    size = (size + (STD_MEM_PAGE_SIZE - 1)) & ~(STD_MEM_PAGE_SIZE - 1);
    shm_unlink(name);
    shmid = shm_open(name, O_RDWR | O_CREAT, 0777);
    if (ftruncate(shmid, size) != 0)
    {
        // Failed to set size
        return 0;
    }
#endif
    if (shmid == -1)
    {
        std_end_process(0);
        return 0;
    }

    *pMemId = (std_shrd_mem_id_t) shmid;
    return 1;
}

extern int std_open_shrd_mem(std_shrd_mem_id_t *pMemId, size_t size, const char *name)
{
    int shmid;
#if 0
    key_t key;

    char path[256];
    STD_SNPRINTF(path, sizeof(path), "%s%s", std_get_temp_dir(), name);
    key = ftok(path, 0);
    if (key == -1)
        // Failed to create key from name
        return 0;

    size = (size + (STD_MEM_PAGE_SIZE - 1)) & ~(STD_MEM_PAGE_SIZE - 1);
    shmid = shmget(key, size, 0);
#else
    size = (size + (STD_MEM_PAGE_SIZE - 1)) & ~(STD_MEM_PAGE_SIZE - 1);
    shmid = shm_open(name, O_RDWR, 0777);
#endif
    if (shmid == -1)
        return 0;

    *pMemId = (std_shrd_mem_id_t) shmid;
    return 1;
}

extern int std_close_shrd_mem(std_shrd_mem_id_t memId)
{
#if 0
    return shmctl(memId, IPC_RMID, NULL) == 0;
#else
    return close(memId) == 0;
#endif
}

extern int std_unlink_shrd_mem(const char *name)
{
    return shm_unlink(name) == 0;
}

// ATTENTION:
// The following code is not thread-safe
// The purpose is to detect SIGBUS after mmap. The solution is not perfect.  
std_spin_lock_t _std_sig_spin_lock = INITED_SPIN_LOCK;

typedef void (*_std_sighandler_t)(int);
static sigjmp_buf _std_err_env;
static void _std_signalBusHandler(int signo)
{
    siglongjmp(_std_err_env, 1);
}

// Lookup the memory to make sure it can be operated
// Return 0 if failed
// Return 0x1XX if succeed
static int _std_verify_mapped_mem(void *p, size_t size)
{
    size_t i;
    unsigned char *np = (unsigned char *) p;
    volatile unsigned char blue; // To prevent "blue += *np" be optimized
    _std_sighandler_t prev_handler;
    int ret;

    std_get_spin_lock(&_std_sig_spin_lock);

    blue = 0;
    if (sigsetjmp(_std_err_env, 1) == 0)
    {
        // Register signal to catch SIGBUS
        prev_handler = signal(SIGBUS, _std_signalBusHandler);

        for (i = 0; i < size; i += STD_MEM_PAGE_SIZE)
        {
            blue += *np;
            np += STD_MEM_PAGE_SIZE;
        }

        ret = 1;
    } else
    {
        // Exception (SIGBUS) occurred
        ret = 0;
        fprintf(stderr, "mmap failed: at = %p, size = %d, NOT ACCESSABLE.\n",
                p, (int) size);
    }

    // Restore signal
    signal(SIGBUS, prev_handler);

    std_release_spin_lock(&_std_sig_spin_lock);
    return ret;
}

extern int std_map_shrd_mem(std_shrd_mem_id_t memId, size_t size, void **ppMapAt)
{
    void *p;

    // Make sure page aligment
    size = (size + (STD_MEM_PAGE_SIZE - 1)) & ~(STD_MEM_PAGE_SIZE - 1);

#if 0
    p = shmat(memId, NULL, 0);
#else
    p = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED,
             memId, 0);
#endif

    // Make sure all pages is valid to access
    if (p != MAP_FAILED)
    {
        if (! _std_verify_mapped_mem(p, size))
        {
            // The memory is not accessible
            munmap(p, size);
            p = MAP_FAILED;
        }
    }

    *ppMapAt = p;
    return p != MAP_FAILED;
}

extern int std_unmap_shrd_mem(std_shrd_mem_id_t memId, void *pMapAt, size_t size)
{
    STD_REFER(memId);

    // Make sure page aligment
    size = (size + (STD_MEM_PAGE_SIZE - 1)) & ~(STD_MEM_PAGE_SIZE - 1);

    return munmap(pMapAt, size) == 0;
}

typedef void *(*std_Posix_Start_Routine_t)(void *arg);

/* BE AWARE address of pTaskId may not in stack since the routine may delay
 * until std_multiThreadBoot set to TRUE */
extern int std_create_task(const char *name, std_task_id_t *pTaskId,
                           void *entry, void *para)
{
    pthread_attr_t attr;
    int rc;
    int stack_size = std_get_default_task_stack_size();
    std_task_id_t taskId;
    struct std_task_para *taskPara;

    taskPara = std_create_task_para(name, entry, para);
    if (taskPara == NULL)
        /* Can't create task para */
        return 0;

    if (stack_size < 65536)
        /* _size of stack at least be 64K */
        stack_size = 65536;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, stack_size);
    rc = pthread_create(&taskId, &attr,
                        (std_Posix_Start_Routine_t) std_task_entry, taskPara);
    pthread_attr_destroy(&attr);

    if (rc != 0)
    {
        /* Failed to create thread */
        return 0;
    }

    if (pTaskId != NULL)
        *pTaskId = taskId;

    STD_REFER(stack_size);
    return 1;
}

extern void std_delete_task(std_task_id_t taskId)
{
    void *result;

    if (taskId == STD_NON_tASK)
        /* Return for operating empty object */
        return;

    /* Cancel & wait the thread done */
    pthread_cancel(taskId);
    pthread_join(taskId, &result);
}

extern int std_resume_task(std_task_id_t taskId)
{
#if 0
    return pthread_suspend_np(taskId) == 0 ? 1 : 0;
#else
    return 1;
#endif
}

extern int std_suspend_task(std_task_id_t taskId)
{
#if 0
    return pthread_unsuspend_np(taskId) == 0 ? 1 : 0;
#else
    return 1;
#endif
}

/* Allocate thread local storage */
extern int std_allocate_tls(std_tls_t *pTls)
{
    if (pthread_key_create(pTls, 0) != 0)
        /* Failed to create tls */
        return 0;

    return 1;
}

/* Get data in thread local storage */
extern void *std_get_tls_data(std_tls_t tls)
{
    return pthread_getspecific(tls);
}

/* Put data into thread local storage */
extern void std_set_tls_data(std_tls_t tls, void *pTlsData)
{
    pthread_setspecific(tls, pTlsData);
}

/* Free thread local storage */
extern void std_free_tls(std_tls_t tls)
{
    pthread_key_delete(tls);
}

/* Return current task id */
extern std_task_id_t std_get_current_task_id()
{
    return (std_task_id_t) pthread_self();
}

/* Return process id */
extern std_pid_t std_get_current_process_id()
{
    return (std_pid_t) getpid();
}

/* Is process alive? */
extern int std_is_process_alive(std_pid_t pid)
{
    if (kill(pid, 0) == -1 && std_get_error() == ESRCH)
        /* Process is dead */
        return 0;

    return 1;
}

/* Relinquish CPU */
extern void std_relinquish()
{
    sched_yield();
}

#endif /* End of STD_MULTI_tHREAD */

#endif  /* End of _POSIX */
