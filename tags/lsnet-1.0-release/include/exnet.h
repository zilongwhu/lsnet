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

#include <unistd.h>
#include <stdint.h>
#include <netinet/in.h>
#include "nethead.h"

#ifndef __CPLUSPLUS
typedef enum
{
    false,
    true,
} bool;
#endif

enum
{
    NET_OP_NOTIFY = 1,
    NET_OP_READ,
    NET_OP_WRITE,
    NET_OP_ACCEPT,
    NET_OP_CONNECT,
};

enum
{
    NET_DONE = 1,
    NET_PART_DONE,
    NET_EDETACHED,
    NET_ETIMEOUT,
    NET_EIDLE,
    NET_ECLOSED,
    NET_ERROR,
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

    void *_user_ptr;
    void *_user_ptr2;
} netresult_t;

typedef void *epex_t;

epex_t epex_open(int size);
void epex_close(epex_t ptr);
bool epex_attach(epex_t ptr, int sock_fd, void *user_arg, int ms);
bool epex_detach(epex_t ptr, int sock_fd, void **p_user_arg);
bool epex_listen(epex_t ptr, int sock_fd, void *user_arg);
bool epex_listen2(epex_t ptr, int sock_fd, int backlog, void *user_arg);
bool epex_connect(epex_t ptr, int sock_fd, struct sockaddr_in *addr, void *user_arg, int ms);
bool epex_enable_notify(epex_t ptr, int sock_fd);
bool epex_disable_notify(epex_t ptr, int sock_fd);
bool epex_read(epex_t ptr, int sock_fd, void *buf, size_t size, void *user_arg, int ms);
bool epex_read_any(epex_t ptr, int sock_fd, void *buf, size_t size, void *user_arg, int ms);
bool epex_write(epex_t ptr, int sock_fd, const void *buf, size_t size, void *user_arg, int ms);
ssize_t epex_poll(epex_t ep, netresult_t *results, size_t size);

#endif
