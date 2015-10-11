/******************************************************************
 *
 * @(#)std_port_debug.c:
 *
 * Purpose: 
 *  To support debug routine for some intertask-synchronize function.
 *
 * Functions:
 *
 * History:
 *  2006.7.17       Initial version
 *                  doing
 *
 ***** Copyright 2001, doing reserved *****************************/

#include "std_port/std_port.h"
#include "std_port/std_port_type.h"

/* Port-Debug information for each thread */
typedef struct std_port_debug_info
{
    std_task_id_t tid;
    void        *pSyncObject;
    const char  *memo;
} std_port_debug_info_t;

/* Max thread debug information node */
#define STD_MAX_tHREAD_COUNT         16

static std_tls_t             _debugTls;
static int                   _threadsIndex;
static std_port_debug_info_t _threadsDebugInfo[STD_MAX_tHREAD_COUNT + 1];

/* Internal routines */
static void _std_allocateThreadDebugInfoIndex();

extern int std_initPortDebug()
{
    if (! std_allocate_tls(&_debugTls))
        /* Failed to init debug tls */
        return 0;

    /* Set index from 0 */
    _threadsIndex = 0;
    return 1;
}

extern void std_shutdownPortDebug()
{
    std_free_tls(_debugTls);
}

/* Save the locking information */
extern void std_saveLockInfo(void *pSyncObject, const char *memo)
{
    IntR index;

    do
    {
        index = (IntR) std_get_tls_data(_debugTls);
        if (index >= 1 && index <= STD_MAX_tHREAD_COUNT)
            /* Right index */
            break;

        /* Allocate again */
        _std_allocateThreadDebugInfoIndex();
    } while (1);

    /* Save information */
    _threadsDebugInfo[index].tid = std_get_current_task_id();
    _threadsDebugInfo[index].pSyncObject = pSyncObject;
    _threadsDebugInfo[index].memo = memo;
}

/* Allocate node for current thread */
/* If node index over limit, just round back to overlap */
/* This routine may be confict between threads (_threadsIndex++ is not locked), it
 * doesn't matter */
static void _std_allocateThreadDebugInfoIndex()
{
    std_set_tls_data(_debugTls, (void *) (IntR) ((_threadsIndex++ % STD_MAX_tHREAD_COUNT) + 1));
}
