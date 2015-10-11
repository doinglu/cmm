/******************************************************************
 *
 * @(#)std_port_nothread.c:
 *
 * Purpose: 
 *  To support some intertask-synchronize function.
 *
 * Functions:
 *
 * History:
 *  2002.1.2        Initial version
 *                  doing
 *
 ***** Copyright 2001, doing reserved *****************************/

#include "std_port/std_port.h"

#ifdef STD_NO_MULTI_tHREAD

extern int std_create_mutex(std_mutex_id_t *pMutexId)
{
    *pMutexId = (std_mutex_id_t) 1234;
    return 1;
}

extern int std_delete_mutex(std_mutex_id_t mutexId)
{
    return 1;
}

extern int std_get_mutex(std_mutex_id_t mutexId)
{
    if (mutexId == STD_NO_MUTEX)
        return 0;

    return 1;
}

extern int std_get_mutex_by_time(std_mutex_id_t mutexId, int timeout)
{
    return std_get_mutex(mutexId);
}

extern int std_release_mutex(std_mutex_id_t mutexId)
{
    return 1;
}

extern int std_forceReleaseMutex(std_mutex_id_t mutexId)
{
    return 1;
}

/* Initialize a critical section */
extern void std_init_critical_section(std_critical_section_t *pSection)
{
    STD_ASSERT(pSection != NULL);
}

/* Destroy the critical section */
extern void std_destroy_critical_section(std_critical_section_t *pSection)
{
    STD_ASSERT(pSection != NULL);
}

/* Enter critical section */
extern void std_enter_critical_section(std_critical_section_t *pSection)
{
    STD_ASSERT(pSection != NULL);
}

/* Try enter critical section */
extern int std_try_enter_critical_section(std_critical_section_t *pSection)
{
    STD_ASSERT(pSection != NULL);
    return 1;
}

/* Leave critical section */
extern void std_leave_critical_section(std_critical_section_t *pSection)
{
    STD_ASSERT(pSection != NULL);
}

extern int std_create_semaphore(std_semaphore_id_t *pSemId)
{
    *pSemId = (std_semaphore_id_t) std_malloc(sizeof(int));
    if (*pSemId == NULL)
        return 0;

    return 1;
}

extern int std_delete_semaphore(std_semaphore_id_t semId)
{
    if (semId == STD_NO_SEMAPHORE)
        return 0;

    std_free((void *) semId);
    return 1;
}

extern int std_take_semaphore(std_semaphore_id_t semId)
{
    if (*(int *) semId > 0)
    {
        (*(int *) semId)--;
        return 1;
    }

    return 0;
}

extern int std_take_semaphore_by_time(std_semaphore_id_t semId, int timeout)
{
    return std_take_semaphore(semId);
}

extern int std_give_semaphore(std_semaphore_id_t semId)
{
    if (*(int *) semId < 0x7FFFFFFF)
    {
        (*(int *) semId)++;
        return 1;
    }

    return 0;
}

extern int std_create_event(std_event_id_t *pEventId)
{
    return 1;
}

extern int std_delete_event(std_event_id_t eventId)
{
    return 1;
}


extern int std_wait_event(std_event_id_t eventId)
{
    return 1;
}

extern int std_waitEventByTime(std_event_id_t eventId, int timeout)
{
    return 1;
}

extern int std_raise_event(std_event_id_t eventId)
{
    return 1;
}

extern int std_create_proc_sem(std_proc_sem_Id_t *pProcSemId, const char *name)
{
    return 1;
}

extern int std_open_proc_sem(std_proc_sem_Id_t *pProcSemId, const char *name)
{
    return 1;
}

extern int std_close_proc_sem(std_proc_sem_Id_t semId)
{
    return 1;
}

extern int std_take_proc_sem(std_proc_sem_Id_t semId)
{
    return 1;
}

extern int std_take_proc_sem_by_time(std_proc_sem_Id_t semId, int timeout)
{
    return 1;
}

extern int std_give_proc_sem(std_proc_sem_Id_t semId)
{
    return 1;
}

extern int std_create_shrd_mem(std_shrd_mem_id_t *pMemId, size_t size, const char *name)
{
    return 1.
}

extern int std_open_shrd_mem(std_shrd_mem_id_t *pMemId, size_t size, const char *name)
{
    return 1;
}

extern int std_close_shrd_mem(std_shrd_mem_id_t memId)
{
    return 1;
}

extern int std_map_shrd_mem(std_shrd_mem_id_t memId, size_t size, void **ppMapAt)
{
    *ppMapAt = std_malloc(size);
    return 1;
}

extern int std_unmap_shrd_mem(std_shrd_mem_id_t memId, void *pMapAt, size_t size)
{
    std_free(pMapAt);
    return 1;
}

extern int std_create_task(const char *name, Vm_task_Id_t *pTaskId,
                           void *entry, void *para)
{
    STD_ASSERT(pTaskId != NULL);
    STD_ASSERT(entry != NULL);

    if (! std_multiThreadBoot)
        STD_FATAL("Can't create any task before multiThreadBoot.\n"
                 "Use std_createTaskSmart() and std_startAllRequestedThreads() instead.\n");

    if (pTaskId != NULL)
        *pTaskId = 0x1255;

    STD_REFER(para);
    STD_REFER(name);
    return 1;
}

extern void std_delete_task(Vm_task_Id_t taskId)
{
    STD_ASSERT(taskId == 0x1255);
    STD_REFER(taskId);
}

/* Return process id */
extern Vm_Pid_Id_t std_get_current_process_id()
{
    return (Vm_Pid_Id_t) getpid();
}

/* Relinquish CPU */
extern void std_relinquish()
{
    /* Do nothing */
}

#endif  /* End of ~STD_MULTI_tHREAD */
