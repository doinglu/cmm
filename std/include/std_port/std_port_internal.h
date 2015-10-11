/******************************************************************
 *
 * @(#)std_port_internal.h:
 *
 * Purpose: 
 *  For std port internal function only
 *
 * Functions:
 *
 * History:
 *  2013.11.23      Initial version
 *                  doing
 *
 ***** Copyright 2001, doing reserved *****************************/

#ifndef __STD_PORT_INTERNAL_H__
#define __STD_PORT_INTERNAL_H__

#ifdef __cplusplus
extern "C" {
#endif
    
struct std_task_para;

extern void                  std_cleanup_all_tasks_name();
extern struct std_task_para *std_create_task_para(const char *name, void *entry, void *para);
extern const char           *std_get_task_name_by_id(std_task_id_t taskId, char *buf, size_t size);
extern int                   std_set_task_name(std_task_id_t taskId, const char *name);
extern void                 *std_task_entry(struct std_task_para *taskPara);

#ifdef __cplusplus
}
#endif

#endif  /* end of __STD_PORT_INTERNAL_H__ */
