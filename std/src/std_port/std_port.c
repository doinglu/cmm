/******************************************************************
 *
 * @(#)std_port.c:
 *
 * Purpose: 
 *  To support some derived portable functions
 *  The function for VM use only.
 *
 * Functions:
 *
 * History:
 *  2006.10.18      Initial version
 *                  doing
 *
 ***** Copyright 2001, doing reserved *****************************/

#include "std_port/std_port.h"

#include <stdlib.h>
#include <time.h>
#include "std_port/std_port_cs.h"
#include "std_port/std_port_internal.h"

#define DEFAULT_TASK_STACK_SIZE         (256 * 1024)

static size_t _default_task_stack_size = DEFAULT_TASK_STACK_SIZE;

/* For task name */
std_tls_t std_task_name_tls_id = STD_NO_TLS_ID;

/* Get the tick  */
extern std_tick_t std_get_current_tick()
{
    return std_get_os_tick();
}

/* Get the high resolution system freqency-counter */
extern std_freq_t std_get_current_us_counter()
{
    return std_get_os_us_counter();
}

/* Set the stack size of task(thread) */
extern void std_set_default_task_stack_size(size_t size)
{
    _default_task_stack_size = size;
}

/* Get the stack size of task(thread) */
extern size_t std_get_default_task_stack_size()
{
    return _default_task_stack_size;
}

/* Return name of specified task name */
extern const char *std_get_task_name(std_task_id_t taskId, char *buf, size_t size)
{
    return std_get_task_name_by_id(taskId, buf, size);
}

/* Set current task's name */
extern int std_set_current_task_name(const char *name)
{
    return std_set_task_name(std_get_current_task_id(), name);
}

/* Memory routins: allocate/free */
static std_malloc_t std_malloc_func = malloc;
static std_free_t   std_free_func = free;

void *std_malloc(size_t size)
{
    return std_malloc_func(size);
}

void std_free(void *p)
{
    std_free_func(p);
}

/* Before init, user should not call std functions except critical section routines */
void std_init(std_malloc_t malloc_func, std_free_t free_func)
{
    if (malloc_func == NULL)
    {
        malloc_func = malloc;
        free_func = free;
    }

    std_malloc_func = malloc_func;
    std_free_func = free_func;
}

/* System is shutdown */
void std_shutdown()
{
    /* Cleanup the task id -> name map */
    std_cleanup_all_tasks_name();
}

/* Function from macro:std_get_spin_lock */
void std_get_spin_lock_impl(std_spin_lock_t *lock)
{
    std_get_spin_lock(lock);
}

/* Function from macro:std_release_spin_lock */
void std_release_spin_lock_impl(std_spin_lock_t *lock)
{
    std_release_spin_lock(lock);
}

/* Allocate a critcal section */
int std_new_critical_section(struct std_critical_section **ppSection)
{
    struct std_critical_section *pSection = std_malloc_func(sizeof(std_critical_section_t));
    if (pSection == NULL)
        // Failed to allocate
        return 0;

    std_init_critical_section(pSection);
    *ppSection = pSection;
    return 1;
}

/* Delete a critical section */
void std_delete_critical_section(struct std_critical_section *pSection)
{
    std_destroy_critical_section(pSection);
    std_free_func(pSection);
}
