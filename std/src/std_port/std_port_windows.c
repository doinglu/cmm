/******************************************************************
 *
 * @(#)std_port_windows.c:
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

#ifdef _WINDOWS

#include "std_port/std_port.h"
#include "std_port/std_port_cs.h"
#include "std_port/std_port_internal.h"

#include <stdio.h>

/* To define WIN32 NT version */
#ifndef _WIN32_WINNT
#define _WIN32_WINNT    0x0400
#endif

#include <windows.h>
#include <sys/timeb.h>
#include <time.h>

#ifndef STD_NO_MULTI_tHREAD

/* Support functions only for windows */
extern int std_create_mutex(std_mutex_id_t *pMutexId)
{
    std_mutex_id_t mutexId;

    STD_ASSERT(pMutexId != NULL);

    mutexId = (std_mutex_id_t) CreateMutex(NULL, FALSE, NULL);

    if (mutexId == STD_NO_MUTEX)
        return 0;

    *pMutexId = mutexId;

    return 1;
}

extern int std_delete_mutex(std_mutex_id_t mutexId)
{
    return CloseHandle((HANDLE) mutexId) ? 1 : 0;
}

extern int std_get_mutex(std_mutex_id_t mutexId)
{
    if (mutexId == STD_NO_MUTEX)
        /* Return failed for operating empty object */
        return 0;

    return WaitForSingleObject((HANDLE) mutexId, INFINITE) == WAIT_OBJECT_0 ? 1 : 0;
}

extern int std_get_mutex_by_time(std_mutex_id_t mutexId, int timeout)
{
    if (mutexId == STD_NO_MUTEX)
        /* Return failed for operating empty object */
        return 0;

    if (timeout == STD_WAIT_FOREVER)
        return std_get_mutex(mutexId);

    return WaitForSingleObject((HANDLE) mutexId, (DWORD) timeout) == WAIT_OBJECT_0 ? 1 : 0;
}

extern int std_release_mutex(std_mutex_id_t mutexId)
{
    if (mutexId == STD_NO_MUTEX)
        /* Return failed for operating empty object */
        return 0;

    return ReleaseMutex((HANDLE) mutexId) ? 1 : 0;
}

/* Initialize a critical section */
extern void std_init_critical_section(std_critical_section_t *pSection)
{
    STD_ASSERT(pSection != NULL);
    InitializeCriticalSection(&pSection->section);
}

/* Destroy the critical section */
extern void std_destroy_critical_section(std_critical_section_t *pSection)
{
    STD_ASSERT(pSection != NULL);
    DeleteCriticalSection(&pSection->section);
}

/* Enter critical section */
extern void std_enter_critical_section(std_critical_section_t *pSection)
{
    STD_ASSERT(pSection != NULL);
    EnterCriticalSection(&pSection->section);
}

/* Try enter critical section */
extern int std_try_enter_critical_section(std_critical_section_t *pSection)
{
    STD_ASSERT(pSection != NULL);
    return TryEnterCriticalSection(&pSection->section) ? 1 : 0;
}

/* Leave critical section */
extern void std_leave_critical_section(std_critical_section_t *pSection)
{
#ifdef _DEBUG
    DWORD  ownerId = GetThreadId(pSection->section.OwningThread);
    DWORD  currentId = std_get_current_task_id();
    STD_ASSERT(pSection->section.RecursionCount > 0);
    STD_ASSERT(ownerId == currentId || (DWORD)(size_t)pSection->section.OwningThread == currentId);
#endif
    LeaveCriticalSection(&pSection->section);
}

extern int std_create_semaphore(std_semaphore_id_t *pSemId)
{
    std_semaphore_id_t semId;

    STD_ASSERT(pSemId != NULL);

    semId = (std_semaphore_id_t) CreateSemaphore(NULL, 0, 0x7FFFFFFF, NULL);
    if (semId == STD_NO_SEMAPHORE)
        return 0;

    *pSemId = semId;
    return 1;
}

extern int std_delete_semaphore(std_semaphore_id_t semId)
{
    if (semId == STD_NO_SEMAPHORE)
        /* Return failed for operating empty object */
        return 0;

    return CloseHandle((HANDLE) semId) ? 1 : 0;
}

extern int std_take_semaphore(std_semaphore_id_t semId)
{
    if (semId == STD_NO_SEMAPHORE)
        /* Return failed for operating empty object */
        return 0;

    return WaitForSingleObject((HANDLE) semId, INFINITE) == WAIT_OBJECT_0 ? 1 : 0;
}

extern int std_take_semaphore_by_time(std_semaphore_id_t semId, int timeout)
{
    if (semId == STD_NO_SEMAPHORE)
        /* Return failed for operating empty object */
        return 0;

    if (timeout == STD_WAIT_FOREVER)
        return std_take_semaphore(semId);

    return WaitForSingleObject((HANDLE) semId, (DWORD) timeout) == WAIT_OBJECT_0 ? 1 : 0;
}

extern int std_give_semaphore(std_semaphore_id_t semId)
{
    if (semId == STD_NO_SEMAPHORE)
        /* Return failed for operating empty object */
        return 0;

    return ReleaseSemaphore((HANDLE) semId, 1, NULL) ? 1 : 0;
}

extern int std_create_event(std_event_id_t *pEventId)
{
    std_event_id_t eventId;

    STD_ASSERT(pEventId != NULL);

    eventId = (std_event_id_t) CreateEvent(NULL, FALSE /* Auto reset */, FALSE, NULL);
    if (eventId == STD_NO_EVENT)
        return 0;

    *pEventId = eventId;
    return 1;
}

extern int std_delete_event(std_event_id_t eventId)
{
    if (eventId == STD_NO_EVENT)
        /* Return failed for operating empty object */
        return 0;

    return CloseHandle((HANDLE) eventId) ? 1 : 0;
}

extern int std_wait_event(std_event_id_t eventId)
{
    if (eventId == STD_NO_EVENT)
        /* Return failed for operating empty object */
        return 0;

    return WaitForSingleObject((HANDLE) eventId, INFINITE) == WAIT_OBJECT_0 ? 1 : 0;
}

extern int std_wait_event_by_time(std_event_id_t eventId, int timeout)
{
    if (eventId == STD_NO_EVENT)
        /* Return failed for operating empty object */
        return 0;

    /* Convert the ticks to msecond */
    if (timeout == STD_WAIT_FOREVER)
        return std_wait_event(eventId);

    return WaitForSingleObject((HANDLE) eventId, (DWORD) timeout) == WAIT_OBJECT_0 ? 1 : 0;
}

extern int std_raise_event(std_event_id_t eventId)
{
    int ret;

    /* Raise event & reset it immediately */
    ret = SetEvent((HANDLE) eventId) ? 1 : 0;

    return ret;
}

extern int std_create_proc_sem(std_proc_sem_id_t *pProcSemId, const char *name)
{
    HANDLE h;
    h = CreateSemaphore(NULL, 0, 0x7FFFFFFF, (LPCSTR) name);
    *pProcSemId = (std_proc_sem_id_t) h;
    return h != NULL;
}

extern int std_open_proc_sem(std_proc_sem_id_t *pProcSemId, const char *name)
{
    HANDLE h;
    h = OpenSemaphore(SEMAPHORE_ALL_ACCESS, TRUE, (LPCSTR) name);
    *pProcSemId = (std_proc_sem_id_t) h;
    return h != NULL;
}

extern int std_close_proc_sem(std_proc_sem_id_t semId)
{
    return (int) CloseHandle((HANDLE) semId);
}

extern int std_unlink_proc_sem(const char *name)
{
    /* Do nothing for windows platform */
    return 1;
}

extern int std_take_proc_sem(std_proc_sem_id_t semId)
{
    return WaitForSingleObject((HANDLE) semId, INFINITE) == WAIT_OBJECT_0;
}

extern int std_take_proc_sem_by_time(std_proc_sem_id_t semId, int timeout)
{
    return WaitForSingleObject((HANDLE) semId, timeout) == WAIT_OBJECT_0;
}

extern int std_try_take_proc_sem(std_proc_sem_id_t semId)
{
    return WaitForSingleObject((HANDLE) semId, 0) == WAIT_OBJECT_0;
}

extern int std_give_proc_sem(std_proc_sem_id_t semId)
{
    return (int) ReleaseSemaphore((HANDLE) semId, 1, NULL);
}

extern int std_create_shrd_mem(std_shrd_mem_id_t *pMemId, size_t size, const char *name)
{
    HANDLE h;

    DWORD high32 = 0;
#ifdef DRIVER64
    high32 = (DWORD) (size >> 32);
#endif

    h = CreateFileMapping(
            INVALID_HANDLE_VALUE,    // use paging file
            NULL,                    // default security
            PAGE_EXECUTE_READWRITE,  // read/write access
            high32,
            (DWORD) (size),          // maximum object size (low-order DWORD)
            (LPCSTR) name);          // name of mapping object

    *pMemId = (std_shrd_mem_id_t) h;
    return h != NULL;
}

extern int std_open_shrd_mem(std_shrd_mem_id_t *pMemId, size_t size, const char *name)
{
    HANDLE h;

    STD_REFER(size);

    h = OpenFileMapping(
            FILE_MAP_ALL_ACCESS | FILE_MAP_EXECUTE, // read/write access
            FALSE,                   // inherit handle
            (LPCSTR) name);          // name of mapping object

    *pMemId = (std_shrd_mem_id_t) h;
    return h != NULL;
}

extern int std_close_shrd_mem(std_proc_sem_id_t memId)
{
    return (int) CloseHandle(memId);
}

extern int std_unlink_shrd_mem(const char *name)
{
    /* Do nothing for windows platform */
    return 1;
}

extern int std_map_shrd_mem(std_shrd_mem_id_t memId, size_t size, void **ppMapAt)
{
    void *p;

    // Make sure page aligment
    size = (size + (STD_MEM_PAGE_SIZE - 1)) & ~(STD_MEM_PAGE_SIZE - 1);

    p = (void *) MapViewOfFile(
            (HANDLE) memId,      // handle to map object
            FILE_MAP_ALL_ACCESS, // read/write permission
            0,
            0,
            (size_t) size);

    *ppMapAt = p;
    return p != NULL;
}

extern int std_unmap_shrd_mem(std_shrd_mem_id_t memId, void *pMapAt, size_t size)
{
    STD_REFER(memId);
    STD_REFER(size);
    return (int) UnmapViewOfFile((LPCVOID) pMapAt);
}

extern int std_create_task(const char *name, std_task_id_t *pTaskId,
                           void *entry, void *para)
{
    DWORD   threadId;
    HANDLE  handle;
    size_t  stack_size = std_get_default_task_stack_size();
    struct  std_task_para *taskPara;

    taskPara = std_create_task_para(name, entry, para);
    if (taskPara == NULL)
        /* Can't create task para */
        return 0;

    handle = CreateThread(NULL, (DWORD) stack_size,
                          (LPTHREAD_START_ROUTINE) std_task_entry,
                          (LPVOID) taskPara,
                          0,
                          &threadId);
    if (handle == NULL)
        // Failed to create thread
        return 0;

    if (pTaskId != NULL)
        *pTaskId = threadId;

    STD_REFER(name);

    return 1;
}

extern void std_delete_task(std_task_id_t taskId)
{
    HANDLE handle;

    if (taskId == STD_NON_tASK)
        /* Return for operating empty object */
        return;

    handle = OpenThread(THREAD_ALL_ACCESS, FALSE, taskId);
    if (handle == NULL)
        /* Invalid task id */
        return;

    TerminateThread(handle, -1);
    CloseHandle(handle);
}

extern int std_resume_task(std_task_id_t taskId)
{
    HANDLE handle;
    int    ret;

    handle = OpenThread(THREAD_ALL_ACCESS, FALSE, taskId);
    if (handle == NULL)
        /* Invalid task id */
        return 0;

    ret = ResumeThread(handle) ? 1 : 0;
    CloseHandle(handle);
    return ret;
}

extern int std_suspend_task(std_task_id_t taskId)
{
    HANDLE handle;
    int    ret;

    handle = OpenThread(THREAD_ALL_ACCESS, FALSE, taskId);
    if (handle == NULL)
        /* Invalid task id */
        return 0;

    ret = SuspendThread(handle) ? 1 : 0;
    CloseHandle(handle);
    return ret;
}

/* Allocate thread local storage */
extern int std_allocate_tls(std_tls_t *pTls)
{
    *pTls = TlsAlloc();
    if (*pTls == 0)
        /* Failed to allocate */
        return 0;

    return 1;
}

/* Get data in thread local storage */
extern void *std_get_tls_data(std_tls_t tls)
{
    return TlsGetValue(tls);
}

/* Put data into thread local storage */
extern void std_set_tls_data(std_tls_t tls, void *pTlsData)
{
    TlsSetValue(tls, pTlsData);
}

/* Free thread local storage */
extern void std_free_tls(std_tls_t tls)
{
    TlsFree(tls);
}

/* Return current task id */
extern std_task_id_t std_get_current_task_id()
{
    return (std_task_id_t) GetCurrentThreadId();
}

/* Return process id */
extern std_pid_t std_get_current_process_id()
{
    return (std_pid_t) GetCurrentProcessId();
}

/* Is process alive? */
extern int std_is_process_alive(std_pid_t pid)
{
    HANDLE handle;

    handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (handle != NULL)
    {
        CloseHandle(handle);
        return 1;
    }

    return GetLastError() == ERROR_ACCESS_DENIED;
}

#endif /* End of STD_MULTI_tHREAD */

/* Like time */
extern std_time_t std_get_os_time()
{
    return (std_time_t) time(NULL);
}

/* Return ticks */
extern std_tick_t std_get_os_tick()
{
#ifdef PLATFORM64
    return GetTickCount64();
#else
    return GetTickCount();
#endif
}

/* Return us counter */
extern std_freq_t std_get_os_us_counter()
{
    static LARGE_INTEGER freq;
    static int flag = 0;

    if (flag == 1)
    {
        /* Got the frequency, get performance counter */
        LARGE_INTEGER counter;

        QueryPerformanceCounter(&counter);
        return (std_freq_t) (counter.QuadPart * 1000000 / freq.QuadPart);
    } else
    if (flag == -1)
    {
        /* System doesn't support performance counter */
        struct _timeb tb;

        _ftime(&tb);
        return (std_freq_t) (tb.time * 1000000 + tb.millitm * 1000);
    }

    /* Init, try to get performance frequency */
    flag = QueryPerformanceFrequency(&freq) ? 1 : -1;
    return std_get_os_us_counter();
}

/* Sleep nms */
extern void std_sleep(int msec)
{
    Sleep((DWORD) msec);
}

/* Relinquish CPU */
extern void std_relinquish(int msec)
{
    Sleep(0);
}

/* Fork process */
/* Not supported */
extern int std_fork()
{
    return 0;
}

/* Start/restart a timer
 * index: which timer (currently, only 0 is supoorted)
 * msec - millisecond
 * If a previous timer is existed, auto cleared */
/* Return 1 means OK, 0 means failed */
static DWORD std_timerTimeoutTick;
static int   std_timerTimeoutFlag = 1; /* Default is timeout */
extern int   std_start_timer(int index, int msec)
{
    if (index != 0)
        /* Bad index, failed */
        return 0;

    if (msec >= STD_MAX_tIMER_tIMEOUT)
        /* Bad timeout time, trim it */
        msec = STD_MAX_tIMER_tIMEOUT;

    /* Set current tick, reset timeout flag */
    std_timerTimeoutTick = (std_tick_t) (GetTickCount() + msec);
    std_timerTimeoutFlag = 0;
    return 1;
}

/* Is the timer timeout?
 * This function can be deteced more then once
 * see: _stdStartTimer */
/* Return 1 means timeout, 0 means no yet */
extern int std_is_timer_timeout(int index)
{
    if (index != 0)
        /* Bad timer, return timeout */
        return 1;

    if (std_timerTimeoutFlag)
        /* Already timeout */
        return 1;

    if ((int) (GetTickCount() - std_timerTimeoutTick) >= 0)
        /* Now timeout, set flag */
        return (std_timerTimeoutFlag = 1);

    /* No timeout yet */
    return 0;
}

/* Stop the timer */
extern void std_clear_timer(int index)
{
    if (index != 0)
        /* Bad timer, return */
        return;

    /* Set timeout */
    std_timerTimeoutFlag = 1;
}

/* Get current working directory */
extern void std_get_cwd(char *path, size_t size)
{
    GetCurrentDirectory((DWORD) size, path);
}

/* Get temporary directory */
extern const char *std_get_temp_dir()
{
    static char tempDir[256];
    GetTempPath(sizeof(tempDir), tempDir);
    return tempDir;
}

/* Get error */
extern int std_get_error()
{
    return (int) errno;
}

/* Exit current process */
extern void std_end_process(int r)
{
    __debugbreak();
}

#endif  /* End of WIN32 */
