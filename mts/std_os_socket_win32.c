// std_socket_port.h
// 2011.1.25    Initial version by ershu
// 2015.10.17   Immigrate by doing

#if 0 ////----

#include "std_port/std_port.h"
#include "std_port/std_port_cs.h"
#include "std_hash_table.h"
#include "std_os_socket.h"
#include "std_socket_bsd_errno.h"

#if USE_OS_SOCKET_WIN32

#include <Ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

typedef struct {
    WSAEVENT hEvent;
    Uint32   events;
} SocketState_T;

/* hashtable map [SOCKET:WSAEVENT] */
static HashTable_T *_socketEventMap = NULL;

/* The mutex for hash table access */
#if 1
static Vm_Critical_Section_T _vm_csHash;
    #define INIT_CS()     _vm_initCriticalSection(&_vm_csHash);
    #define SHUT_CS()     _vm_destroyCriticalSection(&_vm_csHash);
    #define ENTER_CS()    _enterCS()
    #define LEAVE_CS()    _leaveCS()
#endif

/* Internal functions */
static IntR _calcHashCode(void *d);
static IntR _keyCompareFunc(void *a, void *b);
static SocketState_T* _createSocketState(WSAEVENT hEvent);
static void _destroySocketState(SocketState_T *sst);
static void _enterCS();
static void _leaveCS();

/*************************************************************************
 * Public interface
 *************************************************************************/

/* init */
int os_socket_init()
{
    WSADATA WSAData;
    VM_VERIFY(WSAStartup(MAKEWORD(2, 2), &WSAData) == 0);

    _socketEventMap = vm_ht_createHashTable(_calcHashCode, _keyCompareFunc, 65535);

    INIT_CS();

    if (! os_epoll_init())
    {
        STD_FATAL("Failed to initialize os_epoll.\n");
        return 0;
    }

    return 1;
}

/* shutdown */
void os_socket_shutdown()
{
    HashTableNode_T *p;
    SocketState_T *state;
    IntR i;

    os_epoll_shutdown();

    SHUT_CS();

    /* Delete all values, upper module forget close socket */
    for (i = 0; i < _socketEventMap->hashTableSize; i++)
    {
        p = _socketEventMap->table[i];
        while (p)
        {
            state = (SocketState_T*) p->value;
            WSACloseEvent(state->hEvent);
            _destroySocketState(state);

            p = p->next;
        }
    }

    /* Delete hash table self */
    vm_ht_destructHashTable(_socketEventMap);

    WSACleanup();
}

/* accept */
int os_accept(__in SOCKET s, __out struct sockaddr *addr, __in_out int *addrlen)
{
    int newSocket;
    WSAEVENT hEvent;
    SocketState_T *state;

    /* Clear can read state flag */
    ENTER_CS();
    VM_ASSERT(vm_ht_isKeyContained(_socketEventMap, (void*) s));
    state = (SocketState_T*) vm_ht_getValueByKey(_socketEventMap, (void*) s);
    state->events &= ~VM_SOCKET_CAN_READ;
    LEAVE_CS();
    
    newSocket = (int) accept(s, addr, addrlen);
    if (newSocket == INVALID_SOCKET)
        return newSocket;

    /* Success, bind a event */
    ENTER_CS();
    VM_ASSERT(! vm_ht_isKeyContained(_socketEventMap, (void*)newSocket));
    hEvent = WSACreateEvent();
    vm_ht_addPair(_socketEventMap, (void*)newSocket, (void*)_createSocketState(hEvent));
    LEAVE_CS();

    return newSocket;
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
    return getpeername(s, name, namelen);
}

/* getsockname */
int os_getsockname(__in SOCKET s, __out struct sockaddr *name, __in_out int *namelen)
{
    return getsockname(s, name, namelen);
}

/* getsockopt */
int os_getsockopt(__in SOCKET s, __in int level, __in int optname, __out void *optval, __in_out int *optlen)
{
    return getsockopt(s, level, optname, (char *) optval, optlen);
}

/* ioctlsocket */
int os_ioctlsocket(__in SOCKET s, __in long cmd, __in_out Uint32* argp)
{
    return ioctlsocket(s, cmd, argp);
}

/* listen */
int os_listen(__in SOCKET s, __in int backlog)
{
    return listen(s, backlog);
}

/* recv */
int os_recv(__in SOCKET s, __out char *buf, __in int len)
{
    int realRead;
    SocketState_T *state;

    realRead = recv(s, buf, len, 0);

    /* Error orcurr or not full filled */
    if (realRead < len)
    {
        ENTER_CS();
        /* Clear can read state flag */
        VM_ASSERT(vm_ht_isKeyContained(_socketEventMap, (void*) s));
        state = (SocketState_T*) vm_ht_getValueByKey(_socketEventMap, (void*) s);
        state->events &= ~VM_SOCKET_CAN_READ;
        LEAVE_CS();
    }

    return realRead;
}

/* recvfrom */
int os_recvfrom(__in SOCKET s, __out char *buf, __in int len, __in int flags, __out struct sockaddr *from, __in_out int *fromlen)
{
    int realRead;
    SocketState_T *state;

    realRead = recvfrom(s, buf, len, flags, from, fromlen);

    /* Error orcurr or not full filled */
    if (realRead < len)
    {
        ENTER_CS();
        /* Clear can read state flag */
        VM_ASSERT(vm_ht_isKeyContained(_socketEventMap, (void*) s));
        state = (SocketState_T*) vm_ht_getValueByKey(_socketEventMap, (void*) s);
        state->events &= ~VM_SOCKET_CAN_READ;
        LEAVE_CS();
    }

    return realRead;
}

/* select */
int os_select(__in int nfds, __in_out fd_set *readfds, __in_out fd_set *writefds, __in_out fd_set *exceptfds, __in const struct timeval* timeout)
{
    VM_VERIFY(0 && "Please do NOT use select modal!! Use os_poll instead !!");
    return 0;
}

/* poll */
int os_poll(__in_out struct pollfd fdarray[], __in int nfds, __in int timeout)
{
    IntR          i = 0, k = 0, idx = 0, count = 0;
    SocketState_T *state;
    Uint32        flag;
    Uint32        nIoRead;
    DWORD         n;
    WSANETWORKEVENTS events;
    WSAEVENT      _eventArray[WSA_MAXIMUM_WAIT_EVENTS];
    int           _gotArray[WSA_MAXIMUM_WAIT_EVENTS] = {0};
    HANDLE        hWakeup = NULL;

    /* Under windows page size is limited. */
    VM_VERIFY(nfds <= WSA_MAXIMUM_WAIT_EVENTS);

    ENTER_CS();

    if (nfds > 0 && fdarray[nfds - 1].events == _POLLWAKEUP)
    {
        /* The last one is a handle to notify wakeup */
        hWakeup = (HANDLE) fdarray[nfds - 1].fd;
        nfds--;
    }

    k = 0;
    for (i = 0; i < nfds; i++)
    {
        /* Get socket state */
        if (vm_ht_isKeyContained(_socketEventMap, (void *) fdarray[i].fd))
        {
            /* Do not save pointer! Save value copy, maybe deleted in another thread */
            state = (SocketState_T *) vm_ht_getValueByKey(_socketEventMap, (void *) fdarray[i].fd);
        }
        else
        {
            /* Was deleted in another thread, treat as error occur */
            _gotArray[i] = 1;
            fdarray[i].revents = POLLERR;
            continue;
        }

        /* Reset event */
        WSAResetEvent(state->hEvent);

        /* Mark flag */
        flag = FD_CLOSE;

        if (fdarray[i].events & POLLIN)
            flag |= FD_ACCEPT | FD_READ | FD_CONNECT;

        if (fdarray[i].events & POLLOUT)
            flag |= FD_ACCEPT | FD_WRITE | FD_CONNECT;

        /* Bind socket and event object */
        WSAEventSelect(fdarray[i].fd, state->hEvent, flag);

        /* Clean revents field */
        fdarray[i].revents = 0;

        /* Put into array */
        _eventArray[k] = state->hEvent;

        /* Increase counter */
        k++;

        /* Process next */
    }
    LEAVE_CS();

    /* No event need to handle */
    if (k <= 0)
        return 0;

    if (hWakeup)
    {
        /* Add handle for wakeup */
        VM_ASSERT(k < WSA_MAXIMUM_WAIT_EVENTS);
        _eventArray[k++] = hWakeup;
        ResetEvent(hWakeup);
    }

    /* Wait now */
    n = WSAWaitForMultipleEvents((DWORD) k, _eventArray, FALSE, (DWORD) timeout, TRUE);
    if (n == WSA_WAIT_TIMEOUT)
        WSASetLastError(ETIMEDOUT);

    /* Handle event now */
    ENTER_CS();
    for (i = 0; i < nfds; i++)
    {
        /* Get socket state */
        if (vm_ht_isKeyContained(_socketEventMap, (void *) fdarray[i].fd))
        {
            /* Donot save pointer! Save value copy, maybe deleted in another thread */
            state = (SocketState_T *) vm_ht_getValueByKey(_socketEventMap, (void *) fdarray[i].fd);
        }
        else
        {
            /* Was deleted in another thread, treat as error occur */
            _gotArray[i] = 1;
            fdarray[i].revents = POLLERR;
            continue;
        }

        /* If already has data in socket buffer, mark can read! */
        if ((fdarray[i].events & POLLIN) &&
            os_ioctlsocket((int) fdarray[i].fd, FIONREAD, &nIoRead) == 0 && 
            nIoRead > 0)
        {
            fdarray[i].revents |= POLLIN;
            state->events |= VM_SOCKET_CAN_READ;
        }

        if (state->events)
        {
            /* Already got events! */
            _gotArray[i] = 1;

            /* Feed back events */
            if (state->events & VM_SOCKET_CAN_READ)
                fdarray[i].revents |= POLLIN;
            if (state->events & VM_SOCKET_CAN_WRITE)
                fdarray[i].revents |= POLLOUT;
            if (state->events & VM_SOCKET_ERROR)
                fdarray[i].revents |= POLLERR;
        }

        /* Process next */
    }
    LEAVE_CS();

    /* Count events captured by WSAWaitForMultipleEvents */
    if (n >= WSA_WAIT_EVENT_0 && n < (WSA_WAIT_EVENT_0 + nfds))
    {
        /* Get the smallest start index */
        n -= WSA_WAIT_EVENT_0;

        ENTER_CS();
        for (i = n; i < k; i++)
        {
            idx = i;

            /* Get socket state */
            if (vm_ht_isKeyContained(_socketEventMap, (void*) fdarray[idx].fd))
            {
                state = (SocketState_T*) vm_ht_getValueByKey(_socketEventMap, (void*) fdarray[idx].fd);
            }
            else
            {
                /* Was deleted in another thread, treat as error */
                _gotArray[i] = 1;
                fdarray[i].revents = POLLERR;
                continue;
            }

            if (WSAEnumNetworkEvents(fdarray[idx].fd, _eventArray[idx], &events) == 0 && 
                events.lNetworkEvents)
            {
                /* Got some events! */
                _gotArray[i] = 1;

                if (events.lNetworkEvents & FD_READ)
                {
                    fdarray[i].revents |= POLLIN;
                    state->events |= VM_SOCKET_CAN_READ;
                }
                if (events.lNetworkEvents & FD_ACCEPT)
                {
                    fdarray[i].revents |= POLLIN | POLLOUT;
                    state->events |= VM_SOCKET_CAN_READ | VM_SOCKET_CAN_WRITE;
                }
                if (events.lNetworkEvents & FD_WRITE)
                {
                    fdarray[i].revents |= POLLOUT;
                    state->events |= VM_SOCKET_CAN_WRITE;
                }
                if (events.lNetworkEvents & FD_CLOSE)
                {
                    fdarray[i].revents |= POLLIN;
                    state->events |= VM_SOCKET_CAN_READ;
                }
                if (events.lNetworkEvents & FD_CONNECT)
                {
                    fdarray[i].revents |= POLLOUT;
                    state->events |= VM_SOCKET_CAN_WRITE;
                }
            }

            /* Process next */
        }
        LEAVE_CS();
    }

    /* Get count */
    count = 0;
    for (i = 0; i < nfds; i++)
        if (_gotArray[i])
            count++;

    /* Return count */
    return (int) count;
}

/* send */
int os_send(__in SOCKET s, __in const char *buf, __in int len)
{
    SocketState_T *state;
    int realSend;

    realSend = send(s, buf, len, 0);

    /* Error occur or not full sent */
    if (realSend < len)
    {
        ENTER_CS();
        /* Clear can read state flag */
        VM_ASSERT(vm_ht_isKeyContained(_socketEventMap, (void*) s));
        state = (SocketState_T*) vm_ht_getValueByKey(_socketEventMap, (void*) s);
        state->events &= ~VM_SOCKET_CAN_WRITE;
        LEAVE_CS();
    }

    return realSend;
}

/* sendto */
int os_sendto(__in SOCKET s, __in const char *buf, __in int len, __in int flags, __in const struct sockaddr *to, __in int tolen)
{
    SocketState_T *state;
    int realSend;

    realSend = sendto(s, buf, len, flags, to, tolen);

    /* Error occur or not full sent */
    if (realSend < len)
    {
        ENTER_CS();
        /* Clear can read state flag */
        VM_ASSERT(vm_ht_isKeyContained(_socketEventMap, (void*) s));
        state = (SocketState_T*) vm_ht_getValueByKey(_socketEventMap, (void*) s);
        state->events &= ~VM_SOCKET_CAN_WRITE;
        LEAVE_CS();
    }

    return realSend;
}

/* setsockopt */
int os_setsockopt(__in SOCKET s, __in int level, __in int optname, __in const char *optval, __in int optlen)
{
    return setsockopt(s, level, optname, (const char *) optval, optlen);
}

/* socket_destroy */
int os_socket_destroy(__in SOCKET s)
{
    SocketState_T *state;

    /* Remove from event map, caller may free some freed socket multi-times */
    ENTER_CS();
    if (vm_ht_isKeyContained(_socketEventMap, (void*)s))
    {
        state = (SocketState_T*) vm_ht_removeByKey(_socketEventMap, (void*)s);
        if (state)
        {
            WSACloseEvent(state->hEvent);
            _destroySocketState(state);
        }
    }
    LEAVE_CS();

    /* Clean up socket */
    shutdown(s, SD_SEND); 
    return closesocket(s);
}

/* socket */
SOCKET os_socket(__in int af, __in int type, __in int protocol)
{
    SOCKET s;
    WSAEVENT hEvent;

    /* Create socket */
    s = (int) socket(af, type, protocol);
    if (s == INVALID_SOCKET)
        return s;

    ENTER_CS();
    /* Success, bind a event */
    VM_ASSERT(! vm_ht_isKeyContained(_socketEventMap, (void*)s));
    hEvent = WSACreateEvent();
    vm_ht_addPair(_socketEventMap, (void*)s, (void*)_createSocketState(hEvent));
    LEAVE_CS();

    return s;
}

/*************************************************************************
 * Internal functions
 *************************************************************************/

/* hash code calculator */
static IntR _calcHashCode(void *d)
{
    return (IntR) d;
}

/* hash key compare */
static IntR _keyCompareFunc(void *a, void *b)
{
    return (IntR)a == (IntR)b;
}

/* create value */
static SocketState_T* _createSocketState(WSAEVENT hEvent)
{
    SocketState_T *ret = (SocketState_T*) VM_MEM_ALLOC(sizeof(SocketState_T));
    ret->hEvent = hEvent;
    ret->events = 0;
    return ret;
}

/* destory value */
static void _destroySocketState(SocketState_T *sst)
{
    VM_MEM_FREE(sst);
}

/* safe lock */
static void _enterCS()
{
    IntR errorNo;

    errorNo = SOCKET_ERRNO();
    _vm_enterCriticalSection(&_vm_csHash);
    SOCKET_SET_ERRNO((int) errorNo);
}

/* safe unlock */
static void _leaveCS()
{
    IntR errorNo;

    errorNo = SOCKET_ERRNO();
    _vm_leaveCriticalSection(&_vm_csHash);
    SOCKET_SET_ERRNO((int) errorNo);
}

#endif /* End of USE_OS_SOCKET_WIN32 */

#endif ////----
