/******************************************************************
 *
 * @(#)std_port_compiler.h:
 *
 * Purpose: 
 *  Header file to define interface for GCC/MSVC compilers
 *
 * Functions:
 *
 * History:
 *  2013.10.20      Initial version
 *                  doing
 *
 ***** Copyright 2001, doing reserved *****************************/

#ifndef __STD_PORT_COMPILER_H__
#define __STD_PORT_COMPILER_H__

#ifdef __GNUC__
/* Under GCC */
#define std_cpu_lock_xchg(ptr, val)                 __sync_lock_test_and_set(ptr, val)
#define std_cpu_lock_cas(ptr, oldval, newval)       __sync_bool_compare_and_swap(ptr, oldval, newval)
#define std_cpu_lock_add(ptr, val)                  __sync_fetch_and_add(ptr, val)
#define std_cpu_pause()                             __asm("pause")
#define std_cpu_mfence()                            __asm("mfence")

#define STD_BEGIN_ALIGNED_STRUCT(n)
#define STD_END_ALIGNED_STRUCT(n)                   __attribute__ ((aligned(n)))
#endif

#ifdef _MSC_VER
/* Under MSC (commonly windows) */
#ifdef _M_X64
/* For X64 */
#define std_cpu_lock_xchg(ptr, val)                 _InterlockedExchange64(ptr, val)
#define std_cpu_lock_cas(ptr, oldval, newval)       (_InterlockedCompareExchange64(ptr, newval, oldval) == (__int64) oldval)
#define std_cpu_lock_add(ptr, val)                  _InterlockedExchangeAdd64(ptr, val)
#else
/* For IA32 */
#define std_cpu_lock_xchg(ptr, val)                 _InterlockedExchange(ptr, val)
#define std_cpu_lock_cas(ptr, oldval, newval)       (_InterlockedCompareExchange(ptr, newval, oldval) == (long) oldval)
#define std_cpu_lock_add(ptr, val)                  _InterlockedExchangeAdd(ptr, val)
#endif /* En of _M_X64 */
#define std_cpu_pause()                             _mm_pause()
#define std_cpu_mfence()                            _mm_mfence()

#define STD_BEGIN_ALIGNED_STRUCT(n)                 __declspec(align(n))
#define STD_END_ALIGNED_STRUCT(n)
#endif /* End of _MSC_VER */

#ifndef STD_BEGIN_ALIGNED_STRUCT
#define STD_BEGIN_ALIGNED_STRUCT(n)
#define STD_END_ALIGNED_STRUCT(n)
#endif

#endif /* end of __STD_PORT_COMPILER_H__ */
