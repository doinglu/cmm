// std_socket_port.h
// 2013.10.2    Initial version by ershu
// 2015.10.17   Immigrate by doing

#include "std_os_socket.h"

#ifdef USE_OS_EPOLL_LINUX

int os_epoll_init()
{
    return 1;
}

void os_epoll_shutdown()
{
}

/* xpoll relative */
int os_epoll_create(__in int size)
{
    return epoll_create(size);
}

int os_epoll_ctl(__in int epfd, __in int op, __in int fd, __in struct epoll_event *event)
{
    return epoll_ctl(epfd, op, fd, event);
}

int os_epoll_wait(__in int epfd, __out struct epoll_event *events, __in int maxevents, __in int timeout)
{
    return epoll_wait(epfd, events, maxevents, timeout);
}

int os_epoll_close(__in int epfd)
{
    return close(epfd);
}

#endif /* End of #ifdef USE_OS_EPOLL_LINUX */
