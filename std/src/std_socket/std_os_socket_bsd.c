// std_socket_port.h
// 2011.1.25    Initial version by ershu
// 2015.10.17   Immigrate by doing

#include "std_os_socket.h"

#ifdef USE_OS_SOCKET_BSD

#include <netdb.h>

/* init */
int os_socket_init()
{
    if (! os_epoll_init())
    {
        VM_FATAL("Failed to initialize os_epoll.\n");
        return 0;
    }

    return 1;
}

/* shutdown */
void os_socket_shutdown()
{
    os_epoll_shutdown();
}

/* accept */
int os_accept(__in SOCKET s, __out struct sockaddr *addr, __in_out int *addrlen)
{
    return accept(s, addr, (socklen_t*) addrlen);
}

/* bind */
int os_bind(__in SOCKET s, __in const struct sockaddr *name, __in int namelen)
{
    return bind(s, name, namelen);
}

/* connect */
int os_connect(__in SOCKET s, __in const struct sockaddr *name, __in int namelen)
{
    return connect(s, name, namelen);
}

/* getaddrinfo */
int os_getaddrinfo(__in const char *name, __in const char *serv, __in struct addrinfo *hints, __out struct addrinfo **result)
{
    return getaddrinfo(name, serv, hints, result);
}

/* freehostaddr */
void os_freeaddrinfo(__in struct addrinfo *ai)
{
    freeaddrinfo(ai);
}

/* gethostname */
int os_gethostname(__out char *name, __in int namelen)
{
    return gethostname(name, namelen);
}

/* getpeername */
int os_getpeername(__in SOCKET s, __out struct sockaddr *name, __in_out int *namelen)
{
    return getpeername(s, name, (socklen_t*) namelen);
}

/* getsockname */
int os_getsockname(__in SOCKET s, __out struct sockaddr *name, __in_out int *namelen)
{
    return getsockname(s, name, (socklen_t*) namelen);
}

/* getsockopt */
int os_getsockopt(__in SOCKET s, __in int level, __in int optname, __out void *optval, __in_out int *optlen)
{
    return getsockopt(s, level, optname, optval, (socklen_t*) optlen);
}

/* ioctlsocket */
int os_ioctlsocket(__in SOCKET s, __in long cmd, __in_out Uint32* argp)
{
    return ioctl(s, cmd, (caddr_t) (argp));
}

/* listen */
int os_listen(__in SOCKET s, __in int backlog)
{
    return listen(s, backlog);
}

/* recv */
int os_recv(__in SOCKET s, __out char *buf, __in int len)
{
    return read(s, buf, len);
}

/* recvfrom */
int os_recvfrom(__in SOCKET s, __out char *buf, __in int len, __in int flags, __out struct sockaddr *from, __in_out int *fromlen)
{
    return recvfrom(s, buf, len, flags, from, (socklen_t*) fromlen);
}

/* select */
int os_select(__in SOCKET nfds, __in_out fd_set *readfds, __in_out fd_set *writefds, __in_out fd_set *exceptfds, __in const struct timeval* timeout)
{
    VM_VERIFY(0 && "Please do NOT use select modal!! Use os_poll instead !!");
    return 0;
}

/* poll */
int os_poll(__in_out struct pollfd fdarray[], __in int nfds, __in int timeout)
{
    return poll(fdarray, nfds, timeout);
}

/* send */
int os_send(__in SOCKET s, __in const char *buf, __in int len)
{
    return write(s, buf, len);
}

/* sendto */
int os_sendto(__in SOCKET s, __in const char *buf, __in int len, __in int flags, __in const struct sockaddr *to, __in int tolen)
{
    return sendto(s, buf, len, flags, to, tolen);
}

/* setsockopt */
int os_setsockopt(__in SOCKET s, __in int level, __in int optname, __in const void *optval, __in int optlen)
{
    return setsockopt(s, level, optname, optval, optlen);
}

/* socket_destroy */
int os_socket_destroy(__in SOCKET s)
{
    /* Clean up socket */
    return close(s);
}

/* socket */
int os_socket(__in int af, __in int type, __in int protocol)
{
    return socket(af, type, protocol);
}

#endif /* End of #ifdef USE_OS_SOCKET_BSD */

