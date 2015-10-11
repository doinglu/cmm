/******************************************************************
 *
 * @(#)std_port_cs.h:
 *
 * Purpose: 
 *  Header file to support portable critical section.
 *
 * Functions:
 *
 * History:
 *  2013.10.27      Initial version
 *                  doing
 *
 ***** Copyright 2001, doing reserved *****************************/

#ifndef __STD_PORT_CS_H__
#define __STD_PORT_CS_H__

#include "std_port_platform.h"

#ifdef _POSIX

#include <pthread.h>

/* Critical section structure */
typedef struct std_critical_section
{
    pthread_mutex_t section;
} std_critical_section_t;

#endif /* End of _POSIX */

#ifdef _WINDOWS

#include <windows.h>

/* Critical section structure */
typedef struct std_critical_section
{
    CRITICAL_SECTION section;
} std_critical_section_t;

#endif /* End of WIN32 */

#ifdef _VXWORKS

#include <vxworks.h>

/* Critical section structure */
typedef struct std_critical_section
{
    SEM_ID section;
} vm_critical_section_t;

#endif /* End of _VXWORKS */

#endif /* end of __STD_PORT_CS_H__ */
