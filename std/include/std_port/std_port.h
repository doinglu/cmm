/******************************************************************
 *
 * @(#)std_port.h:
 *
 * Purpose: 
 *  Header file to support portable.
 *
 * Functions:
 *
 * History:
 *  2002.1.2        Initial version
 *                  doing
 *
 ***** Copyright 2001, doing reserved *****************************/

#ifndef __STD_PORT_H__
#define __STD_PORT_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _DEBUG
#include <stdio.h>
#endif

#include <ctype.h>
#include "std_port_platform.h"
#include "std_port_type.h"
#include "std_port_spin_lock.h"

#define STD_SIZE_N(arr)             sizeof(arr) / sizeof(arr[0])
#ifndef SIZE_MAX
#define SIZE_MAX                    ((size_t)-1)
#endif

#define STD_UNKNOWN_ERROR           -1

/* Platform utillib */
#ifdef _WINDOWS
#define STD_ALLOCA(n)               _alloca(n)
#define STD_SNPRINTF                _snprintf
#define STD_VSNPRINTF               _vsnprintf
#define STD_ERRNO()                 std_get_error()
#else
#include <stdlib.h>
#define STD_ALLOCA(n)               alloca(n)
#define STD_SNPRINTF                snprintf
#define STD_VSNPRINTF               vsnprintf
#define STD_ERRNO()                 stdcd_getError()
#endif

#ifdef _DEBUG
/* Debug mode */
#define STD_ASSERT(b)           do { if (! (b)) { printf("assertion failed: %s\nFile = %s\nLine = %d\nSystem Error = %d\n", #b, __FILE__, (int) __LINE__, std_get_error()); STD_FATAL("Assert error"); } } while (0)
#define STD_ASSURE(e, r)        do { if ((e) != (r)) { printf("assuration failed: %s == %s\nFile = %s\nLine = %d\nSystem Error = %d\n", #e, #r, __FILE__, (int) __LINE__, std_get_error()); STD_FATAL("Unexpeced error occured"); } } while (0)
#define STD_FATAL(msg)          do { printf("Fatal error = %s\nFILE(%s)\nLINE(%d)\n", msg, __FILE__, __LINE__); std_end_process(0); } while (0)
#define	STD_REFER(v)            *((char *) &v) = 0
#define STD_TRACE(...)          printf(__VA_ARGS__)
#else
/* Release mode */
#define STD_ASSERT(b)
#define STD_ASSURE(e, r)        (e)
#define STD_FATAL(msg)          do { printf("Fatal error = %s\nFILE(%s)\nLINE(%d)\n", msg, __FILE__, __LINE__); std_end_process(0); for (;;); } while (0)
#define	STD_REFER(v)
#define STD_TRACE(...)
#endif

/* Common type */
typedef unsigned                std_time_t;
typedef unsigned                std_tick_t;
typedef unsigned                std_freq_t;

/* Max timer timeout */
#define STD_MAX_tIMER_tIMEOUT   1200000 /* 120s */

/* Memory page size */
#define STD_MEM_PAGE_SIZE       4096

#ifdef _VXWORKS
/* Under VxWorks */

#include <vxworks.h>
#include <tasklib.h>
#include <semlib.h>
#include <sysLib.h>

#define STD_MULTI_tHREAD

typedef unsigned            std_locked_counter_t;
typedef unsigned            std_task_id_t;
typedef unsigned            std_pid_t;
typedef SEM_ID              std_mutex_id_t;
typedef SEM_ID              std_semaphore_id_t;
typedef SEM_ID              std_event_Id_t;
typedef SEM_ID              std_proc_sem_id_t; /* Not implemented yet */
typedef void *              std_shrd_mem_id_t; /* Not implemented yet */
typedef void *              std_tls_t; /* Not implemented yet */

#define STD_NON_tASK        (std_task_id_t) 0
#define STD_NO_TLS_ID       -1
#define STD_NO_PID          (std_pid_t) 0
#define STD_NO_MUTEX        (std_mutex_id_t) 0
#define STD_NO_SEMAPHORE    (std_semaphore_id_t) 0
#define STD_NO_EVENT        (std_event_id_t) 0
#define STD_NO_PROC_SEM     (std_proc_sem_id_t) 0
#define STD_NO_SHRD_MEM     (std_shrd_mem_id_t) 0
#define STD_MAX_TICKS       0x7FFFFFFF
#define STD_WAIT_FOREVER    WAIT_FOREVER
#define STD_NO_WAIT         NO_WAIT

/* Macro routines under VXWORKS */
#define std_getOSTick()     (std_tick_t) tickGet()
#define std_getOSTick64()   ((std_freq_t) time(NULL) * 1000000 + (tickGet() % 1000) * 1000)
#define std_tickPerSecond() (std_tick_t) sysClkRateGet()

#else   /* NO _VXWORKS */

#ifdef _WINDOWS

#define STD_MULTI_tHREAD

/* My critical section with init flag */

typedef unsigned            std_locked_counter_t;
typedef unsigned            std_task_id_t;
typedef unsigned            std_pid_t;
typedef void *              std_mutex_id_t;
typedef void *              std_semaphore_id_t;
typedef void *              std_event_id_t;
typedef void *              std_proc_sem_id_t;
typedef void *              std_shrd_mem_id_t;
typedef unsigned            std_tls_t;

#define STD_NON_tASK        0
#define STD_NO_TLS_ID       -1
#define STD_NO_PID          0
#define STD_NO_MUTEX        NULL
#define STD_NO_SEMAPHORE    NULL
#define STD_NO_EVENT        (std_event_id_t) 0
#define STD_MAX_tICKS       MAX_R_VALUE
#define STD_WAIT_FOREVER    -1
#define STD_NO_WAIT         0
#define STD_NO_PROC_SEM     NULL
#define STD_NO_SHRD_MEM     (std_shrd_mem_id_t) 0

/* Macro routines under WIN32 */
#define std_tickPerSecond()     (std_tick_t) 1000
extern std_time_t std_getOSTime();
extern std_tick_t std_getOSTick();
extern std_freq_t std_getOSUsCounter();

#else   /* NOT WINDOWS */

#include <unistd.h>

typedef unsigned              std_locked_counter_t;

#ifdef _POSIX

/* Posix */
#include <pthread.h> 

#define STD_MULTI_tHREAD
/* Multi thread for POSIX under UNIX */
/* To implement semaphore under posix */
typedef struct std_posix_sem
{
    pthread_mutex_t  mutex;
    pthread_cond_t   condition;
    int              count;
} std_posix_sem_t;

/* To implement event under posix */
typedef struct std_posix_event
{
    pthread_mutex_t  mutex;
    pthread_cond_t   condition;
} std_posix_event_t;

typedef pthread_t           std_task_id_t;
typedef pid_t               std_pid_t;
typedef pthread_mutex_t *   std_mutex_id_t;
typedef std_posix_sem_t *   std_semaphore_id_t;
typedef std_posix_event_t * std_event_id_t;
typedef void *              std_proc_sem_id_t;
typedef int                 std_shrd_mem_id_t;
typedef pthread_key_t       std_tls_t;
#else
/* Single thread under UNIX */
#undef  STD_MULTI_tHREAD
#define STD_NO_MULTI_tHREAD
#endif

/* If not STD_MULTI_tHREAD is defined, declare common definition */
#ifdef STD_NO_MULTI_tHREAD
typedef unsigned            std_task_id_t;
typedef pid_t               std_pid_t;
typedef unsigned            std_mutex_id_t;
typedef void *              std_semaphore_id_t;
typedef void *              std_event_id_t;
typedef void *              std_proc_sem_id_t;
typedef int                 std_shrd_mem_id_t;
typedef unsigned            std_tls_t;
#endif

#define STD_NON_tASK        (std_task_id_t) 0
#define STD_NO_TLS_ID       -1
#define STD_NO_PID          (std_pid_t) 0
#define STD_NO_MUTEX        (std_mutex_id_t) 0
#define STD_NO_SEMAPHORE    (std_semaphore_id_t) 0
#define STD_NO_EVENT        (std_event_id_t) 0
#define STD_NO_PROC_SEM     (std_proc_sem_id_t) 0
#define STD_NO_SHRD_MEM     (std_shrd_mem_id_t) 0
#define STD_MAX_tICKS       0x7FFFFFFF
#define STD_WAIT_FOREVER    -1
#define STD_NO_WAIT         0

/* Macro routines under normal UNIX OS */
#define std_tickPerSecond() (std_tick_t) 1000
extern std_time_t std_getOSTime();
extern std_tick_t std_getOSTick();
extern std_freq_t std_getOSUsCounter();

#endif  /* End of WIN32 */

#endif  /* End of _VXWORKS */

/* Max length of task name */
#define STD_MAX_tASK_NAME_LEN               16

struct std_critical_section;

/* Memory allocation/free routines */
typedef void *(*std_malloc_t)(size_t);
typedef void (*std_free_t)(void *ptr);

/* Spin lock */
/* This routines implemented in std_port.c, so the caller won't be need to
 * include the header file "std_port_cs.h" which may include a huge header
 * file: "windows.h" */
extern void          std_get_spin_lock_impl(std_spin_lock_t *lock);
extern void          std_release_spin_lock_impl(std_spin_lock_t *lock);

/* Mutex Operations */
extern int           std_create_mutex(std_mutex_id_t *pMutexId);
extern int           std_delete_mutex(std_mutex_id_t mutexId);
extern int           std_get_mutex(std_mutex_id_t mutexId);
extern int           std_get_mutex_by_time(std_mutex_id_t mutexId, int timeout);
extern int           std_release_mutex(std_mutex_id_t mutexId);

/* CriticalSection Operations */
extern int           std_new_critical_section(struct std_critical_section **ppSection);
extern void          std_delete_critical_section(struct std_critical_section *pSection);
extern void          std_init_critical_section(struct std_critical_section *pSection);
extern void          std_destroy_critical_section(struct std_critical_section *pSection);
extern void          std_enter_critical_section(struct std_critical_section *pSection);
extern int           std_try_enter_critical_section(struct std_critical_section *pSection);
extern void          std_leave_critical_section(struct std_critical_section *pSection);

/* Semaphore Operations */
extern int           std_create_semaphore(std_semaphore_id_t *pSemId);
extern int           std_delete_semaphore(std_semaphore_id_t semId);
extern int           std_take_semaphore(std_semaphore_id_t semId);
extern int           std_take_semaphore_by_time(std_semaphore_id_t semId, int timeout);
extern int           std_give_semaphore(std_semaphore_id_t semId);

/* Event Operations */
extern int           std_create_event(std_event_id_t *pEventId);
extern int           std_delete_event(std_event_id_t eventId);
extern int           std_wait_event(std_event_id_t eventId);
extern int           std_waitEventByTime(std_event_id_t eventId, int timeout);
extern int           std_raise_event(std_event_id_t eventId);

/* Semaphore may be accessed in processes */
extern int           std_create_proc_sem(std_proc_sem_id_t *pProcSemId, const char *name);
extern int           std_open_proc_sem(std_proc_sem_id_t *pProcSemId, const char *name);
extern int           std_close_proc_sem(std_proc_sem_id_t semId);
extern int           std_unlink_proc_sem(const char *name);
extern int           std_take_proc_sem(std_proc_sem_id_t semId);
extern int           std_take_proc_sem_by_time(std_proc_sem_id_t semId, int timeout);
extern int           std_try_take_proc_sem(std_proc_sem_id_t semId);
extern int           std_give_proc_sem(std_proc_sem_id_t semId);

/* Shared memory */
extern int           std_create_shrd_mem(std_shrd_mem_id_t *pMemId, size_t size, const char *name);
extern int           std_open_shrd_mem(std_shrd_mem_id_t *pMemId, size_t size, const char *name);
extern int           std_close_shrd_mem(std_shrd_mem_id_t memId);
extern int           std_unlink_shrd_mem(const char *name);
extern int           std_map_shrd_mem(std_shrd_mem_id_t memId, size_t size, void **ppMapAt);
extern int           std_unmap_shrd_mem(std_shrd_mem_id_t memId, void *pMapAt, size_t size);

/* Thread Operations */
extern int           std_create_task(const char *name, std_task_id_t *pTaskId, void *entry, void *para);
extern void          std_delete_task(std_task_id_t taskId);
extern const char   *std_get_task_name(std_task_id_t taskId, char *buf, size_t size);
extern int           std_set_current_task_name(const char *name);
extern int           std_resume_task(std_task_id_t taskId);
extern int           std_suspend_task(std_task_id_t taskId);
extern int           std_allocate_tls(std_tls_t *pTls);
extern void         *std_get_tls_data(std_tls_t tls);
extern void          std_set_tls_data(std_tls_t tls, void *pTlsData);
extern void          std_free_tls(std_tls_t tls);
extern std_task_id_t std_get_current_task_id();
extern std_pid_t     std_get_current_process_id();
extern int           std_is_process_alive(std_pid_t pid);
extern void          std_sleep(int msec);
extern void          std_relinquish();
extern void          std_set_default_task_stack_size(size_t size);
extern size_t        std_get_default_task_stack_size();

extern int           std_fork();

/* Timer Operations */
extern int           std_start_timer(int index, int msec);
extern int           std_is_timer_timeout(int index);
extern void          std_clear_timer(int index);

/* Overrided system functions */
extern void          std_get_cwd(char *path, size_t size);
extern const char   *std_get_temp_dir();
extern int           std_get_error();
extern void          std_end_process(int r);
extern std_tick_t    std_get_current_tick();
extern std_freq_t    std_get_current_us_counter();

/* Memory operations */
extern void         *std_malloc(size_t size);
extern void          std_free(void *p);

/* Initialize std port */
extern void          std_init(std_malloc_t malloc_func, std_free_t free_func);
extern void          std_shutdown();

#ifdef __cplusplus
}
#endif

#endif  /* end of __STD_PORT_H__ */
