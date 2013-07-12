/*
 * =====================================================================================
 *
 *       Filename:  exnet.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2012年03月06日 23时08分55秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zilong.Zhu (), zilong.whu@gmail.com
 *        Company:  edu.whu
 *
 * =====================================================================================
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/time.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "log.h"
#include "error.h"
#include "utils.h"
#include "dlist.h"
#include "hash.h"
#include "timer.h"
#include "exnet.h"

#define MAGIC_CHECK_NUM_1 19880109u
#define MAGIC_CHECK_NUM_2 19850309u

#define IS_VALID_EPEX(p) ((MAGIC_CHECK_NUM_1 == (p)->_magic_checknum_1) && (MAGIC_CHECK_NUM_2 == (p)->_magic_checknum_2))

enum
{
    TIMER_TYPE_TASK = 1,
    TIMER_TYPE_SOCK,
};

enum
{
    SOCK_NONE = 0,
    SOCK_READABLE = 0x01,
    SOCK_WRITEABLE = 0x02,
    SOCK_CLOSED = 0x04,
    SOCK_DETACHED = 0x08,
    SOCK_ERROR = 0x10,
    SOCK_IDLE = 0x20,
};

struct _netstub;
struct _epex_in;

typedef struct _nettask
{
    struct _netstub *_stub;

    __dlist_t _list;                            /* list, read or write or done */

    struct timeval _req_time;
    struct timeval _done_time;
    int _timeout;
    struct __timer_unit _tm_unit;

    uint8_t _op_type;
    uint8_t _read_all;
    uint16_t _status;
    int _errno;

    void *_buffer;
    size_t _curpos;
    size_t _size;

    void *_user_ptr;
} nettask_t;

typedef struct _netstub
{
    struct _epex_in *_epex;

    int _timeout;
    struct __timer_unit _tm_unit;

    __dlist_t _list;                            /* list */
    __dlist_t _dlist;                           /* done list */

    __dlist_t _rlist;                           /* readable list */
    __dlist_t _wlist;                           /* writeable list */
    __dlist_t _elist;                           /* error list */

    int _sock_fd;                               /* socket */
    int8_t _is_listen;                          /* listen flag */
    int8_t _is_silence;                         /* silence flag, do not notify events */
    uint16_t _status;                           /* status */
    int _errno;                                 /* errno */

    void *_user_ptr;

    __dlist_t _rd_q;                            /* read request queue */
    __dlist_t _wr_q;                            /* write request queue */
    __dlist_t _done_q;                          /* done queue */
} netstub_t;

struct _epex_in
{
    uint32_t _magic_checknum_1;

    int _ep_fd;
    int _ep_num;
    int _evt_size;
    struct epoll_event *_events;

    mstimer_t *_timer;

    __ex_hash_t *_sock2stub;

    __dlist_t _stubs;                           /* associated sockets */
    __dlist_t _avail_stubs;                     /* list of stubs which have completed requests */

    __dlist_t _rlist;                           /* readable stub list */
    __dlist_t _wlist;                           /* writeable stub list */
    __dlist_t _elist;                           /* error stub list */

    mempool_t _stub_pool;
    mempool_t _task_pool;

    uint32_t _magic_checknum_2;
};

static const char *const g_op_names[] = 
{
    "*",
    "NOTIFY",
    "READ",
    "WRITE",
    "ACCEPT",
    "CONNECT",
};

static const char *const g_status_names[] = 
{
    "*",
    "DONE",
    "PART_DONE",
    "DETACHED",
    "TIMEOUT",
    "IDLE",
    "CLOSED",
    "ERROR",
};

#ifdef DEBUG_EPEX

#define RESULT_DUMP(p)    result_dump(p)
#define NETTASK_DUMP(p)    nettask_dump(p)
#define NETSTUB_DUMP(p)    netstub_dump(p)

#else

#define RESULT_DUMP(p)
#define NETTASK_DUMP(p)
#define NETSTUB_DUMP(p)

#endif

static void result_dump(netresult_t *result)
{
    NOTICE("begin dump result: @%p", result);
    NOTICE("   sock[%d]", result->_sock_fd);
    NOTICE("   op type[%s]", g_op_names[result->_op_type]);
    NOTICE("   status[%s]", g_status_names[result->_status]);
    NOTICE("   error[%s]", strerror_t(result->_errno));
    NOTICE("   buffer[%p]", result->_buffer);
    NOTICE("   curpos[%d]", (int)result->_curpos);
    NOTICE("   size[%d]", (int)result->_size);
    NOTICE("   user ptr[%p]", result->_user_ptr);
    NOTICE("   user ptr2[%p]", result->_user_ptr2);
    NOTICE("end dump result: @%p", result);
}

static void nettask_dump(nettask_t *task)
{
    int num;

    NOTICE("begin dump task: @%p", task);
    DLIST_COUNT(&task->_list, num);
    NOTICE("   stub[%p]", task->_stub);
    NOTICE("   list num[%d]", num);
    NOTICE("   request time[%ld:%d]", (long)task->_req_time.tv_sec, (int)task->_req_time.tv_usec);
    NOTICE("   done time[%ld:%d]", (long)task->_done_time.tv_sec, (int)task->_done_time.tv_usec);
    NOTICE("   timeout[%d ms]", task->_timeout);
    NOTICE("   op type[%s]", g_op_names[task->_op_type]);
    NOTICE("   read all[%s]", task->_read_all ? "true" : "false");
    NOTICE("   status[%s]", g_status_names[task->_status]);
    NOTICE("   error[%s]", strerror_t(task->_errno));
    NOTICE("   buffer[%p]", task->_buffer);
    NOTICE("   curpos[%d]", (int)task->_curpos);
    NOTICE("   size[%d]", (int)task->_size);
    NOTICE("   user ptr[%p]", task->_user_ptr);
    NOTICE("end dump task: @%p", task);
}

static void netstub_dump(netstub_t *stub)
{
    int num;

    NOTICE("begin dump stub: @%p", stub);
    NOTICE("   epex[%p]", stub->_epex);
    NOTICE("   sock[%d]", stub->_sock_fd);
    NOTICE("   type[%s]", stub->_is_listen ? "listen" : "normal");
    NOTICE("   silence[%s]", stub->_is_silence ? "yes" : "no");
    NOTICE("   status[%hu]", stub->_status);
    NOTICE("   error[%s]", strerror_t(stub->_errno));
    NOTICE("   timeout[%d ms]", stub->_timeout);
    NOTICE("   user ptr[%p]", stub->_user_ptr);
    DLIST_COUNT(&stub->_list, num);
    NOTICE("   list num[%d]", num);
    DLIST_COUNT(&stub->_dlist, num);
    NOTICE("   done list num[%d]", num);
    DLIST_COUNT(&stub->_rlist, num);
    NOTICE("   rlist num[%d]", num);
    DLIST_COUNT(&stub->_wlist, num);
    NOTICE("   wlist num[%d]", num);
    DLIST_COUNT(&stub->_elist, num);
    NOTICE("   elist num[%d]", num);
    DLIST_COUNT(&stub->_rd_q, num);
    NOTICE("   read queue num[%d]", num);
    DLIST_COUNT(&stub->_wr_q, num);
    NOTICE("   write queue num[%d]", num);
    DLIST_COUNT(&stub->_done_q, num);
    NOTICE("   done queue num[%d]", num);
    NOTICE("end dump stub: @%p", stub);
}

#define MAKE_STUB_AVAIL(sock_stub) do {                                   \
    if ( DLIST_EMPTY(&(sock_stub)->_dlist) )                              \
    {                                                                     \
        DLIST_INSERT_B(&(sock_stub)->_dlist,                              \
                &(sock_stub)->_epex->_avail_stubs);                       \
    }                                                                     \
} while (0)

#define ACTIVE_STUB(sock_stub) do {                                       \
    if ( (sock_stub)->_timeout > 0 )                                      \
    {                                                                     \
        timer_del((sock_stub)->_epex->_timer,                             \
                &(sock_stub)->_tm_unit);                                  \
        timer_add2((sock_stub)->_epex->_timer,                            \
                &(sock_stub)->_tm_unit, (sock_stub)->_timeout);           \
    }                                                                     \
} while(0)

#define DO_WITH_ERROR(ep_hdl, sock_stub, err) do {                        \
    DEBUG("begin: do with error.");                                       \
    NETSTUB_DUMP(sock_stub);                                              \
    netstub_clean(ep_hdl, sock_stub);                                     \
    (sock_stub)->_status = SOCK_ERROR;                                    \
    (sock_stub)->_errno = (err);                                          \
    DLIST_INSERT_B(&(sock_stub)->_elist, &(ep_hdl)->_elist);              \
    NETSTUB_DUMP(sock_stub);                                              \
    DEBUG("end: do with error.");                                         \
} while(0)

#define DO_WITH_TASK_DONE(ep_hdl, sock_stub, task_ptr, status) do {       \
    DEBUG("begin: do with task done.");                                   \
    NETSTUB_DUMP(sock_stub);                                              \
    NETTASK_DUMP(task_ptr);                                               \
    if ( (task_ptr)->_timeout > 0 )                                       \
    {                                                                     \
        timer_del((ep_hdl)->_timer, &(task_ptr)->_tm_unit);               \
    }                                                                     \
    DLIST_REMOVE(&(task_ptr)->_list);                                     \
    DLIST_INSERT_B(&(task_ptr)->_list, &(sock_stub)->_done_q);            \
    MAKE_STUB_AVAIL(sock_stub);                                           \
    gettimeofday(&(task_ptr)->_done_time, NULL);                          \
    (task_ptr)->_status = (status);                                       \
    (task_ptr)->_errno = (sock_stub)->_errno;                             \
    NETTASK_DUMP(task_ptr);                                               \
    NETSTUB_DUMP(sock_stub);                                              \
    DEBUG("end: do with task done.");                                     \
} while(0)

#define DO_WITH_TASK_TIMEOUT(ep_hdl, sock_stub, task_ptr) do {            \
    DEBUG("begin: do with task timeout.");                                \
    NETSTUB_DUMP(sock_stub);                                              \
    NETTASK_DUMP(task_ptr);                                               \
    DLIST_REMOVE(&(task_ptr)->_list);                                     \
    DLIST_INSERT_B(&(task_ptr)->_list, &(sock_stub)->_done_q);            \
    MAKE_STUB_AVAIL(sock_stub);                                           \
    gettimeofday(&(task_ptr)->_done_time, NULL);                          \
    (task_ptr)->_status = NET_ETIMEOUT;                                   \
    (task_ptr)->_errno = ETIMEDOUT;                                       \
    NETTASK_DUMP(task_ptr);                                               \
    NETSTUB_DUMP(sock_stub);                                              \
    DEBUG("end: do with task timeout.");                                  \
} while(0)

#define TASK2RESULT(task, result) do {                                    \
    (result)->_sock_fd = (task)->_stub->_sock_fd;                         \
    (result)->_op_type = (task)->_op_type;                                \
    (result)->_status = (task)->_status;                                  \
    (result)->_errno = (task)->_errno;                                    \
    (result)->_buffer = (task)->_buffer;                                  \
    (result)->_curpos = (task)->_curpos;                                  \
    (result)->_size = (task)->_size;                                      \
    (result)->_user_ptr = (task)->_user_ptr;                              \
    (result)->_user_ptr2 = (task)->_stub->_user_ptr;                      \
    RESULT_DUMP(result);                                                  \
} while(0)

static uint32_t epex_hash(const void *key)
{
    return *(int *)key;
}

static int epex_cmp(const void *key1, const void *key2)
{
    return *(int *)key1 - *(int *)key2;
}

static nettask_t *nettask_alloc(struct _epex_in *h)
{
    nettask_t *task = (nettask_t *)mp_alloc(h->_task_pool);
    if ( NULL == task )
    {
        WARNING("failed to alloc nettask_t.");
        return NULL;
    }
    bzero(task, sizeof(*task));
    DLIST_INIT(&task->_list);
    DLIST_INIT(&task->_tm_unit._list);
    return task;
}

static void nettask_free(struct _epex_in *h, nettask_t *task)
{
    mp_free(h->_task_pool, task);
}

static netstub_t *netstub_alloc(struct _epex_in *h)
{
    netstub_t *res = (netstub_t *)mp_alloc(h->_stub_pool);
    if ( NULL == res )
    {
        WARNING("failed to alloc netstub_t.");
        return NULL;
    }
    res->_epex = h;
    res->_timeout = -1;
    bzero(&res->_tm_unit, sizeof res->_tm_unit);
    DLIST_INIT(&res->_tm_unit._list);
    DLIST_INIT(&res->_list);
    DLIST_INIT(&res->_dlist);
    DLIST_INIT(&res->_rlist);
    DLIST_INIT(&res->_wlist);
    DLIST_INIT(&res->_elist);
    res->_sock_fd = -1;
    res->_is_listen = 0;
    res->_is_silence = 0;
    res->_status = SOCK_NONE;
    res->_errno = 0;
    res->_user_ptr = NULL;
    DLIST_INIT(&res->_rd_q);
    DLIST_INIT(&res->_wr_q);
    DLIST_INIT(&res->_done_q);
    return res;
}

static void netstub_free(struct _epex_in *h, netstub_t *stub)
{
    int num;

    DLIST_REMOVE(&stub->_tm_unit._list);

    DLIST_REMOVE(&stub->_list);
    DLIST_REMOVE(&stub->_dlist);

    DLIST_REMOVE(&stub->_rlist);
    DLIST_REMOVE(&stub->_wlist);
    DLIST_REMOVE(&stub->_elist);

    DLIST_COUNT(&stub->_rd_q, num);
    if ( num > 0 )
    {
        WARNING("stub[%p] is going to be freed, but read queue has [%d] elems.", stub, num);
    }
    DLIST_COUNT(&stub->_wr_q, num);
    if ( num > 0 )
    {
        WARNING("stub[%p] is going to be freed, but write queue has [%d] elems.", stub, num);
    }
    DLIST_COUNT(&stub->_done_q, num);
    if ( num > 0 )
    {
        WARNING("stub[%p] is going to be freed, but done queue has [%d] elems.", stub, num);
    }
    DLIST_REMOVE(&stub->_rd_q);
    DLIST_REMOVE(&stub->_wr_q);
    DLIST_REMOVE(&stub->_done_q);

    mp_free(h->_stub_pool, stub);
}

static void netstub_clean(struct _epex_in *h, netstub_t *stub)
{
    if ( stub->_timeout > 0 )
    {
        timer_del(h->_timer, &stub->_tm_unit);
    }
    if ( epoll_ctl(h->_ep_fd, EPOLL_CTL_DEL, stub->_sock_fd, NULL) < 0 )
    {
        WARNING("failed to remove stub from epoll[%d], sock[%d], error[%s].", h->_ep_fd, stub->_sock_fd, strerror_t(errno));
    }
    DLIST_REMOVE(&stub->_rlist);
    DLIST_REMOVE(&stub->_wlist);
    DLIST_REMOVE(&stub->_elist);
}

static netstub_t *get_netstub_by_fd(struct _epex_in *h, int sock, int ignore_err)
{
    int key = sock;
    netstub_t *stub = NULL;
    if ( EX_HASH_SEEK_OK != ex_hash_seek(h->_sock2stub, &key, &stub) )
    {
        DEBUG("cannot find sock[%d] from sock2stub hash table.", sock);
        return NULL;
    }
    if ( NULL == stub )
    {
        FATAL("hash table may be fucked.");
        return NULL;
    }
    if ( (SOCK_ERROR & stub->_status) || (SOCK_DETACHED & stub->_status) )
    {
        if ( !ignore_err )
        {
            return NULL;
        }
    }
    return stub;
}

epex_t epex_open(int size)
{
    struct _epex_in *res = NULL;
    __ex_hash_arg_t args;

    if ( size <= 0 )
    {
        WARNING("invalid arg: size[%d].", size);
        return NULL;
    }

    res = (struct _epex_in *)calloc(1, sizeof(struct _epex_in));
    if ( NULL == res )
    {
        WARNING("failed to alloc mem for epex.");
        return NULL;
    }
    res->_magic_checknum_1 = MAGIC_CHECK_NUM_1;
    res->_magic_checknum_2 = MAGIC_CHECK_NUM_2;

    res->_ep_fd = epoll_create(size);
    if ( res->_ep_fd < 0 )
    {
        WARNING("failed to create epoll, error[%s].", strerror_t(errno));
        goto ERR;
    }
    res->_ep_num = size;
    res->_evt_size = size;
    if ( res->_evt_size > 1024 )
    {
        res->_evt_size = 1024;
    }

    res->_events = (struct epoll_event *)malloc(sizeof(struct epoll_event) * res->_evt_size);
    if ( NULL == res->_events )
    {
        WARNING("failed to alloc mem for epoll_event[%d].", (int)res->_evt_size);
        goto ERR;
    }

    args._bucket_num = res->_ep_num;
    args._key_size = sizeof(int);
    args._value_size = sizeof(void *);
    args._hash_fun = epex_hash;
    args._hash_cmp = epex_cmp;

    res->_sock2stub = ex_hash_init(&args);
    if ( NULL == res->_sock2stub )
    {
        WARNING("failed to init hash table.");
        goto ERR;
    }
    res->_timer = timer_new();
    if ( NULL == res->_timer )
    {
        WARNING("failed to init timer.");
        goto ERR;
    }
    res->_stub_pool = mp_init(sizeof(netstub_t));
    if ( NULL == res->_stub_pool )
    {
        WARNING("failed to init netstub_t mempool.");
        goto ERR;
    }
    res->_task_pool = mp_init(sizeof(nettask_t));
    if ( NULL == res->_task_pool )
    {
        WARNING("failed to init nettask_t mempool.");
        goto ERR;
    }
    DLIST_INIT(&res->_rlist);
    DLIST_INIT(&res->_wlist);
    DLIST_INIT(&res->_elist);
    DLIST_INIT(&res->_stubs);
    DLIST_INIT(&res->_avail_stubs);

    DEBUG("init epex ok.");
    return res;
ERR:
    epex_close(res);
    return NULL;
}

void epex_close(epex_t ptr)
{
    if ( NULL == ptr )
    {
        return ;
    }
    struct _epex_in *h = (struct _epex_in *)ptr;
    if ( !IS_VALID_EPEX(h) )
    {
        WARNING("invalid epex handle[%p].", h);
        return ;
    }
    SAFE_CLOSE(h->_ep_fd);
    if ( NULL != h->_events )
    {
        free(h->_events);
        h->_events = NULL;
    }
    if ( NULL != h->_sock2stub )
    {
        ex_hash_free(h->_sock2stub);
        h->_sock2stub = NULL;
    }
    if ( NULL != h->_timer )
    {
        timer_destroy(h->_timer);
        h->_timer = NULL;
    }
    if ( NULL != h->_stub_pool )
    {
        mp_close(h->_stub_pool);
        h->_stub_pool = NULL;
    }
    if ( NULL != h->_task_pool )
    {
        mp_close(h->_task_pool);
        h->_task_pool = NULL;
    }
    free(h);
}

static bool epex_attach_impl(epex_t ptr, int sock_fd, void *user_arg, int ms, int is_listen)
{
    if ( NULL == ptr || sock_fd < 0 )
    {
        WARNING("invalid args: ptr[%p] sock fd[%d].", ptr, sock_fd);
        return false;
    }
    struct _epex_in *h = (struct _epex_in *)ptr;
    if ( !IS_VALID_EPEX(h) )
    {
        WARNING("invalid epex handle[%p].", h);
        return false;
    }

    int key = sock_fd;
    netstub_t *stub = get_netstub_by_fd(h, sock_fd, 1);
    if ( NULL != stub )
    {
        if ( (SOCK_ERROR & stub->_status) || (SOCK_DETACHED & stub->_status) )
        {
            WARNING("dying stub[%p] fd[%d] status[%d].", stub, stub->_sock_fd, (int)stub->_status);
            return false;
        }
        DEBUG("already exist, stub[%p] fd[%d] status[%d].", stub, stub->_sock_fd, (int)stub->_status);
        return true;
    }
    if ( set_nonblock(sock_fd) < 0 )
    {
        WARNING("failed to set sock[%d] to nonblock mode, error[%s].", sock_fd, strerror_t(errno));
        return false;
    }

    stub = netstub_alloc(h);
    if ( NULL == stub )
    {
        WARNING("failed to alloc stub, sock[%d].", sock_fd);
        return false;
    }
    if ( EX_HASH_ADD_OK != ex_hash_add(h->_sock2stub, &key, &stub, 0) )
    {
        netstub_free(h, stub);
        WARNING("failed to add stub to hash table, sock[%d].", sock_fd);
        return false;
    }

    struct epoll_event ev;
    bzero(&ev, sizeof ev);
    ev.data.ptr = stub;

    stub->_is_listen = is_listen;
    if ( stub->_is_listen )
    {
        ev.events = EPOLLIN;                    /* listen fd use level triggered */
    }
    else
    {
        ev.events = EPOLLIN | EPOLLOUT | EPOLLET; /* edge triggered, spirit */
    }
    if ( epoll_ctl(h->_ep_fd, EPOLL_CTL_ADD, sock_fd, &ev) < 0 )
    {
        ex_hash_del(h->_sock2stub, &key, NULL);
        netstub_free(h, stub);
        WARNING("failed to add stub to epoll[%d], sock[%d], error[%s].", h->_ep_fd, sock_fd, strerror_t(errno));
        return false;
    }

    if ( ms > 0 )
    {
        stub->_timeout = ms;
        stub->_tm_unit._user_value = TIMER_TYPE_SOCK;
        if ( timer_add2(h->_timer, &stub->_tm_unit, stub->_timeout) < 0 )
        {
            if ( epoll_ctl(h->_ep_fd, EPOLL_CTL_DEL, sock_fd, NULL) < 0 )
            {
                WARNING("failed to remove sock[%d] from epoll[%d], error[%s].", stub->_sock_fd, h->_ep_fd, strerror_t(errno));
            }
            ex_hash_del(h->_sock2stub, &key, NULL);
            netstub_free(h, stub);
            WARNING("failed to add stub to timer, sock[%d].", sock_fd);
            return false;
        }
    }

    stub->_sock_fd = sock_fd;
    stub->_user_ptr = user_arg;
    DLIST_INSERT_B(&stub->_list, &h->_stubs);

    NETSTUB_DUMP(stub);
    DEBUG("stub attach ok, stub[%p] fd[%d] status[%d].", stub, stub->_sock_fd, (int)stub->_status);
    return true;
}

bool epex_attach(epex_t ptr, int sock_fd, void *user_arg, int ms)
{
    return epex_attach_impl(ptr, sock_fd, user_arg, ms, 0);
}

bool epex_detach(epex_t ptr, int sock_fd, void **p_user_arg)
{
    if ( NULL == ptr || sock_fd < 0 )
    {
        WARNING("invalid args: ptr[%p] sock fd[%d].", ptr, sock_fd);
        return false;
    }
    struct _epex_in *h = (struct _epex_in *)ptr;
    if ( !IS_VALID_EPEX(h) )
    {
        WARNING("invalid epex handle[%p].", h);
        return false;
    }
    netstub_t *stub = get_netstub_by_fd(h, sock_fd, 0);
    if ( NULL == stub )
    {
        WARNING("cannot get stub from hash table, sock[%d].", sock_fd);
        return false;
    }
    DEBUG("begin: detach stub, stub[%p] fd[%d] status[%d].", stub, stub->_sock_fd, (int)stub->_status);
    NETSTUB_DUMP(stub);

    netstub_clean(h, stub);
    stub->_status = SOCK_DETACHED;

    nettask_t *task;
    __dlist_t *cc;
    __dlist_t *nn;
    for ( cc = DLIST_NEXT(&stub->_rd_q); cc != &stub->_rd_q; cc = nn )
    {
        nn = DLIST_NEXT(cc);
        task = GET_OWNER(cc, nettask_t, _list);
        DO_WITH_TASK_DONE(h, stub, task, NET_EDETACHED);
    }
    for ( cc = DLIST_NEXT(&stub->_wr_q); cc != &stub->_wr_q; cc = nn )
    {
        nn = DLIST_NEXT(cc);
        task = GET_OWNER(cc, nettask_t, _list);
        DO_WITH_TASK_DONE(h, stub, task, NET_EDETACHED);
    }
    MAKE_STUB_AVAIL(stub);

    NETSTUB_DUMP(stub);
    DEBUG("end: stub detach ok, stub[%p] fd[%d] status[%d].", stub, stub->_sock_fd, (int)stub->_status);
    if ( NULL != p_user_arg )
    {
        *p_user_arg = stub->_user_ptr;
    }
    return true;
}

bool epex_op(int op, epex_t ptr, int sock_fd, void *buf, size_t size, void *user_arg, int ms, int read_all)
{
    if ( NULL == ptr || sock_fd < 0 || (NULL == buf && size > 0) )
    {
        WARNING("invalid args: ptr[%p] sock fd[%d] buf[%p] size[%u].", ptr, sock_fd, buf, (uint32_t)size);
        return false;
    }
    struct _epex_in *h = (struct _epex_in *)ptr;
    if ( !IS_VALID_EPEX(h) )
    {
        WARNING("invalid epex handle[%p].", h);
        return false;
    }
    netstub_t *stub = get_netstub_by_fd(h, sock_fd, 0);
    if ( NULL == stub )
    {
        WARNING("cannot get stub from hash table, sock[%d].", sock_fd);
        return false;
    }
    nettask_t *task = nettask_alloc(h);
    if ( NULL == task )
    {
        WARNING("failed to alloc task, op[%d] sock[%d].", op, sock_fd);
        return false;
    }
    gettimeofday(&task->_req_time, NULL);       /* now */
    if ( ms > 0 )
    {
        task->_tm_unit._tv = task->_req_time;
        task->_tm_unit._tv.tv_sec += ms / 1000;
        task->_tm_unit._tv.tv_usec += ms % 1000 * 1000;
        if ( task->_tm_unit._tv.tv_usec >= 1000000 )
        {
            ++task->_tm_unit._tv.tv_sec;
            task->_tm_unit._tv.tv_usec -= 1000000;
        }
        task->_tm_unit._user_value = TIMER_TYPE_TASK;
        if ( timer_add(h->_timer, &task->_tm_unit) < 0 )
        {
            nettask_free(h, task);
            WARNING("failed to add to timer, op[%d] sock[%d].", op, sock_fd);
            return false;
        }
        task->_timeout = ms;
        DEBUG("add to timer ok, op[%d] sock[%d] ms[%d].", op, sock_fd, ms);
    }
    else
    {
        task->_timeout = -1;
        DEBUG("no timer, op[%d] sock[%d] ms[%d].", op, sock_fd, ms);
    }
    task->_stub = stub;

    if ( NET_OP_READ == op )
    {
        DLIST_INSERT_B(&task->_list, &stub->_rd_q);
        if ( read_all )
        {
            task->_read_all = 1;
        }
        else
        {
            task->_read_all = 0;
        }
    }
    else
    {
        DLIST_INSERT_B(&task->_list, &stub->_wr_q);
    }

    task->_op_type = op;
    task->_status = 0;
    task->_buffer = buf;
    task->_curpos = 0;
    task->_size = size;
    task->_user_ptr = user_arg;

    NETTASK_DUMP(task);
    NETSTUB_DUMP(stub);
    DEBUG("add task[%p] op[%d] sock[%d] to stub[%p] ok.", task, op, sock_fd, stub);
    return true;
}

bool epex_listen(epex_t ptr, int sock_fd, void *user_arg)
{
    if ( NULL == ptr || sock_fd < 0 )
    {
        WARNING("invalid args: ptr[%p] sock fd[%d].", ptr, sock_fd);
        return false;
    }
    struct _epex_in *h = (struct _epex_in *)ptr;
    if ( !IS_VALID_EPEX(h) )
    {
        WARNING("invalid epex handle[%p].", h);
        return false;
    }
    if ( !epex_attach_impl(ptr, sock_fd, user_arg, -1, 1) )
    {
        WARNING("failed to attach sock[%d].", sock_fd);
        return false;
    }
    DEBUG("listen on sock[%d] ok.", sock_fd);
    return true;
}

bool epex_listen2(epex_t ptr, int sock_fd, int backlog, void *user_arg)
{
    if ( NULL == ptr || sock_fd < 0 || backlog <= 0 )
    {
        WARNING("invalid args: ptr[%p] sock fd[%d] backlog[%d].", ptr, sock_fd, backlog);
        return false;
    }
    struct _epex_in *h = (struct _epex_in *)ptr;
    if ( !IS_VALID_EPEX(h) )
    {
        WARNING("invalid epex handle[%p].", h);
        return false;
    }
    if ( listen(sock_fd, backlog) < 0 )
    {
        WARNING("failed to listen sock[%d], error[%s].", sock_fd, strerror_t(errno));
        return false;
    }
    return epex_listen(h, sock_fd, user_arg);
}

bool epex_connect(epex_t ptr, int sock_fd, struct sockaddr_in *addr, void *user_arg, int ms)
{
    if ( NULL == ptr || NULL == addr || sock_fd < 0 )
    {
        WARNING("invalid args: ptr[%p] addr[%p] sock fd[%d].", ptr, addr, sock_fd);
        return false;
    }
    struct _epex_in *h = (struct _epex_in *)ptr;
    if ( !IS_VALID_EPEX(h) )
    {
        WARNING("invalid epex handle[%p].", h);
        return false;
    }
    if ( !epex_attach(ptr, sock_fd, user_arg, ms) )
    {
        WARNING("failed to attach sock[%d].", sock_fd);
        return false;
    }
    int ret;
    do
    {
        ret = connect(sock_fd, (struct sockaddr *)addr, sizeof(*addr));
    } while (ret < 0 && EINTR == errno);
    int is_ok = 0;
    if ( ret < 0 )
    {
        if ( EINPROGRESS == errno )
        {
            is_ok = 1;
            DEBUG("sock[%d] connecting, EINPROGRESS.", sock_fd);
        }
        else
        {
            WARNING("sock[%d] connect failed, error[%s].", sock_fd, strerror_t(errno));
        }
    }
    else if ( 0 == ret )
    {
        netstub_t *stub = get_netstub_by_fd(h, sock_fd, 0);
        if ( NULL != stub )
        {
            is_ok = 1;
            /* stub is writeable */
            stub->_status |= SOCK_WRITEABLE;
            DLIST_INSERT_B(&stub->_wlist, &h->_wlist);
            NETSTUB_DUMP(stub);
        }
        else
        {
            WARNING("cannot get stub from hash table, sock[%d].", sock_fd);
        }
    }
    if ( is_ok )
    {
        /* send a empty write request */
        if ( epex_op(NET_OP_CONNECT, ptr, sock_fd, NULL, 0, user_arg, ms, 1) )
        {
            DEBUG("send connect request ok, sock[%d].", sock_fd);
            return true;
        }
        else
        {
            WARNING("faled to send connect request, sock[%d].", sock_fd);
        }
    }
    epex_detach(ptr, sock_fd, NULL);
    return false;
}

bool epex_read_any(epex_t ptr, int sock_fd, void *buf, size_t size, void *user_arg, int ms)
{
    return epex_op(NET_OP_READ, ptr, sock_fd, buf, size, user_arg, ms, 0);
}

bool epex_read(epex_t ptr, int sock_fd, void *buf, size_t size, void *user_arg, int ms)
{
    return epex_op(NET_OP_READ, ptr, sock_fd, buf, size, user_arg, ms, 1);
}

bool epex_write(epex_t ptr, int sock_fd, const void *buf, size_t size, void *user_arg, int ms)
{
    return epex_op(NET_OP_WRITE, ptr, sock_fd, (void *)buf, size, user_arg, ms, 1);
}

static bool set_notify_status_for_netstub(epex_t ptr, int sock_fd, int silence)
{
    if ( NULL == ptr || sock_fd < 0 )
    {
        WARNING("invalid args: ptr[%p] sock fd[%d].", ptr, sock_fd);
        return false;
    }
    struct _epex_in *h = (struct _epex_in *)ptr;
    if ( !IS_VALID_EPEX(h) )
    {
        WARNING("invalid epex handle[%p].", h);
        return false;
    }
    netstub_t *stub = get_netstub_by_fd(h, sock_fd, 1);
    if ( NULL == stub )
    {
        WARNING("cannot get stub from hash table, sock[%d].", sock_fd);
        return false;
    }
    DEBUG("%s nofify for stub[%p] fd[%d].", silence ? "disable" : "enable", stub, stub->_sock_fd);
    stub->_is_silence = silence ? 1 : 0;
    NETSTUB_DUMP(stub);
    return true;
}

bool epex_enable_notify(epex_t ptr, int sock_fd)
{
    return set_notify_status_for_netstub(ptr, sock_fd, 0);
}

bool epex_disable_notify(epex_t ptr, int sock_fd)
{
    return set_notify_status_for_netstub(ptr, sock_fd, 1);
}

ssize_t epex_poll(epex_t ptr, netresult_t *results, size_t size)
{
    if ( NULL == ptr || NULL == results || 0 == size )
    {
        WARNING("invalid args: ptr[%p] results[%p] size[%u].", ptr, results, (uint32_t)size);
        return -1;
    }
    struct _epex_in *h = (struct _epex_in *)ptr;
    if ( !IS_VALID_EPEX(h) )
    {
        WARNING("invalid epex handle[%p].", h);
        return -1;
    }
    int ret;
    do
    {
        ret = epoll_wait(h->_ep_fd, h->_events, h->_evt_size, 0);
    } while (ret<0 && EINTR == errno);
    int i;
    int flag;
    int work_flag = 0;
    int err_num;
    socklen_t optlen;
    netstub_t *stub;
    /* tranform events */
    for ( i = 0; i < ret; ++i )
    {
        stub = (netstub_t *)h->_events[i].data.ptr;
        if ( (h->_events[i].events & EPOLLHUP) || (h->_events[i].events & EPOLLERR) )
        {
            WARNING("epoll[%d]: sock[%d] error[HUP|ERR].", h->_ep_fd, stub->_sock_fd);
            optlen = sizeof(err_num);
            if ( getsockopt(stub->_sock_fd, SOL_SOCKET, SO_ERROR, &err_num, &optlen) < 0 )
            {
                WARNING("getsockopt error, error[%s].", strerror_t(errno));
                err_num = EIO;
            }
            DO_WITH_ERROR(h, stub, err_num); /* error */
        }
        else
        {
            if ( h->_events[i].events & EPOLLIN )
            {
                /* check and insert to readable list */
                if ( !(stub->_status & SOCK_READABLE) )
                {
                    DEBUG("begin: set readable.");
                    NETSTUB_DUMP(stub);
                    stub->_status |= SOCK_READABLE;
                    DLIST_INSERT_B(&stub->_rlist, &h->_rlist);
                    NETSTUB_DUMP(stub);
                    DEBUG("end: set readable.");
                }
            }
            if ( h->_events[i].events & EPOLLOUT )
            {
                /* check and insert to writeable list */
                if ( !(stub->_status & SOCK_WRITEABLE) )
                {
                    DEBUG("begin: set writeable.");
                    NETSTUB_DUMP(stub);
                    stub->_status |= SOCK_WRITEABLE;
                    DLIST_INSERT_B(&stub->_wlist, &h->_wlist);
                    NETSTUB_DUMP(stub);
                    DEBUG("end: set writeable.");
                }
            }
        }
    }
    nettask_t *task;
    __dlist_t *cur;
    __dlist_t *next;
    __dlist_t *cc;
    __dlist_t *nn;
    /* do with readable sockets */
    for ( cur = DLIST_NEXT(&h->_rlist); cur != &h->_rlist; cur = next )
    {
        next = DLIST_NEXT(cur);
        stub = GET_OWNER(cur, netstub_t, _rlist);
        if ( stub->_is_listen )
        {
            task = nettask_alloc(h);
            if ( NULL == task )
            {
                WARNING("listen fd[%d] is readable, but failed to alloc task to notice.", stub->_sock_fd);
            }
            else
            {
                DEBUG("begin: do with listen.");
                NETSTUB_DUMP(stub);
                NETTASK_DUMP(task);
                task->_stub = stub;
                task->_timeout = -1;
                task->_op_type = NET_OP_ACCEPT;
                task->_status = NET_DONE;
                DLIST_INSERT_B(&task->_list, &stub->_done_q);
                MAKE_STUB_AVAIL(stub);
                NETTASK_DUMP(task);
                NETSTUB_DUMP(stub);
                DEBUG("end: do with listen.");
            }
            /* listen fd use level trigger mode */
            stub->_status &= ~SOCK_READABLE;
            DLIST_REMOVE(cur);
            continue;
        }
        flag = 0;
        /* go through read request queue */
        for ( cc = DLIST_NEXT(&stub->_rd_q); cc != &stub->_rd_q; cc = nn )
        {
            nn = DLIST_NEXT(cc);
            task = GET_OWNER(cc, nettask_t, _list);
            /* socket not closed */
            if ( 0 == (stub->_status & SOCK_CLOSED) ) 
            {
                while ( task->_curpos < task->_size )
                {
                    do
                    {
                        ret = read(stub->_sock_fd, (char *)task->_buffer + task->_curpos, task->_size - task->_curpos);
                        flag = 1;
                        work_flag = 1;
                    } while (ret < 0 && EINTR == errno);
                    if ( ret < 0 )
                    {
                        if ( EAGAIN != errno ) /* error */
                        {
                            err_num = errno;
                            WARNING("read sock[%d] error[%s].", stub->_sock_fd, strerror_t(err_num));
                            DO_WITH_ERROR(h, stub, err_num);
                        }
                        else
                        {
                            if ( !task->_read_all && task->_curpos > 0 ) /* partly done */
                            {
                                DO_WITH_TASK_DONE(h, stub, task, NET_PART_DONE);
                            }
                            /* temply not readable */
                            DEBUG("begin: set unreadable.");
                            NETSTUB_DUMP(stub);
                            stub->_status &= ~SOCK_READABLE;
                            DLIST_REMOVE(cur);
                            NETSTUB_DUMP(stub);
                            DEBUG("end: set unreadable.");
                        }
                        break;
                    }
                    else if ( 0 == ret ) /* end of file */
                    {
                        /* not readable any more */
                        DEBUG("begin: set sock closed.");
                        NETSTUB_DUMP(stub);
                        stub->_status &= ~SOCK_READABLE;
                        stub->_status |= SOCK_CLOSED;
                        DLIST_REMOVE(cur);
                        NETSTUB_DUMP(stub);
                        DEBUG("end: set sock closed.");
                        break;
                    }
                    else    /* read some data */
                    {
                        task->_curpos += ret;
                    }
                }
                if ( task->_curpos >= task->_size ) /* fully done */
                {
                    DO_WITH_TASK_DONE(h, stub, task, NET_DONE);
                }
                else if ( ret < 0 )
                {
                    break;  /* already processed */
                }
                /* ret == 0 need to go through */
            }
            /* socket closed by peer */
            if ( stub->_status & SOCK_CLOSED )
            {
                DO_WITH_TASK_DONE(h, stub, task, NET_ECLOSED);
            }
        }
        if ( flag )
        {
            ACTIVE_STUB(stub);
        }
    }
    /* do with writeable sockets */
    for ( cur = DLIST_NEXT(&h->_wlist); cur != &h->_wlist; cur = next )
    {
        next = DLIST_NEXT(cur);
        stub = GET_OWNER(cur, netstub_t, _wlist);
        /* go through write request queue */
        for ( cc = DLIST_NEXT(&stub->_wr_q); cc != &stub->_wr_q; cc = nn )
        {
            nn = DLIST_NEXT(cc);
            task = GET_OWNER(cc, nettask_t, _list);

            while ( task->_curpos < task->_size )
            {
                do
                {
                    ret = write(stub->_sock_fd, (char *)task->_buffer + task->_curpos, task->_size - task->_curpos);
                    work_flag = 1;
                } while (ret < 0 && EINTR == errno);
                if ( ret < 0 )
                {
                    if ( EAGAIN != errno ) /* error */
                    {
                        err_num = errno;
                        WARNING("write sock[%d] error[%s].", stub->_sock_fd, strerror_t(err_num));
                        DO_WITH_ERROR(h, stub, err_num);
                    }
                    else
                    {
                        /* temply not writeable */
                        DEBUG("begin: set unwriteable.");
                        NETSTUB_DUMP(stub);
                        stub->_status &= ~SOCK_WRITEABLE;
                        DLIST_REMOVE(cur);
                        NETSTUB_DUMP(stub);
                        DEBUG("end: set unwriteable.");
                    }
                    break;
                }
                else
                {
                    task->_curpos += ret;
                }
            }
            if ( task->_curpos >= task->_size )
            {
                DO_WITH_TASK_DONE(h, stub, task, NET_DONE);
            }
            else
            {
                break;          /* already processed */
            }
        }
    }
    /* do with error sockets */
    for ( cur = DLIST_NEXT(&h->_elist); cur != &h->_elist; cur = next )
    {
        next = DLIST_NEXT(cur);
        stub = GET_OWNER(cur, netstub_t, _elist);
        /* move all pending request to done list */
        __dlist_t *cc;
        __dlist_t *nn;
        for ( cc = DLIST_NEXT(&stub->_rd_q); cc != &stub->_rd_q; cc = nn )
        {
            nn = DLIST_NEXT(cc);
            task = GET_OWNER(cc, nettask_t, _list);
            DO_WITH_TASK_DONE(h, stub, task, NET_ERROR);
        }
        for ( cc = DLIST_NEXT(&stub->_wr_q); cc != &stub->_wr_q; cc = nn )
        {
            nn = DLIST_NEXT(cc);
            task = GET_OWNER(cc, nettask_t, _list);
            DO_WITH_TASK_DONE(h, stub, task, NET_ERROR);
        }
        /* make sure moved to done list */
        MAKE_STUB_AVAIL(stub);
    }
    /* do with delays */
    struct __timer_unit *unit;

    timer_timeout(h->_timer);
    while ( NULL != (unit = timer_next(h->_timer)) )
    {
        switch (unit->_user_value)
        {
            case TIMER_TYPE_TASK:
                task = GET_OWNER(unit, nettask_t, _tm_unit);
                stub = task->_stub;
                DO_WITH_TASK_TIMEOUT(h, stub, task);
                break;
            case TIMER_TYPE_SOCK:
                stub = GET_OWNER(unit, netstub_t, _tm_unit);
                DEBUG("begin: do with sock timeout.");
                NETSTUB_DUMP(stub);
                netstub_clean(h, stub);
                stub->_status = SOCK_IDLE;
                stub->_errno = ETIMEDOUT;
                /* move all pending request to done list */
                for ( cc = DLIST_NEXT(&stub->_rd_q); cc != &stub->_rd_q; cc = nn )
                {
                    nn = DLIST_NEXT(cc);
                    task = GET_OWNER(cc, nettask_t, _list);
                    DO_WITH_TASK_DONE(h, stub, task, NET_EIDLE);
                }
                for ( cc = DLIST_NEXT(&stub->_wr_q); cc != &stub->_wr_q; cc = nn )
                {
                    nn = DLIST_NEXT(cc);
                    task = GET_OWNER(cc, nettask_t, _list);
                    DO_WITH_TASK_DONE(h, stub, task, NET_EIDLE);
                }
                /* make sure moved to done list */
                MAKE_STUB_AVAIL(stub);
                NETSTUB_DUMP(stub);
                DEBUG("end: do with sock timeout.");
                break;
            default:
                break;
        }
    }
    /* fill results */
    int key;
    size_t cnt = 0;
    int can_gen_notify;
    /* do with available sockets */
    for ( cur = DLIST_NEXT(&h->_avail_stubs); cur != &h->_avail_stubs; cur = next )
    {
        next = DLIST_NEXT(cur);
        stub = GET_OWNER(cur, netstub_t, _dlist);
        if (stub->_is_silence)
        {
            DEBUG("sock[%d]'s stub is silence, do not notify caller", stub->_sock_fd);
            continue;
        }
        can_gen_notify = 1;
        /* go through done queue */
        for ( cc = DLIST_NEXT(&stub->_done_q); cc != &stub->_done_q; cc = nn )
        {
            nn = DLIST_NEXT(cc);
            task = GET_OWNER(cc, nettask_t, _list);

            if ( cnt >= size )
            {
                break;
            }
            TASK2RESULT(task, results + cnt);
            DLIST_REMOVE(cc);
            nettask_free(h, task);
            can_gen_notify = 0;
            ++cnt;
        }
        if ( DLIST_EMPTY(&stub->_done_q) )
        {
            flag = 0;
            if ( stub->_status & SOCK_DETACHED )
            {
                if (!can_gen_notify)
                {
                    DEBUG("cannot gen nofity, notify detached stub[%p] sock[%d] next time."
                            , stub, stub->_sock_fd);
                }
                else if ( cnt < size )
                {
                    flag = 1;
                    DEBUG("detached stub, need to notify.");
                    results[cnt]._sock_fd = stub->_sock_fd;
                    results[cnt]._op_type = NET_OP_NOTIFY;
                    results[cnt]._status = NET_EDETACHED;
                    results[cnt]._errno = 0;
                    results[cnt]._buffer = NULL;
                    results[cnt]._curpos = 0;
                    results[cnt]._size = 0;
                    results[cnt]._user_ptr = NULL;
                    results[cnt]._user_ptr2 = stub->_user_ptr;
                    RESULT_DUMP(results + cnt);
                    ++cnt;
                }
                else
                {
                    DEBUG("results[%u] is full, notify detached stub[%p] sock[%d] next time."
                            , (uint32_t)size, stub, stub->_sock_fd);
                }
            }
            else if ( (stub->_status & SOCK_ERROR) /* error */
                    || (stub->_status & SOCK_IDLE) /* idle */
                    )
            {
                if (!can_gen_notify)
                {
                    DEBUG("cannot gen nofity, notify stub[%p] sock[%d] error next time."
                            , stub, stub->_sock_fd);
                }
                else if ( cnt < size )
                {
                    flag = 1;
                    DEBUG("notify upper caller now.");
                    results[cnt]._sock_fd = stub->_sock_fd;
                    results[cnt]._op_type = NET_OP_NOTIFY;
                    results[cnt]._status = (stub->_status & SOCK_IDLE) ? NET_EIDLE : NET_ERROR;
                    results[cnt]._errno = stub->_errno;
                    results[cnt]._buffer = NULL;
                    results[cnt]._curpos = 0;
                    results[cnt]._size = 0;
                    results[cnt]._user_ptr = NULL;
                    results[cnt]._user_ptr2 = stub->_user_ptr;
                    RESULT_DUMP(results + cnt);
                    ++cnt;
                }
                else
                {
                    DEBUG("results[%u] is full, notify stub[%p] sock[%d] error next time."
                            , (uint32_t)size, stub, stub->_sock_fd);
                }
            }
            else
            {
                /* done queue become empty, socket not available any more */
                DEBUG("normally remove stub[%p] sock[%d] from avail list.", stub, stub->_sock_fd);
                DLIST_REMOVE(cur);
            }
            if ( flag )
            {
                DLIST_REMOVE(cur);

                key = stub->_sock_fd;
                ex_hash_del(h->_sock2stub, &key, NULL); /* remove from hash table */
                NETSTUB_DUMP(stub);
                DEBUG("make dying stub[%p] sock[%d] dead.", stub, stub->_sock_fd);
                netstub_free(h, stub); /* rd_q, wr_q and done_q must be empty here */
            }
        }
    }
    if ( 0 == work_flag && 0 == cnt )
    {
        /* this loop do nothing, not busy, sleep a while to keep cpu idle at a high level */
        usleep(10);
    }
    return cnt;
}
