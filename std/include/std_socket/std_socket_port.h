// std_socket_port.h
// 2010.09.27   Initial version by doing
// 2015.10.17   Immigrate by doing

#ifndef __STD_SOCKET_PORT_H__
#define __STD_SOCKET_PORT_H__

#include "std_port/std_port.h"
#include "std_port/std_port_platform.h"

#define STD_MAX_POLL_SOCKETS    4096

#ifdef _WINDOWS

/* Include winsock.h */

#include <windows.h>
//#include <winsock2.h>

/* Max sockets count (per page) when scanning for winsock */
#define MAX_POLL_SOCKETS         (FD_SETSIZE - 1)

typedef int FAR socklen_t;

#define SOCKET_ERRNO()           WSAGetLastError()
#define SOCKET_SET_ERRNO(no)     WSASetLastError(no);

#else

#ifndef _VXWORKS

/* Normal UNIX */

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>

#ifndef SOCKET
#define SOCKET                      int
#endif
#define SOCKET_ERRNO()              errno
#define SOCKET_SET_ERRNO(no)        errno = no;

/* Max sockets count (per page) when scanning for poll */
#define MAX_POLL_SOCKETS         4096

#define HOSTENT     struct hostent

#else

/* vxWorks */

#include <errno.h>
#include <errnoLib.h>
#include <fcntl.h>
#include <inetLib.h>
#include <ioLib.h>
#include <netLib.h>
#include <sockLib.h>

extern int errno;

#define SOCKET                      IntR
#define SOCKET_ERRNO()              errno
#define SOCKET_SET_ERRNO(no)        errno = no;

/* Max sockets count (per page) when scanning for BSD socket */
#define MAX_POLL_SOCKETS         1024

#define HOSTENT     struct hostent

#endif /* End of UNIX & vxWorks */

#endif /* End of all OS */

/* Utilities function */
int is_raw_socket(SOCKET socketFd);
SOCKET get_raw_socket(SOCKET socketFd);

/* Socket operations */
int port_accept(SOCKET socketFd, struct sockaddr *address, socklen_t *address_len);
int port_bind(SOCKET socketFd, const struct sockaddr *address, socklen_t address_len);
int port_close(SOCKET socketFd);
int port_connect(SOCKET socketFd, const struct sockaddr *address, socklen_t address_len);
int port_getpeername(SOCKET socketFd, struct sockaddr *address, socklen_t *address_len);
int port_getsockname(SOCKET socketFd, struct sockaddr *address, socklen_t *address_len);
int port_getsockopt(SOCKET socketFd, int level, int option_name, void *option_value, socklen_t *option_len);
int port_listen(SOCKET socketFd, int backlog);
int port_recv(SOCKET socketFd, char *buffer, size_t length, int flags);
int port_recvfrom(SOCKET socketFd, char *buffer, size_t length, int flags, struct sockaddr *address, socklen_t *address_len);
int port_send(SOCKET socketFd, const char *buffer, size_t length, int flags);
int port_sendto(SOCKET socketFd, const char *buffer, size_t length, int flags, const struct sockaddr *to, int tolen);
int port_setsockopt(SOCKET socketFd, int level, int option_name, const void *option_value, socklen_t option_len);
SOCKET port_socket(int domain, int type, int protocol);

/* Extend operations */
int port_isDataArrived(SOCKET socketFd);
int port_waitConnectFinished(SOCKET socketFd, int sec);
int port_waitDataArrived(SOCKET socketFd, int sec);

#endif /* End of __STD_SOCKET_PORT_H__ */
