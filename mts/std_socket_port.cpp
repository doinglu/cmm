// std_socket_port.cpp
// 2010.09.27   Initial version by doing
// 2015.10.17   Immigrate by doing

#if 0 ////----

#include "std_port/std_port.h"
#include "std_socket_bsd_errno.h"
#include "std_socket_port.h"
#include "std_os_socket.h"

/* There may be server type of socket.
 * If the highest 4 bits of fd (treat as 32bits number) isn't 0, the socket
 * fd isn't an raw OS socket, it can be a RUDP socket.
 */ 
int is_raw_socket(SOCKET socket_fd)
{
    return socket_fd >= 0 && (socket_fd & 0xF0000000) == 0;
}

/* Get raw socket */
/* Such as rudp, the raw socket is a udp socket */
SOCKET get_raw_socket(SOCKET socket_fd)
{
#if 0
    if (rudp_is_rudp_socket((int) socket_fd))
        return rudp_get_socket_fd((int) socket_fd);

    if (lcs_is_lcs_socket((int) socket_fd))
        return -1;
#endif

    return socket_fd;
}

int port_accept(SOCKET socket_fd, struct sockaddr *address, socklen_t *address_len)
{
#if 0
    if (rudp_is_rudp_socket((int) socket_fd))
        return rudp_accept((int) socket_fd, address, address_len);

    if (lcs_is_lcs_socket((int) socket_fd))
        return lcs_accept((int) socket_fd, address, address_len);
#endif

    return os_accept(socket_fd, address, (int*) address_len);
}

int port_bind(SOCKET socket_fd, const struct sockaddr *address, socklen_t address_len)
{
#if 0
    if (rudp_is_rudp_socket((int) socket_fd))
        return rudp_bind((int) socket_fd, address, address_len);

    if (lcs_is_lcs_socket((int) socket_fd))
        return lcs_bind((int) socket_fd, address, address_len);
#endif

    return os_bind(socket_fd, address, address_len);
}

int port_close(SOCKET socket_fd)
{
#if 0
    if (rudp_is_rudp_socket((int) socket_fd))
        return rudp_close((int) socket_fd);

    if (lcs_is_lcs_socket((int) socket_fd))
        return lcs_close((int) socket_fd);
#endif

    return os_socket_destroy(socket_fd);
}

int port_connect(SOCKET socket_fd, const struct sockaddr *address, socklen_t address_len)
{
#if 0
    if (rudp_is_rudp_socket((int) socket_fd))
        return rudp_connect((int) socket_fd, address, address_len);

    if (lcs_is_lcs_socket((int) socket_fd))
        return lcs_connect((int) socket_fd, address, address_len);
#endif

    return os_connect(socket_fd, address, address_len);
}

int port_getpeername(SOCKET socket_fd, struct sockaddr *address, socklen_t *address_len)
{
#if 0
    if (rudp_is_rudp_socket((int) socket_fd))
        return rudp_getpeername((int) socket_fd, address, address_len);

    if (lcs_is_lcs_socket((int) socket_fd))
        return lcs_getpeername((int) socket_fd, address, address_len);
#endif

    return os_getpeername(socket_fd, address, (int*) address_len);
}

int port_getsockname(SOCKET socket_fd, struct sockaddr *address, socklen_t *address_len)
{
#if 0
    if (rudp_is_rudp_socket((int) socket_fd))
        return rudp_getsockname((int) socket_fd, address, address_len);

    if (lcs_is_lcs_socket((int) socket_fd))
        return lcs_getsockname((int) socket_fd, address, address_len);
#endif

    return os_getsockname(socket_fd, address, (int*) address_len);
}

int port_getsockopt(SOCKET socket_fd, int level, int option_name, void *option_value, socklen_t *option_len)
{
#if 0
    if (rudp_is_rudp_socket((int) socket_fd))
        return rudp_getsockopt((int) socket_fd, level, option_name, option_value, option_len);

    if (lcs_is_lcs_socket((int) socket_fd))
        return lcs_getsockopt((int) socket_fd, level, option_name, option_value, option_len);
#endif

    return os_getsockopt(socket_fd, level, option_name, option_value, (int*) option_len);
}

int port_listen(SOCKET socket_fd, int backlog)
{
#if 0
    if (rudp_is_rudp_socket((int) socket_fd))
        return rudp_listen((int) socket_fd, backlog);

    if (lcs_is_lcs_socket((int) socket_fd))
        return lcs_listen((int) socket_fd, backlog);
#endif

    return os_listen(socket_fd, backlog);
}

int port_recv(SOCKET socket_fd, char *buffer, size_t length, int flags)
{
#if 0
    if (rudp_is_rudp_socket((int) socket_fd))
        return rudp_recv((int) socket_fd, buffer, length, flags);

    if (lcs_is_lcs_socket((int) socket_fd))
        return lcs_recv((int) socket_fd, buffer, length, flags);
#endif

    return os_recv(socket_fd, buffer, (int) length);
}

int port_recvfrom(
    SOCKET     socket_fd,
    char      *buffer,
    size_t     length,
    int        flags,
    struct     sockaddr *address,
    socklen_t *address_len)
{
#if 0
    if (rudp_is_rudp_socket((int) socket_fd))
        return -1;

    if (lcs_is_lcs_socket((int) socket_fd))
        return -1;
#endif

    return os_recvfrom(socket_fd, buffer, (int) length, flags, address, (int*) address_len);
}

int port_send(SOCKET socket_fd, const char *buffer, size_t length, int flags)
{
#if 0
    if (rudp_is_rudp_socket((int) socket_fd))
        return rudp_send((int) socket_fd, buffer, length, flags);

    if (lcs_is_lcs_socket((int) socket_fd))
        return lcs_send((int) socket_fd, buffer, length, flags);
#endif

    return os_send(socket_fd, buffer, (int) length);
}

int port_sendto(SOCKET socket_fd, const char *buffer, size_t length, int flags, const struct sockaddr *to, int tolen)
{
    if (! is_raw_socket(socket_fd))
    {
        SOCKET_SET_ERRNO(ENOTSOCK);
        return -1;
    }

    return os_sendto(socket_fd, buffer, (int) length, flags, to, tolen);
}

int port_setsockopt(SOCKET socket_fd, int level, int option_name, const void *option_value, socklen_t option_len)
{
#if 0
    if (rudp_is_rudp_socket((int) socket_fd))
        return rudp_setsockopt((int) socket_fd, level, option_name, option_value, option_len);

    if (lcs_is_lcs_socket((int) socket_fd))
        return lcs_setsockopt((int) socket_fd, level, option_name, option_value, option_len);
#endif

    return os_setsockopt(socket_fd, level, option_name, option_value, option_len);
}

SOCKET port_socket(int domain, int type, int protocol)
{
#if 0
    if (type == RUDP_SOCK_STREAM)
        return (SOCKET) rudp_socket(domain, type, protocol);

    if (type == LCS_SOCK_STREAM)
        return (SOCKET) lcs_socket(domain, type, protocol);
#endif

    return os_socket(domain, type, protocol);
}

/* Extend routines */

/* Is data arrived? */
/* Return 0 means there is data to read, -1 means not */
int port_isDataArrived(SOCKET socket_fd)
{
    Uint32 nIoRead;

#if 0
    if (rudp_is_rudp_socket((int) socket_fd))
    {
        /* For rudp socket */
        int r;
        if (rudp_select_single((int) socket_fd, &r, NULL, NULL) < 0 || ! r)
            /* Bad socket or there is not data to read */
            return -1;

        return 0;
    }

    if (lcs_is_lcs_socket((int) socket_fd))
    {
        /* For lcs socket */
        int r;
        if (lcs_select_single_only((int) socket_fd, &r, NULL, NULL) < 0 || ! r)
            /* Bad socket or there is not data to read */
            return -1;

        return 0;
    }
#endif

    if (os_ioctlsocket(socket_fd, FIONREAD, &nIoRead) == 0 && nIoRead > 0)
        /* Got data in recv buffer! */
        return 0;

    /* Use poll lib*/
    do
    {
        struct pollfd socketOperation;
        socketOperation.fd = socket_fd;
        socketOperation.events = POLLIN;
        socketOperation.revents = 0;
        nIoRead = os_poll(&socketOperation, 1, 0);
    } while (0);

    return nIoRead > 0 ? 0 : -1;
}

/* Wait a while for connection finished */
/* Return 0 means connected, -1 means failed */
int port_waitConnectFinished(SOCKET socket_fd, int sec)
{
    IntR       ret;
    int        optval;
    socklen_t  optlen;

#if 0
    if (rudp_is_rudp_socket((int) socket_fd))
    {
        std_tick_t start_tick, now_tick;
        int       w;

        /* For rudp socket */
        start_tick = std_get_current_tick();
        for (;;)
        {
            /* Scan single socket */
            rudp_scan_single((int) socket_fd);
            rudp_select_single((int) socket_fd, NULL, &w, NULL);
            if (w)
                /* Connected */
                return 0;

            now_tick = std_get_current_tick();
            if ((int) (now_tick - start_tick) > sec * 1000)
                /* Timeout */
                return -1;

            _sleep(1);
        }
    }

    if (lcs_is_lcs_socket((int) socket_fd))
    {
        /* For lcs socket, it will connected alway with pend mode */
        return -1;
    }
#endif

    /* Wait for the status-change */
    ret = -1;
    do
    {
        /* Use poll lib*/
        struct pollfd socketOperation;
        socketOperation.fd = socket_fd;
        socketOperation.events = POLLIN;
        socketOperation.revents = 0;
        if (os_poll(&socketOperation, 1, sec * 1000) < 1 ||
            socketOperation.revents & POLLERR)
            /* Failed to connect */
            break;

        optlen = sizeof(optval);
        if (os_getsockopt(socket_fd, SOL_SOCKET, SO_ERROR,
                          (char *) &optval, (int*) &optlen) < 0)
        {
            /* Failed to get result */
            break;
        }

        if (optval)
            /* Bad responding, failed to connect */
            break;

        ret = 0;
    } while (0);

    return (int) ret;
}

/* Wait a while for data arrived */
/* Return 0 means OK, -1 means failed */
int port_waitDataArrived(SOCKET socket_fd, int sec)
{
    int     n;
    Uint32  nIoRead;

#if 0
    if (rudp_is_rudp_socket((int) socket_fd))
    {
        std_tick_t start_tick, now_tick;
        int       r;

        /* For rudp socket */
        start_tick = std_get_current_tick();
        for (;;)
        {
            /* Scan single socket */
            rudp_scan_single((int) socket_fd);
            rudp_select_single((int) socket_fd, &r, NULL, NULL);
            if (r)
                /* Got arrived data */
                return 0;

            now_tick = std_get_current_tick();
            if ((int) (now_tick - start_tick) > sec * 1000)
                /* Timeout */
                return -1;

            _sleep(1);
        }
    }

    if (lcs_is_lcs_socket((int) socket_fd))
    {
        std_tick_t start_tick, now_tick;
        int       r;

        /* For lcs socket */
        start_tick = std_get_current_tick();
        for (;;)
        {
            /* Scan single socket */
            lcs_select_single_only((int) socket_fd, &r, NULL, NULL);
            if (r)
                /* Got arrived data */
                return 0;

            now_tick = std_get_current_tick();
            if ((int) (now_tick - start_tick) > sec * 1000)
                /* Timeout */
                return -1;

            _sleep(1);
        }
    }
#endif

    if (os_ioctlsocket(socket_fd, FIONREAD, &nIoRead) == 0 && nIoRead > 0)
        /* Data is waiting for read */
        return 0;

    /* Use poll lib*/
    do
    {
        struct pollfd socketOperation;
        socketOperation.fd = socket_fd;
        socketOperation.events = POLLIN;
        socketOperation.revents = 0;
        n = os_poll(&socketOperation, 1, 0);
    } while (0);

    if (n < 1)
    {
        /* No more data is arrived */
        /* Just return */
        return -1;
    }

    /* There is data arrived */
    return 0;
}

#endif ////----
