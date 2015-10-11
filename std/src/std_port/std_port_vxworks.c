/******************************************************************
 *
 * @(#)std_port_vxworks.c:
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

#ifdef _VXWORKS

#include "std_port/std_port.h"
#include "std_port/std_port_internal.h"

#include <semlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <tasklib.h>
#include <ticklib.h>
#include <time.h>

#ifndef STD_NO_MULTI_tHREAD

/* Under VxWorks */
extern int std_create_mutex(std_mutex_id_t *pMutexId)
{
    *pMutexId = semBCreate(SEM_Q_FIFO | SEM_DELETE_SAFE);
    if (*pMutexId == NULL)
        return 0;

    return 1;
}

extern int std_delete_mutex(std_mutex_id_t mutexId)
{
    if (mutexId == VM_NO_MUTEX)
        /* Return failed for operating empty object */
        return 0;

    if (semDelete(mutexId) != OK)
        return 0;

    return 1;
}

/* Create mutex before PDB up */
extern int std_createIdependentMutex(std_mutex_id_t *pMutexId)
{
    return std_create_mutex(pMutexId);
}

/* Delete mutex created by std_createIdependentMutex */
extern int std_deleteIdependentMutex(std_mutex_id_t mutexId)
{
    return std_delete_mutex(mutexId);
}

extern int std_get_mutex(std_mutex_id_t mutexId)
{
    if (mutexId == VM_NO_MUTEX)
        return 0;

    if (semTake(mutexId, WAIT_FOREVER) != OK)
        return 0;

    return 1;
}

extern int std_get_mutex_by_time(std_mutex_id_t mutexId, int timeout)
{
    if (mutexId == VM_NO_MUTEX)
        /* Return failed for operating empty object */
        return 0;

    if (semTake(mutexId, timeout) != OK)
        return 0;

    return 1;
}

extern int std_release_mutex(std_mutex_id_t mutexId)
{
    if (mutexId == VM_NO_MUTEX)
        /* Return failed for operating empty object */
        return 0;

    if (semGive(mutexId) != OK)
        return 0;

    return 1;
}

extern int std_forceReleaseMutex(std_mutex_id_t mutexId)
{
    if (mutexId == VM_NO_MUTEX)
        /* Return failed for operating empty object */
        return 0;

    if (semGive(mutexId) != OK)
        return 0;

    return 1;
}

extern int std_create_semaphore(std_semaphore_t *pSemId)
{
    *pSemId = semCreate(SEM_Q_FIFO, SEM_EMPTY, 0x7FFFFFFF);
    if (*pSemId == NULL)
        return 0;

    return 1;
}

extern int std_delete_semaphore(std_semaphore_t semId)
{
    if (semId == VM_NO_SEMAPHORE)
        /* Return failed for operating empty object */
        return 0;

    if (semDelete(semId) != OK)
        return 0;

    return 1;
}

extern int std_take_semaphore(std_semaphore_t semId)
{
    if (semId == VM_NO_SEMAPHORE)
        /* Return failed for operating empty object */
        return 0;

    if (semTake(semId, WAIT_FOREVER) != OK)
        return 0;

    return 1;
}

extern int std_take_semaphore_by_time(std_semaphore_t semId, int timeout)
{
    if (semId == VM_NO_SEMAPHORE)
        /* Return failed for operating empty object */
        return 0;

    if (semTake(semId, timeout) != OK)
        return 0;

    return 1;
}

extern int std_give_semaphore(std_semaphore_t semId)
{
    if (semId == VM_NO_SEMAPHORE)
        /* Return failed for operating empty object */
        return 0;

    if (semGive(semId) != OK)
        return 0;

    return 1;
}

extern int std_create_task(const char *name, std_pid_t *pTaskId,
                           void *entry, void *para)
{
    int taskId;
    int stack_size = std_get_default_task_stack_size();
    struct std_task_para *taskPara;

    taskPara = std_create_task_para(name, entry, para);
    if (taskPara == NULL)
        /* Can't create task para */
        return 0;

    VM_ASSERT(pEntry != NULL);

    if (! std_multiThreadBoot)
        VM_FATAL("Can't create any task before multiThreadBoot.\n"
                 "Use std_createTaskSmart() and std_startAllRequestedThreads() instead.\n");

    taskId = taskSpawn(name,            /* Task name        */
                       100,             /* Task priority    */
                       0,               /* Option           */
                       stack_size,       /* Stack size       */
                       std_task_entry,  /* Entry point      */
                       (int) para, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    if (taskId == ERROR)
        /* Failed to spawn task */
        return 0;

    /* Set return task id */
    if (pTaskId != NULL)
        *pTaskId = (std_pid_t) taskId;

    return 1;
}

extern void std_delete_task(std_pid_t taskId)
{
    if (taskId == VM_NON_tASK)
        /* Return for operating empty object */
        return;

    taskDelete((int) taskId);
}

/* Allocate thread local storage */
extern std_tls_t std_allocate_tls()
{
    // Not impletented, should be added, don't remove this line
}

/* Get data in thread local storage */
extern void *std_get_tls_data(std_tls_t tls)
{
    // Not impletented, should be added, don't remove this line
}

/* Put data into thread local storage */
extern void std_set_tls_data(std_tls_t tls, void *pTlsData)
{
    // Not impletented, should be added, don't remove this line
}

/* Free thread local storage */
extern void std_free_tls(std_tls_t tls)
{
    // Not impletented, should be added, don't remove this line
}

#endif /* End of VM_MULTI_tHREAD */

/* Return current task id */
extern std_pid_t std_get_current_task_id()
{
    return (std_pid_t) taskIdSelf();
}

/* Return process id */
extern std_pid_t std_get_current_process_id()
{
    return (std_pid_t) taskIdSelf(); // ???
}

/* Is process alive? */
extern int std_is_process_alive(std_pid_t pid)
{
    if (kill(pid, 0) == -1 && std_get_error() == ESRCH)
        /* Process is dead */
        return 0;

    return 1;
}

/* Sleep nms */
extern void std_sleep(int msec)
{
    taskDelay(msec);
}

/* Relinquish cpu */
extern void std_relinquish()
{
    taskRelinquish(0); // ???
}

/* Get current working directory */
extern void std_get_cwd(char *path, size_t size)
{
    getcwd(path, size);
}

/* Get temporary directory */
extern const char *std_get_temp_dir(char *path, size_t size)
{
    return "/tmp";
}

/* Get error */
extern int std_get_error();
{
    return errno;
}

/* Exit current process */
extern void std_end_process(int r)
{
    taskSuspend(r);
}

#endif /* End of _VXWORKS */
