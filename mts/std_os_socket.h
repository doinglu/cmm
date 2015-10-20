// std_os_socket.h
// 2011.1.25    Initial version by ershu
// 2015.10.17   Immigrate by doing

#ifndef __STD_OS_SOCKET_H__
#define __STD_OS_SOCKET_H__

#include "std_socket_port.h"

#ifdef _WINDOWS
    #define USE_OS_SOCKET_WIN32                1
#else
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <sys/poll.h>
    #include <sys/ioctl.h>

    #define USE_OS_SOCKET_BSD                  1
#endif

#ifdef __linux
/* Linux support epoll */
#include <sys/epoll.h>
    #define USE_OS_EPOLL_LINUX
#else
/* Other system doesn't support epoll */
    #define USE_OS_EPOLL_SIM

    typedef union epoll_data {
        void    *ptr;
        int      fd;
        unsigned u32;
        unsigned long long u64;
    } epoll_data_t;

    struct epoll_event {
        unsigned     events;    /* Epoll events */
        epoll_data_t data;      /* User data variable */
    };

    #define EPOLL_CTL_ADD 1 /* Add a file decriptor to the interface.  */
    #define EPOLL_CTL_DEL 2 /* Remove a file decriptor from the interface.  */
    #define EPOLL_CTL_MOD 3 /* Change file decriptor epoll_event structure.  */

    #define EPOLLIN     POLLIN
    #define EPOLLOUT    POLLOUT
    #define EPOLLERR    POLLERR
    #define EPOLLHUP    POLLHUP

#endif

/* Definition for poll interface */
#ifdef _WINDOWS
#ifndef POLLIN

    /* The POLLIN is defined after _WIN32_WINNT >= 0x0600 */
    /* If the compiler is not support, use these definitions */

    #define POLLRDNORM  0x0100
    #define POLLRDBAND  0x0200
    #define POLLIN      (POLLRDNORM | POLLRDBAND)
    #define POLLPRI     0x0400

    #define POLLWRNORM  0x0010
    #define POLLOUT     (POLLWRNORM)
    #define POLLWRBAND  0x0020

    #define POLLERR     0x0001
    #define POLLHUP     0x0002
    #define POLLNVAL    0x0004

    struct pollfd
    {
        SOCKET  fd;
        SHORT   events;
        SHORT   revents;
    };

#endif /* POLLIN */

#define _POLLWAKEUP      0x7FFF
#endif /* WIN32 */

/* Define some macro to make parameter meaningful */
#ifndef __in
#define __in
#endif

#ifndef __out
#define __out
#endif

#ifndef __in_out
#define __in_out
#endif

/* init */
int os_socket_init(
);

/* shutdown */
void os_socket_shutdown(
);

/* accept */
int os_accept(
  __in          SOCKET s, 
  __out         struct sockaddr *addr, 
  __in_out      int *addrlen
);

/* bind */
int os_bind(
  __in          SOCKET s,
  __in          const struct sockaddr *name,
  __in          int namelen
);

/* connect */
int os_connect(
  __in          SOCKET s,
  __in          const struct sockaddr *name,
  __in          int namelen
);

/* gethostbyname */
int os_getaddrinfo(
  __in          const char *name,
  __in          const char *serv,
  __in          struct addrinfo *hints,
  __out         struct addrinfo **result
);

void os_freeaddrinfo(
  __in          struct addrinfo *ai
);

/* gethostname */
int os_gethostname(
  __out         char *name,
  __in          int namelen
);

/* getpeername */
int os_getpeername(
  __in          SOCKET s,
  __out         struct sockaddr *name,
  __in_out      int *namelen
);

/* getsockname */
int os_getsockname(
  __in          SOCKET s,
  __out         struct sockaddr *name,
  __in_out      int *namelen
);

/* getsockopt */
int os_getsockopt(
  __in          SOCKET s,
  __in          int level,
  __in          int optname,
  __out         void *optval,
  __in_out      int *optlen
);

/* ioctlsocket */
int os_ioctlsocket(
  __in          SOCKET s,
  __in          long cmd,
  __in_out      Uint32* argp
);

/* listen */
int os_listen(
  __in          SOCKET s,
  __in          int backlog
);

/* recv */
int os_recv(
  __in          SOCKET s,
  __out         char *buf,
  __in          int len
);

/* recvfrom */
int os_recvfrom(
  __in          SOCKET s,
  __out         char *buf,
  __in          int len,
  __in          int flags,
  __out         struct sockaddr *from,
  __in_out      int *fromlen
);

/* select */
int os_select(
  __in          int nfds,
  __in_out      fd_set *readfds,
  __in_out      fd_set *writefds,
  __in_out      fd_set *exceptfds,
  __in          const struct timeval* timeout
);

/* poll */
int os_poll(
  __in_out      struct pollfd fdarray[],
  __in          int nfds,
  __in          int timeout                 /* ms */
);

/* send */
int os_send(
  __in          SOCKET s,
  __in          const char *buf,
  __in          int len
);

/* sendto */
int os_sendto(
  __in          SOCKET s,
  __in          const char *buf,
  __in          int len,
  __in          int flags,
  __in          const struct sockaddr *to,
  __in          int tolen
);

/* setsockopt */
int os_setsockopt(
  __in          SOCKET s,
  __in          int level,
  __in          int optname,
  __in          const void *optval,
  __in          int optlen
);

/* socket_destroy */
int os_socket_destroy(
  __in          SOCKET s
);

/* socket */
SOCKET os_socket(
  __in          int af,
  __in          int type,
  __in          int protocol
);

/* epoll relative */

/* For vm_os_socket_xxx */

int os_epoll_init(
);

void os_epoll_shutdown(
);

/* Interface */

int os_epoll_create(
  __in          int size
);

int os_epoll_ctl(
  __in          int epfd,
  __in          int op,
  __in          int fd,
  __in          struct epoll_event *event
);

int os_epoll_wait(
  __in          int epfd,
  __out         struct epoll_event *events,
  __in          int maxevents,
  __in          int timeout
);

int os_epoll_close(
  __in          int epfd
);

#endif /* __STD_OS_SOCKET_H__ */
