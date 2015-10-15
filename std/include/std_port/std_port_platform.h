/******************************************************************
 *
 * @(#)std_port_platform.h:
 *
 * Purpose: 
 *  Header file to support portable.
 *
 * Functions:
 *
 * History:
 *  2006.2.23       Initial version
 *                  doing
 *
 ***** Copyright 2001, doing reserved *****************************/

#ifndef __STD_PORT_PLATFORM_H__
#define __STD_PORT_PLATFORM_H__

#ifdef _WIN32
#define _WINDOWS
#endif

#ifdef __APPLE__
#define _UNIX
#define _POSIX
#endif

#ifdef __linux__
#define _UNIX
#define _POSIX
#endif

#if defined(_MSC_VER)
    #define ARCH_INTEL
    #if defined(_M_IX86)
        #define ARCH_IA32
        #define PLATFORM32
    #elif defined(_M_AMD64) || defined(_M_X64)
        #define ARCH_X64
        #define PLATFORM64
    #else
        #error "Unknow arch"
    #endif
#elif defined(__GNUC__)
    #if defined(__i386__)
        #define ARCH_INTEL
        #define ARCH_IA32
        #define PLATFORM32
    #elif defined(__x86_64) || defined(__amd64__)
        #define ARCH_INTEL
        #define ARCH_X64
        #define PLATFORM64
    #elif defined(__arm__)
        #define ARCH_ARM
        #define PLATFORM32
    #else
        #error "Unknow arch"
    #endif
#else
#error "Unknow compiler"
#endif

// Align size for all types
#define STD_BEST_ALIGN_SIZE     16

#endif  /* end of __STD_PORT_tYPE_H__ */
