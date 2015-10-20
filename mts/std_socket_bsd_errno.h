// std_socket_bsd_errno.h
// Map BSD socket error
// 2015.10.17   Immigrate by doing

#if 0

#ifdef _WINDOWS
#ifndef __MAP_WSA_ERR_TO_BSD_ERR__
#define __MAP_WSA_ERR_TO_BSD_ERR__

#ifndef EAGAIN
#define EAGAIN                      WSAEWOULDBLOCK
#endif

#define EWOULDBLOCK                 WSAEWOULDBLOCK
#define EINPROGRESS                 WSAEINPROGRESS

#ifndef EINTR
#define EINTR                       WSAEINTR
#endif

#define EALREADY                    WSAEALREADY
#define ENOTSOCK                    WSAENOTSOCK
#define EDESTADDRREQ                WSAEDESTADDRREQ
#define EMSGSIZE                    WSAEMSGSIZE
#define EPROTOTYPE                  WSAEPROTOTYPE
#define ENOPROTOOPT                 WSAENOPROTOOPT
#define EPROTONOSUPPORT             WSAEPROTONOSUPPORT
#define ESOCKTNOSUPPORT             WSAESOCKTNOSUPPORT
#define EOPNOTSUPP                  WSAEOPNOTSUPP
#define EPFNOSUPPORT                WSAEPFNOSUPPORT
#define EAFNOSUPPORT                WSAEAFNOSUPPORT
#define EADDRINUSE                  WSAEADDRINUSE
#define EADDRNOTAVAIL               WSAEADDRNOTAVAIL
#define ENETDOWN                    WSAENETDOWN
#define ENETUNREACH                 WSAENETUNREACH
#define ENETRESET                   WSAENETRESET
#define ECONNABORTED                WSAECONNABORTED
#define ECONNRESET                  WSAECONNRESET
#define ENOBUFS                     WSAENOBUFS
#define EISCONN                     WSAEISCONN
#define ENOTCONN                    WSAENOTCONN
#define ESHUTDOWN                   WSAESHUTDOWN
#define ETOOMANYREFS                WSAETOOMANYREFS
#define ETIMEDOUT                   WSAETIMEDOUT
#define ECONNREFUSED                WSAECONNREFUSED
#define ELOOP                       WSAELOOP
#define EHOSTDOWN                   WSAEHOSTDOWN
#define EHOSTUNREACH                WSAEHOSTUNREACH
#define EUSERS                      WSAEUSERS
#define EDQUOT                      WSAEDQUOT
#define ESTALE                      WSAESTALE
#define EREMOTE                     WSAEREMOTE
#define ENOMEM                      60010500
#define EEXIST                      60010501
#define ENOENT                      60010502
#define EBADF                       60010503

#endif /* __MAP_WSA_ERR_TO_BSD_ERR__ */
#endif

#endif
