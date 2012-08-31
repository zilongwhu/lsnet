/*
 * =====================================================================================
 *
 *       Filename:  client.c
 *
 *    Description:  客户端
 *
 *        Version:  1.0
 *        Created:  2012年05月23日 15时24分57秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zilong.Zhu (), zilong.whu@gmail.com
 *        Company:  edu.whu
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "log.h"
#include "server.h"

char buffer[] = "hello";
char buffer2[256];

int on_accept(ls_srv_t srv, int sock)
{
	return ls_srv_read_any(srv, sock, buffer2, sizeof buffer, NULL, 100);
}

int proc(ls_srv_t srv, const netresult_t *result)
{
	if ( NULL == result )
	{
		WARNING("how can it be.");
		return -1;
	}
	switch (result->_op_type)
	{
		case NET_OP_NOTIFY:
			return 0;
			break;
		case NET_OP_READ:
			if ( NET_DONE == result->_status || NET_PART_DONE == result->_status )
			{
				DEBUG("read ok[%s]", (char *)result->_buffer);
				if ( ls_srv_write(srv, result->_sock_fd, buffer2, result->_curpos, NULL, -1) < 0 )
				{
					return -1;
				}
				DEBUG("submit write request[%s]", buffer2);
			}
			else if ( NET_ETIMEOUT == result->_status )
			{
				DEBUG("sock[%d] read timeout", result->_sock_fd);
			}
			else if ( NET_EIDLE == result->_status )
			{
				DEBUG("sock[%d] timeout", result->_sock_fd);
			}
			else if ( NET_ECLOSED == result->_status )
			{
				DEBUG("sock[%d] closed", result->_sock_fd);
				return -1;
			}
			break;
		case NET_OP_WRITE:
			if ( NET_DONE == result->_status )
			{
				DEBUG("write ok");
				if ( ls_srv_read_any(srv, result->_sock_fd, buffer2, sizeof buffer, NULL, -1) < 0 )
				{
					return -1;
				}
				DEBUG("submit read request");
			}
			else
			{
				return -1;
			}
			break;
	}
	return 0;
}

int main(int argc, char *argv[])
{
	struct sockaddr_in addr;
	bzero(&addr, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(12345);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	ls_srv_t srv = ls_srv_create(1024, proc, on_accept);
	ls_srv_listen(srv, (struct sockaddr *)&addr, sizeof(addr), 5);
	ls_srv_set_idle_timeout(srv, 500);
	ls_srv_run(srv);
	ls_srv_close(srv);
	return 0;
}
