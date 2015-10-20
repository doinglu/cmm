// std_socket_port.h
// 2013.10.2    Initial version by ershu
// 2015.10.17   Immigrate by doing

#if 0 ////----

#include "std_port/std_port.h"
#include "std_port/std_port_cs.h"
#include "std_memmgr/std_memmgr.h"
#include "std_os_socket.h"
#include "std_socket_bsd_errno.h"

#ifdef USE_OS_EPOLL_SIM

#define BASIC_EPOLL_INDEX   10000

typedef struct epoll_cookie
{
    epoll_data_t data;
} epoll_cookie_t;

typedef struct simEpoll
{
    int             epfd;
    int             maxSize;
    int             size;
    struct pollfd  *pollArr;
    epoll_cookie_t *epollCookie;
} sim_epoll_t;

static sim_epoll_t **_allEpoll = NULL;
static int           _epollCount = 0;
static int           _epollMaxCount = 0;

static std_critical_section_t os_epoll_cs;
#define ENTER_CS()    os_epoll_enterCS()
#define LEAVE_CS()    os_epoll_leaveCS()

/* Internal routines */
static void os_epoll_enterCS();
static void os_epoll_leaveCS();
static int  os_epoll_ctlAdd(sim_epoll_t *simEpoll, int fd, struct epoll_event *event);
static int  os_epoll_ctlDel(sim_epoll_t *simEpoll, int fd);
static int  os_epoll_ctlMod(sim_epoll_t *simEpoll, int fd, struct epoll_event *event);
static int  os_epoll_extendPoll(sim_epoll_t *simEpoll);
static int  os_epoll_findFd(sim_epoll_t *simEpoll, int fd);
static sim_epoll_t *os_epoll_getEpoll(int epfd);
static int  os_epoll_mapEventsToPoll(int epollEvents);
static int  os_epoll_mapEventsToEPoll(int epollEvents);

/* Initialize the module */
int os_epoll_init()
{
    STD_ASSERT(_allEpoll == NULL);
    STD_ASSERT(_epollCount == 0);
    STD_ASSERT(_epollMaxCount == 0);

    std_init_critical_section(&os_epoll_cs);

    return 1;
}

/* Shutdown the module */
void os_epoll_shutdown()
{
    int i;

    ENTER_CS();

    /* Close all existed epoll*/
    if (_allEpoll != NULL)
    {
        for (i = 0; i < _epollMaxCount; i++)
            if (_allEpoll[i] != NULL)
                os_epoll_close(i);

        STD_MEM_FREE(_allEpoll);
        _allEpoll = NULL;
        _epollCount = 0;
        _epollMaxCount= 0;
    }

    LEAVE_CS();

    std_destroy_critical_section(&os_epoll_cs);
}

/* Create a fd for epoll operation */
int os_epoll_create(__in int size)
{
    int ret = 0;
    int i;
    sim_epoll_t *simEpoll = 0;

    ENTER_CS();
    do
    {
        if (_epollCount >= _epollMaxCount)
        {
            sim_epoll_t **newEpoll;
            int newEpollMaxCount = _epollMaxCount * 2;
            if (newEpollMaxCount < 16)
                newEpollMaxCount = 16;
            newEpoll = (sim_epoll_t **) STD_MEM_ALLOC(sizeof(sim_epoll_t *) * newEpollMaxCount);
            if (newEpoll == NULL)
            {
                /* Memory is not enough */
                ret = ENOMEM;
                break;
            }
            memset(newEpoll, 0, sizeof(sim_epoll_t *) * newEpollMaxCount);
            if (_allEpoll != NULL)
            {
                /* Copy from old */
                memcpy(newEpoll, _allEpoll, sizeof(sim_epoll_t *) * _epollMaxCount);
                STD_MEM_FREE(_allEpoll);
            }
            _allEpoll = newEpoll;
            _epollMaxCount = newEpollMaxCount;
        }
        STD_ASSERT(_allEpoll != NULL);

        /* Search a free one */
        for (i = 0; i < _epollMaxCount; i++)
            if (_allEpoll[i] == NULL)
                break;
        if (i >= _epollMaxCount)
        {
            /* Impossible here */
            STD_ASSERT(0);
            ret = ENOMEM;
            break;
        }

        /* Found entry, create it */
        simEpoll = (sim_epoll_t *) STD_MEM_ALLOC(sizeof(sim_epoll_t));
        if (simEpoll == NULL)
        {
            /* Failed to allocate node */
            ret = ENOMEM;
            break;
        }
        memset(simEpoll, 0, sizeof(sim_epoll_t));

        /* Initialize the structure */
        simEpoll->epfd = i + BASIC_EPOLL_INDEX;

        /* Put into list */
        _allEpoll[i] = simEpoll;
    } while (0);
    LEAVE_CS();

    if (ret == 0)
        return simEpoll->epfd;

    /* Failed */
    SOCKET_SET_ERRNO(ret);
    return -1;
}

int os_epoll_ctl(__in int epfd, __in int op, __in int fd, __in struct epoll_event *event)
{
    int ret = 0;
    sim_epoll_t *simEpoll;

    ENTER_CS();

    do
    {
        simEpoll = os_epoll_getEpoll(epfd);
        if (simEpoll == NULL)
        {
            ret = EBADF;
            break;
        }
        STD_ASSERT(simEpoll->epfd == epfd);
 
        switch (op)
        {
        case EPOLL_CTL_ADD:
            ret = os_epoll_ctlAdd(simEpoll, fd, event);
            break;

        case EPOLL_CTL_DEL:
            ret = os_epoll_ctlDel(simEpoll, fd);
            break;

        case EPOLL_CTL_MOD:
            ret = os_epoll_ctlMod(simEpoll, fd, event);
            break;
        }
    } while (0);

    LEAVE_CS();

    if (ret == 0)
        return 0;

    /* Failed */
    SOCKET_SET_ERRNO(ret);
    return -1;
}

/* ATTENTION:
 * Since the operation ADD/DEL/MOD may be occurred during wait, so I can't
 * pend to wait signal too long */
int os_epoll_wait(__in int epfd, __out struct epoll_event *events, __in int maxevents, __in int timeout)
{
    std_tick_t lastTick = std_get_current_tick();
    sim_epoll_t *simEpoll;
    int ret = 0;
    struct pollfd *pollArr = NULL;
    int pollMaxCount, pollCount, n = 0;

    /* Max wait time limited to 100 for simulation */
    if (timeout > 100)
        timeout = 100;

    /* Split wait to 10ms */
    do
    {
        int elapsedTicks;
        int nowTick;
        int i;

        /* Poll */
        ENTER_CS();
        do
        {
            simEpoll = os_epoll_getEpoll(epfd);
            if (simEpoll == NULL)
            {
                ret = EBADF;
                break;
            }

            /* Copy the pollFd array into pollArr since it may be modified out of
             * critical section */

            /* Prepare/check buffer */
            if (pollArr == NULL)
            {
                pollArr = (struct pollfd *) STD_ALLOCA(sizeof(struct pollfd) * simEpoll->maxSize);
                pollMaxCount = simEpoll->maxSize;
            } else
            if (pollMaxCount < simEpoll->size)
                /* Allocated buffer is not enough, return failed */
                break;

            /* Copy array */
            pollCount = simEpoll->size;
            memcpy(pollArr, simEpoll->pollArr, sizeof(struct pollfd) * simEpoll->size);
        } while (0);
        LEAVE_CS();
        if (ret != 0)
            break;

        if (pollCount > STD_MAX_POLL_SOCKETS)
        {
            /* Poll sockets page by page */
            int polledResult, thisPollCount;
            n = 0;
            polledResult = 0;
            for (i = 0; i < pollCount; i += STD_MAX_POLL_SOCKETS)
            {
                /* Calculate count to poll */
                thisPollCount = pollCount - i;
                if (thisPollCount > STD_MAX_POLL_SOCKETS)
                    thisPollCount = STD_MAX_POLL_SOCKETS;

                /* Got the result */
                polledResult = os_poll(pollArr + i, thisPollCount, 0);
                if (polledResult > 0)
                    n += polledResult;
            }
            if (n == 0 && timeout > 0)
                /* Polled nothing, sleep a while (no more than 10ms) */
                std_sleep(timeout > 10 ? 10 : timeout);
        } else
        {
            /* Poll all sockets */
            n = os_poll(pollArr, pollCount, timeout > 10 ? 10 : timeout);
        }

        if (n > 0)
        {
            int eventCount = 0;
            int revents;

            /* Lookup events */
            ENTER_CS();
            if (pollCount > simEpoll->size)
                pollCount = simEpoll->size;
            for (i = 0; i < pollCount; i++)
            {
                if (! pollArr[i].revents)
                    continue;

                if (pollArr[i].fd != simEpoll->pollArr[i].fd)
                    /* The socket is modified, skip */
                    continue;

                /* Filter events with current of simEpoll */
                revents = pollArr[i].revents & simEpoll->pollArr[i].events;
                if (! revents)
                    continue;

                if (eventCount >= maxevents)
                    /* Result event full */
                    break;

                /* Got events */
                events[eventCount].data = simEpoll->epollCookie[i].data;
                events[eventCount].events = os_epoll_mapEventsToEPoll(revents);
                eventCount++;
            }
            LEAVE_CS();

            ret = eventCount;
            break;
        }


        /* Update timeout by elapsed ticks */
        nowTick = std_get_current_tick();
        elapsedTicks = nowTick - lastTick;
        lastTick = nowTick;
        if (elapsedTicks < 0)
            elapsedTicks = 0;
        if (timeout > elapsedTicks)
            timeout -= elapsedTicks;
        else
            timeout = 0;
    } while (timeout > 0);

    if (ret >= 0)
        return ret;

    /* Failed */
    SOCKET_SET_ERRNO(ret);
    return -1;
}

int os_epoll_close(__in int epfd)
{
    sim_epoll_t *simEpoll;
    int ret = 0;

    /* Poll */
    ENTER_CS();
    do
    {
        simEpoll = os_epoll_getEpoll(epfd);
        if (simEpoll == NULL)
        {
            ret = EBADF;
            break;
        }

        if (simEpoll->pollArr != NULL)
            STD_MEM_FREE(simEpoll->pollArr);

        if (simEpoll->epollCookie != NULL)
            STD_MEM_FREE(simEpoll->epollCookie);

        /* Erase entry */
        STD_ASSERT(epfd >= BASIC_EPOLL_INDEX && epfd < BASIC_EPOLL_INDEX + _epollMaxCount);
        STD_ASSERT(_allEpoll[epfd - BASIC_EPOLL_INDEX] == simEpoll);
        _allEpoll[epfd - BASIC_EPOLL_INDEX] = NULL;
        STD_MEM_FREE(simEpoll);
    } while (0);
    LEAVE_CS();

    if (ret == 0)
        return 0;

    /* Failed */
    SOCKET_SET_ERRNO(ret);
    return -1;
}

/* safe lock */
static void os_epoll_enterCS()
{
    IntR errorNo;

    errorNo = SOCKET_ERRNO();
    std_enter_critical_section(&os_epoll_cs);
    SOCKET_SET_ERRNO((int) errorNo);
}

/* safe unlock */
static void os_epoll_leaveCS()
{
    IntR errorNo;

    errorNo = SOCKET_ERRNO();
    std_leave_critical_section(&os_epoll_cs);
    SOCKET_SET_ERRNO((int) errorNo);
}

/* Add a socket to poll */
static int os_epoll_ctlAdd(sim_epoll_t *simEpoll, int fd, struct epoll_event *event)
{
    if (os_epoll_findFd(simEpoll, fd) != -1)
    {
        /* Already existed */
        STD_TRACE("Socket %d is already in epoll %d, failed to EPOLL_CTL_ADD.\n",
                  (int) fd, (int) simEpoll->epfd);
        return EEXIST;
    }

    /* Append fd */

    /* Extend when full */
    if (simEpoll->size >= simEpoll->maxSize)
    {
        if (! os_epoll_extendPoll(simEpoll))
        {
            /* Find to extend */
            STD_TRACE("Failed to extend epoll %d(size = %d), failed to EPOLL_CTL_ADD.\n",
                      (int) simEpoll->epfd, (int) simEpoll->maxSize);
            return ENOMEM;
        }
    }

    STD_ASSERT(simEpoll->size < simEpoll->maxSize);
    simEpoll->pollArr[simEpoll->size].fd = fd;
    simEpoll->pollArr[simEpoll->size].events = os_epoll_mapEventsToPoll(event->events) |
                                               (POLLERR | POLLHUP);
    simEpoll->epollCookie[simEpoll->size].data = event->data;
    simEpoll->size++;
    return 0;
}

/* Remove a socket from poll */
static int os_epoll_ctlDel(sim_epoll_t *simEpoll, int fd)
{
    int index;

    if ((index = os_epoll_findFd(simEpoll, fd)) == -1)
    {
        /* Not found */
        STD_TRACE("Socket %d is not in epoll %d, failed to EPOLL_CTL_DEL.\n",
                  (int) fd, (int) simEpoll->epfd);
        return ENOENT;
    }

    STD_ASSERT(simEpoll->size > 0);
    if (index != simEpoll->size - 1)
    {
        /* Replace current one with last one */
        simEpoll->pollArr[index] = simEpoll->pollArr[simEpoll->size - 1];
        simEpoll->epollCookie[index] = simEpoll->epollCookie[simEpoll->size - 1];
    }
    simEpoll->size--;
    return 0;
}

/* Modify socket event to poll */
static int os_epoll_ctlMod(sim_epoll_t *simEpoll, int fd, struct epoll_event *event)
{
    int index;

    if ((index = os_epoll_findFd(simEpoll, fd)) == -1)
    {
        /* Not found */
        STD_TRACE("Socket %d is not in epoll %d, failed to EPOLL_CTL_MOD.\n",
                  (int) fd, (int) simEpoll->epfd);
        return ENOENT;
    }

    /* Modify it */
    simEpoll->pollArr[index].events = os_epoll_mapEventsToPoll(event->events) |
                                      (POLLERR | POLLHUP);
    simEpoll->epollCookie[index].data = event->data;
    return 0;
}

/* Extend poll */
/* Return 0 means failed to extend */
static int os_epoll_extendPoll(sim_epoll_t *simEpoll)
{
    struct pollfd  *newPollArr;
    epoll_cookie_t *newEpollCookie;
    int             newMaxSize;

    newMaxSize = simEpoll->maxSize * 2;
    if (newMaxSize < 8)
        newMaxSize = 8;

    /* Allocate new memory */
    newPollArr = (struct pollfd *) STD_MEM_ALLOC(sizeof(struct pollfd) * newMaxSize);
    if (newPollArr == NULL)
        return 0;

    newEpollCookie = (epoll_cookie_t *) STD_MEM_ALLOC(sizeof(epoll_cookie_t) * newMaxSize);
    if (newEpollCookie == NULL)
    {
        STD_MEM_FREE(newPollArr);
        return 0;
    }

    memset(newPollArr, 0, sizeof(struct pollfd) * newMaxSize);
    memset(newEpollCookie, 0, sizeof(epoll_cookie_t) * newMaxSize);

    /* Copy from old */
    if (simEpoll->maxSize > 0)
    {
        STD_ASSERT(simEpoll->pollArr != NULL);
        STD_ASSERT(simEpoll->epollCookie != NULL);
        memcpy(newPollArr, simEpoll->pollArr, simEpoll->maxSize * sizeof(struct pollfd));
        memcpy(newEpollCookie, simEpoll->epollCookie, simEpoll->maxSize * sizeof(epoll_cookie_t));
        STD_MEM_FREE(simEpoll->pollArr);
        STD_MEM_FREE(simEpoll->epollCookie);
    }
    simEpoll->pollArr = newPollArr;
    simEpoll->epollCookie = newEpollCookie;
    simEpoll->maxSize = newMaxSize;

    return 1;
}

/* Find socket in poll */
/* Return -1 means not found */
static int os_epoll_findFd(sim_epoll_t *simEpoll, int fd)
{
    int i;

    if (simEpoll->size <= 0)
        /* The array is empty */
        return -1;

    for (i = 0; i < simEpoll->size; i++)
        if (simEpoll->pollArr[i].fd == fd)
            return i;

    return -1;
}

static sim_epoll_t *os_epoll_getEpoll(int epfd)
{
    if (epfd < BASIC_EPOLL_INDEX || epfd >= BASIC_EPOLL_INDEX + _epollMaxCount)
        return NULL;

    return _allEpoll[epfd - BASIC_EPOLL_INDEX];
}

/* epoll events -> poll events */
static int os_epoll_mapEventsToPoll(int epollEvents)
{
    int pollEvents = 0;

    if (epollEvents & EPOLLIN)  pollEvents |= POLLIN;
    if (epollEvents & EPOLLOUT) pollEvents |= POLLOUT;
    if (epollEvents & EPOLLERR) pollEvents |= POLLERR;
    if (epollEvents & EPOLLHUP) pollEvents |= POLLHUP;

    return pollEvents;
}

/* poll events -> epoll events */
static int os_epoll_mapEventsToEPoll(int pollEvents)
{
    int epollEvents = 0;

    if (pollEvents & POLLIN)  epollEvents |= EPOLLIN;
    if (pollEvents & POLLOUT) epollEvents |= EPOLLOUT;
    if (pollEvents & POLLERR) epollEvents |= EPOLLERR;
    if (pollEvents & POLLHUP) epollEvents |= EPOLLHUP;

    return epollEvents;
}

#endif /* End of #ifdef USE_OS_EPOLL_SIM */

#endif ////----
