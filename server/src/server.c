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
#include "exnet.h"

int main(int argc, char *argv[])
{
	netresult_t result;
	char buffer[] = "hello";
	char buffer2[256];

	epex_t handle = epex_open(1024);
	if ( NULL == handle )
	{
		return -1;
	}
	struct sockaddr_in addr;
	bzero(&addr, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(12345);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int lsfd = socket(AF_INET, SOCK_STREAM, 0);
	if ( lsfd < 0 )
	{
		return -1;
	}
	if ( bind(lsfd, (struct sockaddr *)&addr, sizeof addr) < 0 )
	{
		return -1;
	}
	if ( epex_listen2(handle, lsfd, 10, NULL) )
	{
		ssize_t sz;
		do
		{
			sz = epex_poll(handle, &result, 1);
			if ( 1 != sz )
			{
				continue;
			}
			switch (result._op_type)
			{
				case NET_OP_NOTIFY:
					break;
				case NET_OP_ACCEPT:
					{
						int sock;
						do
						{
							sock = accept(result._sock_fd, NULL, NULL);
							if ( sock >= 0 )
							{
								if ( epex_attach(handle, sock, NULL, 500) )
								{
									epex_read(handle, sock, buffer2, sizeof buffer, NULL, 100);
								}
								else
								{
									close(sock);
								}
							}
						} while (sock >= 0);
					}
					break;
				case NET_OP_READ:
					if ( NET_DONE == result._status )
					{
						DEBUG("read ok[%s]", (char *)result._buffer);
						if ( !epex_write(handle, result._sock_fd, buffer2, sizeof buffer, NULL, -1) )
						{
							goto OUT;
						}
						DEBUG("submit write request[%s]", buffer2);
					}
					else if ( NET_ETIMEOUT == result._status )
					{
						DEBUG("sock[%d] read timeout", result._sock_fd);
					}
					else if ( NET_EIDLE == result._status )
					{
						DEBUG("sock[%d] timeout", result._sock_fd);
						close(result._sock_fd);
					}
					break;
				case NET_OP_WRITE:
					if ( NET_DONE == result._status )
					{
						DEBUG("write ok");
						if ( !epex_read(handle, result._sock_fd, buffer2, sizeof buffer, NULL, -1) )
						{
							goto OUT;
						}
						DEBUG("submit read request");
					}
					else
					{
						goto OUT;
					}
					break;
			}
		} while(1);
OUT:
		;
	}
	epex_close(handle);
	return 0;
}
