/******************************************************************
 *
 * @(#)std_port_os.h:
 *
 * Purpose: 
 *  Header file to include os dependencies header files.
 *
 * Functions:
 *
 * History:
 *  2015.9.9        Initial version
 *                  doing
 *
 ***** Copyright 2001, doing reserved *****************************/

#ifndef __STD_PORT_OS_H__
#define __STD_PORT_OS_H__

#include "std_port_platform.h"

#ifdef _POSIX

#include <pthread.h>

#endif /* End of _POSIX */

#ifdef _WINDOWS

#include <windows.h>
#include <emmintrin.h>

#endif /* End of WIN32 */

#ifdef _VXWORKS

#include <vxworks.h>

#endif

#endif /* end of __STD_PORT_CS_H__ */
