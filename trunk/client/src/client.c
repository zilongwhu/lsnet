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

	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if ( sock < 0 )
	{
		return -1;
	}
	int first = 1;
	if ( epex_connect(handle, sock, &addr, NULL, 100) )
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
				case NET_OP_CONNECT:
					if ( NET_DONE == result._status )
					{
						NOTICE("connect ok");
						if ( !epex_write(handle, sock,
									"POST /index.html http/1.1\r\ntransfer-encoding:chunked\r\n\r\n\r\n   \r",
									sizeof("POST /index.html http/1.1\r\ntransfer-encoding:chunked\r\n\r\n\r\n   \r") - 1,
									NULL, -1) )
						{
							goto OUT;
						}
						NOTICE("submit write request[%s]", buffer);
					}
					else
					{
						goto OUT;
					}
					break;
				case NET_OP_WRITE:
					if ( NET_DONE == result._status )
					{
						NOTICE("submit read request");
						usleep(50*1000);
						epex_write(handle, sock,
									"\n 00010 \"\r\n \\\"a\r\n\"\r\n0123456789ABCDEF\r\n 001\r\nL\r\n 0 \r\n first : abc\r\n efg\r\nnext  :  \r\n every    one   \r\n\r\n",
									sizeof("\n 00010 \"\r\n \\\"a\r\n\"\r\n0123456789ABCDEF\r\n 001\r\nL\r\n 0 \r\n first : abc\r\n efg\r\nnext  :  \r\n every    one   \r\n\r\n") - 1,
									NULL, -1);
						if ( 0 == first )
						{
							goto OUT;
						}
						else
						{
							first = 0;
						}
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
