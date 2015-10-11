// std_port_util.h

#ifndef _STD_PORT_UTIL_H_
#define _STD_PORT_UTIL_H_

#ifdef _DEBUG
#define STD_DEBUG_SET_NULL(p)       p = NULL
#else
#define STD_DEBUG_SET_NULL(p)
#endif

#endif

