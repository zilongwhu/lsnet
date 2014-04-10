/*
 * =====================================================================================
 *
 *       Filename:  exnet.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2012年03月05日 22时00分33秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zilong.Zhu (), zilong.whu@gmail.com
 *        Company:  edu.whu
 *
 * =====================================================================================
 */
#ifndef __EXNET_H__
#define __EXNET_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <unistd.h>
#include <stdint.h>
#include <netinet/in.h>
#include "nethead.h"

#ifndef __cplusplus
typedef enum
{
    false,
    true,
} bool;
#endif

enum
{
    /*
     * NET_OP_NOTIFY is used by framework when:
     *   1. the user has detached a socket by calling epex_detach (with NET_EDETACHED)
     *   2. some error ocurred on socket (with NET_ERROR)
     *   3. socket becomes idle (with NET_EIDLE)
     */
    NET_OP_NOTIFY = 1,
    NET_OP_READ,
    NET_OP_WRITE,
    /*
     * NET_OP_ACCEPT is used by framework when listen socket becomes readable (with NET_DONE)
     */
    NET_OP_ACCEPT,
    /*
     * call epex_connect to connect socket to a sockaddr:
     *   1. connect ok, notified with NET_OP_CONNECT & NET_DONE
     *   2. connect fail, notified with NET_OP_CONNECT & NET_ERROR
     *   3. connect timeout, notified with NET_OP_CONNECT & NET_ETIMEOUT
     */
    NET_OP_CONNECT,
};

enum
{
    NET_DONE = 1,                               /* request has been processed successfully */
    NET_PART_DONE,                              /* has read some data, for read request only */
    NET_EDETACHED,                              /* socket is detached by user */
    NET_ETIMEOUT,                               /* task is timeout */
    NET_EIDLE,                                  /* socket becomes idle */
    NET_ECLOSED,                                /* peer has closed socket */
    NET_ERROR,                                  /* error occured */
};

typedef struct _netresult
{
    int _sock_fd;
    uint16_t _op_type;
    uint16_t _status;
    int _errno;

    void *_buffer;
    size_t _curpos;
    size_t _size;

    void *_user_ptr;                            /* task specific user data */
    void *_user_ptr2;                           /* stub specific user data */
} netresult_t;

typedef void *epex_t;

epex_t epex_open(int size);
void epex_close(epex_t ptr);
bool epex_attach(epex_t ptr, int sock_fd, void *user_arg, int ms); /* ms is max idle time */
bool epex_detach(epex_t ptr, int sock_fd, void **p_user_arg);
bool epex_listen(epex_t ptr, int sock_fd, void *user_arg);
bool epex_listen2(epex_t ptr, int sock_fd, int backlog, void *user_arg);
bool epex_connect(epex_t ptr, int sock_fd, struct sockaddr_in *addr, void *user_arg, int connect_ms, int idle_ms);
bool epex_enable_notify(epex_t ptr, int sock_fd);
bool epex_disable_notify(epex_t ptr, int sock_fd);
bool epex_read(epex_t ptr, int sock_fd, void *buf, size_t size, void *user_arg, int ms);
bool epex_read_any(epex_t ptr, int sock_fd, void *buf, size_t size, void *user_arg, int ms);
bool epex_write(epex_t ptr, int sock_fd, const void *buf, size_t size, void *user_arg, int ms);
ssize_t epex_poll(epex_t ep, netresult_t *results, size_t size);

#ifdef __cplusplus
}
#endif

#endif
