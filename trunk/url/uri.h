/*
 * =====================================================================================
 *
 *       Filename:  uri.h
 *
 *    Description:  uri RFC3896
 *
 *        Version:  1.0
 *        Created:  2011年04月26日 18时34分36秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zilong.Zhu (), zilong.whu@gmail.com
 *        Company:  edu.whu
 *
 * =====================================================================================
 */
#ifndef __URI_RFC_3896_H__
#define __URI_RFC_3896_H__

#include <stddef.h>
#include <stdint.h>

enum 
{
	URI_HOST_IPV4 = 1,
	URI_HOST_IPV6,
	URI_HOST_IPVF,
	URI_HOST_REGNAME,
};

enum
{
	URI_SCHEME = 0x01,
	URI_AUTHORITY = 0x02,
	URI_PATH = 0x04,
	URI_QUERY = 0x08,
	URI_FRAGMENT = 0x10,
};

typedef struct uri_s
{
	uint16_t parts;
	uint16_t host_type;
	char *scheme;
	char *userinfo;
	union
	{
		char *name;
		char *ipv4;
		char *ipv6;
		char *ipvf;
		char *regname;
	};
	char *port;
	char *path;
	char *query;
	char *fragment;
} uri_t;

const char *parse_uri_ref(const char *str);
uri_t *parse_uri_ref_2(const char *str);
uri_t *resolve_uri(uri_t *base, uri_t *rel);
size_t recomp_uri(uri_t *uri, char *buf, size_t len);

void remove_dots(char *path);
void normalize_uri(uri_t *uri);

uri_t *uri_cpy(uri_t *src);
void uri_free(uri_t *uri);

char *new_strncat(const char *first, ...);

#endif
