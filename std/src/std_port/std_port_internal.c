/******************************************************************
 *
 * @(#)std_port_internal.c:
 *
 * Purpose: 
 *  Std port internal use only
 *
 * Functions:
 *
 * History:
 *  2013.11.23      Initial version
 *                  doing
 *
 ***** Copyright 2001, doing reserved *****************************/

#include "std_port/std_port.h"

#include <stdio.h>
#include <string.h>
#include "std_port/std_port_internal.h"

/* Internal routines */
static void std_delete_task_name(std_task_id_t taskId);
static int  std_find_task_id(std_task_id_t taskId);

/* For task name */
typedef struct std_task_id_name
{
    std_task_id_t taskId;
    char          taskName[STD_MAX_tASK_NAME_LEN];
} std_task_id_name_t;
static std_task_id_name_t *std_task_id_name_map = NULL;
static int std_task_id_name_size = 0;
static int std_task_id_name_max_size = 0;
static std_spin_lock_t std_tin_spin_lock = INITED_SPIN_LOCK;

/* Start parameters */
typedef struct std_task_para
{
    char    name[STD_MAX_tASK_NAME_LEN];
    void   *entry;
    void   *para;
} std_task_para_t;

/* Free all tasks' name */
extern void std_cleanup_all_tasks_name()
{
    std_get_spin_lock(&std_tin_spin_lock);

    /* Free map */
    std_free(std_task_id_name_map);
    std_task_id_name_map = NULL;
    std_task_id_name_size = 0;
    std_task_id_name_max_size = 0;

    std_release_spin_lock(&std_tin_spin_lock);
}

/* Allocate & create task para for task entry */
extern std_task_para_t *std_create_task_para(const char *name, void *entry, void *para)
{
    std_task_para_t *taskPara;

    taskPara = std_malloc(sizeof(std_task_para_t));
    if (taskPara == NULL)
        /* Can't not create task para */
        return NULL;

    memset(taskPara, 0, sizeof(std_task_para_t));
    if (name != NULL)
    {
        strncpy(taskPara->name, name, sizeof(taskPara->name));
        taskPara->name[sizeof(taskPara->name) - 1] = 0;
    }
    taskPara->entry = entry;
    taskPara->para = para;

    return taskPara;
}

/* Find & return name */
extern const char *std_get_task_name_by_id(std_task_id_t taskId, char *buf, size_t size)
{
    const char *taskName;
    int index;

    if (size > STD_MAX_tASK_NAME_LEN)
        size = STD_MAX_tASK_NAME_LEN + 1;

    std_get_spin_lock(&std_tin_spin_lock);
    do
    {
        index = std_find_task_id(taskId);
        if (index >= std_task_id_name_size ||
            std_task_id_name_map[index].taskId != taskId)
        {
            taskName = "N/A";
            break;
        }

        taskName = std_task_id_name_map[index].taskName;
    } while (0);
    strncpy(buf, taskName, size);
    buf[size - 1] = 0;
    std_release_spin_lock(&std_tin_spin_lock);

    return buf;
}

/* Set task id -> name */
extern int std_set_task_name(std_task_id_t taskId, const char *name)
{
    int index;
    int ret = 0;

    std_get_spin_lock(&std_tin_spin_lock);
    do
    {
        /* Get index to set/insert new name */
        index = std_find_task_id(taskId);
        STD_ASSERT(index >= 0 && index <= std_task_id_name_size);
        if (index < std_task_id_name_size &&
            std_task_id_name_map[index].taskId == taskId)
        {
            /* Replace previous one */
            strncpy(std_task_id_name_map[index].taskName, name,
                    STD_MAX_tASK_NAME_LEN);
            break;
        }

        /* Insert @ index */

        if (std_task_id_name_size >= std_task_id_name_max_size)
        {
            std_task_id_name_t *new_name_map;
            int                 new_size;

            /* Expand map */
            new_size = std_task_id_name_max_size * 2;
            if (new_size < 16)
                new_size = 16;
            new_name_map = std_malloc(sizeof(std_task_id_name_t) * new_size);
            if (new_name_map == NULL)
            {
                /* Failed to allocate memory to add task name */
                break;
            }

            /* Free old & replace with new one */
            if (std_task_id_name_map != NULL)
            {
                memcpy(new_name_map, std_task_id_name_map,
                       sizeof(std_task_id_name_t) * std_task_id_name_max_size);
                std_free(std_task_id_name_map);
            }
            std_task_id_name_map = new_name_map;
            std_task_id_name_max_size = new_size;
        }

        /* Put item @ index */
        memmove(&std_task_id_name_map[index + 1], 
                &std_task_id_name_map[index],
                sizeof(std_task_id_name_t) * (std_task_id_name_size - index));
        std_task_id_name_map[index].taskId = taskId;
        strncpy(std_task_id_name_map[index].taskName, name,
                STD_MAX_tASK_NAME_LEN);
        std_task_id_name_size++;
        ret = 1;
    } while (0);
    std_release_spin_lock(&std_tin_spin_lock);

    return ret;
}

/* Entry of os thread */
extern void *std_task_entry(std_task_para_t *taskPara)
{
    void *entry;
    void *para;
    void *ret;

    std_set_task_name(std_get_current_task_id(), taskPara->name);

    /* Save to stack */
    entry = taskPara->entry;
    para = taskPara->para;

    /* Drop input parameter */
    std_free(taskPara);

    /* Enter */
    ret = ((void *(*)(void *)) entry)(para);

    /* Free name when thread exit */
    std_delete_task_name(std_get_current_task_id());

    return ret;
}

/* Remove task id -> name */
static void std_delete_task_name(std_task_id_t taskId)
{
    int index;

    std_get_spin_lock(&std_tin_spin_lock);

    index = std_find_task_id(taskId);
    STD_ASSERT(index >= 0 && index <= std_task_id_name_size);
    if (index < std_task_id_name_size &&
        std_task_id_name_map[index].taskId == taskId)
    {
        /* Erase the entry */
        memmove(&std_task_id_name_map[index],
                &std_task_id_name_map[index + 1],
                sizeof(std_task_id_name_t) * (std_task_id_name_size - index - 1));
        std_task_id_name_size--;
    }

    std_release_spin_lock(&std_tin_spin_lock);    
}

/* Return the nearest index of taskId */
/* For example: { 1, 5, 7, 9, 10, 115 }
 * find(0)   = 0
 * find(1)   = 1
 * find(6)   = 2
 * find(118) = 6
 */
static int std_find_task_id(std_task_id_t taskId)
{
    int b, e, m;

    STD_ASSERT(std_tin_spin_lock.locked);
    STD_ASSERT(std_task_id_name_map != NULL || std_task_id_name_size == 0);
    b = 0;
    e = std_task_id_name_size;
    while (b != e)
    {
        m = (b + e) >> 1;
        if (taskId > std_task_id_name_map[m].taskId)
        {
            b = m + 1;
        } else
        {
            e = m;
        }
    }

    return e;
}

