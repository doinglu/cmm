/******************************************************************
 *
 * @(#)std_port_type.h:
 *
 * Purpose: 
 *  Define types
 *
 * Functions:
 *
 * History:
 *  2015.8.31       Initial version
 *                  doing
 *
 ***** Copyright 2001, doing reserved *****************************/

#ifndef __STD_PORT_TYPE_H__
#define __STD_PORT_TYPE_H__

#include "std_port_platform.h"

#ifndef Int8
#define Int8    char                
#define Int16   short               
#define Int32   int
#define Int64   long long
#endif

#ifndef Uint8
#define Uint8   unsigned char
#define Uint16  unsigned short
#define Uint32  unsigned int
#define Uint64  unsigned long long
#endif

#ifdef PLATFORM64
#define IntR    Int64
#define UintR   Uint64
#else
#define IntR    Int32
#define UintR   Uint32
#endif

#define AtomInt UintR

#endif  /* end of __STD_PORT_tYPE_H__ */
