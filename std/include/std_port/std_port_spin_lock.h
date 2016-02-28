/******************************************************************
 *
 * @(#)std_port_spin_lock.h:
 *
 * Purpose: 
 *  Header file to support spin-lock.
 *
 * Functions:
 *
 * History:
 *  2013.10.5       Initial version
 *                  doing
 *
 ***** Copyright 2001, doing reserved *****************************/

#ifndef __STD_PORT_SPIN_LOCK_H__
#define __STD_PORT_SPIN_LOCK_H__

#include "std_port_compiler.h"
#include "std_port_type.h"

#ifdef std_cpu_lock_xchg

/* Spin-lock */
typedef struct std_spin_lock
{
    AtomInt locked;
} std_spin_lock_t;

/* For auto initialized when declare */
#define INITED_SPIN_LOCK    { 0 }

#define std_init_spin_lock(lock) \
    (lock)->locked = 0;

#define std_release_spin_lock(lock) \
do \
{ \
    std_cpu_mfence(); \
    (lock)->locked = 0; \
} while (0)

#define std_destroy_spin_lock(lock)

#define std_get_spin_lock(lock) \
do \
{ \
    while (std_cpu_lock_xchg(&((lock)->locked), 1)) \
        std_cpu_pause(); \
} while (0)

#else /* std_port_cpu_lock_xchg is not defined*/

/* Use pthread_mutex instead */
#define std_spin_lock_t             std_critical_section_t
#define std_get_spin_lock(pl)       std_enter_critical_section(pl)
#define std_release_spin_lock(pl)   std_leave_critical_section(pl)
#define std_init_spin_lock(pl)      std_init_critical_section(pl)
#define std_destroy_spin_lock(pl)   std_destroy_critical_section(pl)

#endif /* End of std_port_cpu_lock_xchg */

#endif /* end of __STD_PORT_SPIN_LOCK_H__ */
