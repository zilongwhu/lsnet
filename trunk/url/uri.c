/*
 * =====================================================================================
 *
 *       Filename:  uri.c
 *
 *    Description:  uri implements
 *
 *        Version:  1.0
 *        Created:  2011年04月26日 18时35分46秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zilong.Zhu (), zilong.whu@gmail.com
 *        Company:  edu.whu
 *
 * =====================================================================================
 */
#include "uri.h"
#include "ip4addr.h"
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

char *strndup(const char *str, size_t n);

#ifndef NULL
#define NULL	0
#endif

enum
{
	URI_UNRESERVED = 0x01,
	URI_RESERVED = 0x02,
	URI_GENDELIM = 0x04,
	URI_SUBDELIM = 0x08,
	URI_HEX	 = 0x10,
};

#define IS_UNRESERVED(ch)	(s_uri_char_traits[(unsigned char)(ch)] & URI_UNRESERVED)
#define IS_RESERVED(ch)		(s_uri_char_traits[(unsigned char)(ch)] & URI_RESERVED)
#define IS_GENDELIMS(ch)	(s_uri_char_traits[(unsigned char)(ch)] & URI_GENDELIM)
#define IS_SUBDELIMS(ch)	(s_uri_char_traits[(unsigned char)(ch)] & URI_SUBDELIM)
#define IS_HEX(ch)		(s_uri_char_traits[(unsigned char)(ch)] & URI_HEX)

static const unsigned char s_uri_char_traits[] =
{
	/*    0 */ 0x00, /*    1 */ 0x00, /*    2 */ 0x00, /*    3 */ 0x00, /*    4 */ 0x00, /*    5 */ 0x00, /*    6 */ 0x00, /*    7 */ 0x00,
	/*    8 */ 0x00, /*    9 */ 0x00, /*   10 */ 0x00, /*   11 */ 0x00, /*   12 */ 0x00, /*   13 */ 0x00, /*   14 */ 0x00, /*   15 */ 0x00,
	/*   16 */ 0x00, /*   17 */ 0x00, /*   18 */ 0x00, /*   19 */ 0x00, /*   20 */ 0x00, /*   21 */ 0x00, /*   22 */ 0x00, /*   23 */ 0x00,
	/*   24 */ 0x00, /*   25 */ 0x00, /*   26 */ 0x00, /*   27 */ 0x00, /*   28 */ 0x00, /*   29 */ 0x00, /*   30 */ 0x00, /*   31 */ 0x00,
	/*      */ 0x00, /*    ! */ 0x0A, /*    " */ 0x00, /*    # */ 0x06, /*    $ */ 0x0A, /*    % */ 0x00, /*    & */ 0x0A, /*    ' */ 0x0A,
	/*    ( */ 0x0A, /*    ) */ 0x0A, /*    * */ 0x0A, /*    + */ 0x0A, /*    , */ 0x0A, /*    - */ 0x01, /*    . */ 0x01, /*    / */ 0x06,
	/*    0 */ 0x11, /*    1 */ 0x11, /*    2 */ 0x11, /*    3 */ 0x11, /*    4 */ 0x11, /*    5 */ 0x11, /*    6 */ 0x11, /*    7 */ 0x11,
	/*    8 */ 0x11, /*    9 */ 0x11, /*    : */ 0x06, /*    ; */ 0x0A, /*    < */ 0x00, /*    = */ 0x0A, /*    > */ 0x00, /*    ? */ 0x06,
	/*    @ */ 0x06, /*    A */ 0x11, /*    B */ 0x11, /*    C */ 0x11, /*    D */ 0x11, /*    E */ 0x11, /*    F */ 0x11, /*    G */ 0x01,
	/*    H */ 0x01, /*    I */ 0x01, /*    J */ 0x01, /*    K */ 0x01, /*    L */ 0x01, /*    M */ 0x01, /*    N */ 0x01, /*    O */ 0x01,
	/*    P */ 0x01, /*    Q */ 0x01, /*    R */ 0x01, /*    S */ 0x01, /*    T */ 0x01, /*    U */ 0x01, /*    V */ 0x01, /*    W */ 0x01,
	/*    X */ 0x01, /*    Y */ 0x01, /*    Z */ 0x01, /*    [ */ 0x06, /*    \ */ 0x00, /*    ] */ 0x06, /*    ^ */ 0x00, /*    _ */ 0x01,
	/*    ` */ 0x00, /*    a */ 0x11, /*    b */ 0x11, /*    c */ 0x11, /*    d */ 0x11, /*    e */ 0x11, /*    f */ 0x11, /*    g */ 0x01,
	/*    h */ 0x01, /*    i */ 0x01, /*    j */ 0x01, /*    k */ 0x01, /*    l */ 0x01, /*    m */ 0x01, /*    n */ 0x01, /*    o */ 0x01,
	/*    p */ 0x01, /*    q */ 0x01, /*    r */ 0x01, /*    s */ 0x01, /*    t */ 0x01, /*    u */ 0x01, /*    v */ 0x01, /*    w */ 0x01,
	/*    x */ 0x01, /*    y */ 0x01, /*    z */ 0x01, /*    { */ 0x00, /*    | */ 0x00, /*    } */ 0x00, /*    ~ */ 0x01, /*  127 */ 0x00,
	/*  128 */ 0x00, /*  129 */ 0x00, /*  130 */ 0x00, /*  131 */ 0x00, /*  132 */ 0x00, /*  133 */ 0x00, /*  134 */ 0x00, /*  135 */ 0x00,
	/*  136 */ 0x00, /*  137 */ 0x00, /*  138 */ 0x00, /*  139 */ 0x00, /*  140 */ 0x00, /*  141 */ 0x00, /*  142 */ 0x00, /*  143 */ 0x00,
	/*  144 */ 0x00, /*  145 */ 0x00, /*  146 */ 0x00, /*  147 */ 0x00, /*  148 */ 0x00, /*  149 */ 0x00, /*  150 */ 0x00, /*  151 */ 0x00,
	/*  152 */ 0x00, /*  153 */ 0x00, /*  154 */ 0x00, /*  155 */ 0x00, /*  156 */ 0x00, /*  157 */ 0x00, /*  158 */ 0x00, /*  159 */ 0x00,
	/*  160 */ 0x00, /*  161 */ 0x00, /*  162 */ 0x00, /*  163 */ 0x00, /*  164 */ 0x00, /*  165 */ 0x00, /*  166 */ 0x00, /*  167 */ 0x00,
	/*  168 */ 0x00, /*  169 */ 0x00, /*  170 */ 0x00, /*  171 */ 0x00, /*  172 */ 0x00, /*  173 */ 0x00, /*  174 */ 0x00, /*  175 */ 0x00,
	/*  176 */ 0x00, /*  177 */ 0x00, /*  178 */ 0x00, /*  179 */ 0x00, /*  180 */ 0x00, /*  181 */ 0x00, /*  182 */ 0x00, /*  183 */ 0x00,
	/*  184 */ 0x00, /*  185 */ 0x00, /*  186 */ 0x00, /*  187 */ 0x00, /*  188 */ 0x00, /*  189 */ 0x00, /*  190 */ 0x00, /*  191 */ 0x00,
	/*  192 */ 0x00, /*  193 */ 0x00, /*  194 */ 0x00, /*  195 */ 0x00, /*  196 */ 0x00, /*  197 */ 0x00, /*  198 */ 0x00, /*  199 */ 0x00,
	/*  200 */ 0x00, /*  201 */ 0x00, /*  202 */ 0x00, /*  203 */ 0x00, /*  204 */ 0x00, /*  205 */ 0x00, /*  206 */ 0x00, /*  207 */ 0x00,
	/*  208 */ 0x00, /*  209 */ 0x00, /*  210 */ 0x00, /*  211 */ 0x00, /*  212 */ 0x00, /*  213 */ 0x00, /*  214 */ 0x00, /*  215 */ 0x00,
	/*  216 */ 0x00, /*  217 */ 0x00, /*  218 */ 0x00, /*  219 */ 0x00, /*  220 */ 0x00, /*  221 */ 0x00, /*  222 */ 0x00, /*  223 */ 0x00,
	/*  224 */ 0x00, /*  225 */ 0x00, /*  226 */ 0x00, /*  227 */ 0x00, /*  228 */ 0x00, /*  229 */ 0x00, /*  230 */ 0x00, /*  231 */ 0x00,
	/*  232 */ 0x00, /*  233 */ 0x00, /*  234 */ 0x00, /*  235 */ 0x00, /*  236 */ 0x00, /*  237 */ 0x00, /*  238 */ 0x00, /*  239 */ 0x00,
	/*  240 */ 0x00, /*  241 */ 0x00, /*  242 */ 0x00, /*  243 */ 0x00, /*  244 */ 0x00, /*  245 */ 0x00, /*  246 */ 0x00, /*  247 */ 0x00,
	/*  248 */ 0x00, /*  249 */ 0x00, /*  250 */ 0x00, /*  251 */ 0x00, /*  252 */ 0x00, /*  253 */ 0x00, /*  254 */ 0x00, /*  255 */ 0x00,
};

// unreserved    = ALPHA / DIGIT / "-" / "." / "_" / "~"
static int is_unreserved(char ch)
{
	return isalnum(ch) || '-' == ch || '.' == ch || '_' == ch || '~' == ch;
}

// gen-delims    = ":" / "/" / "?" / "#" / "[" / "]" / "@"
static int is_gendelims(char ch)
{
	return ':' == ch || '/' == ch || '?' == ch || '#' == ch || '[' == ch || ']' == ch || '@' == ch;
}

// sub-delims    = "!" / "$" / "&" / "'" / "(" / ")"
//               / "*" / "+" / "," / ";" / "="
static int is_subdelims(char ch)
{
	return '!' == ch || '$' == ch || '&' == ch || '\'' == ch || '(' == ch || ')' == ch
		|| '*' == ch || '+' == ch || ',' == ch || ';' == ch || '=' == ch;
}

static int is_reserved(char ch)
{
	return is_gendelims(ch) || is_subdelims(ch);
}

static int is_hex(char ch)
{
	return isdigit(ch) || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
}

static void print_metrix(void)
{
	int i;
	int v;
	printf("\n{\n");
	for ( i = 0; i < 256; ++i )
	{
		v = 0;
		if ( is_unreserved(i) )
			v |= URI_UNRESERVED;
		if ( is_reserved(i) )
			v |= URI_RESERVED;
		if ( is_gendelims(i) )
			v |= URI_GENDELIM;
		if ( is_subdelims(i) )
			v |= URI_SUBDELIM;
		if ( is_hex(i) )
			v |= URI_HEX;
		if ( isprint(i) )
			printf("\t/* %3c */ 0x%02X,", (char)i, v);
		else
			printf("\t/* %3d */ 0x%02X,", i, v);
		if ( i % 8 == 7 )
			printf("\n");
	}
	printf("}\n");
}

// scheme        = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
static const char *parse_scheme(const char *str)
{
	if ( !isalpha(*str) )
	{
		return NULL;
	}
	++str;
	while ( isalnum(*str) || '+' == *str || '-' == *str || '.' == *str )
	{
		++str;
	}
	return str;
}

//  h16         = 1*4HEXDIG
//              ; 16 bits of address represented in hexadecimal
static const char *parse_h16(const char *str)
{
	int i;
	for ( i = 0; i < 4; ++i, ++str )
	{
		if ( !IS_HEX(*str) )
		{
			break;
		}
	}
	if ( 0 == i )
	{
		return NULL;
	}
	return str;
}

//  IPv6address =                            6( h16 ":" ) ls32
//              /                       "::" 5( h16 ":" ) ls32
//              / [               h16 ] "::" 4( h16 ":" ) ls32
//              / [ *1( h16 ":" ) h16 ] "::" 3( h16 ":" ) ls32
//              / [ *2( h16 ":" ) h16 ] "::" 2( h16 ":" ) ls32
//              / [ *3( h16 ":" ) h16 ] "::"    h16 ":"   ls32
//              / [ *4( h16 ":" ) h16 ] "::"              ls32
//              / [ *5( h16 ":" ) h16 ] "::"              h16
//              / [ *6( h16 ":" ) h16 ] "::"
//
//  ls32        = ( h16 ":" h16 ) / IPv4address
//              ; least-significant 32 bits of address
static const char *parse_ipv6(const char *str)
{
	const char *ptr = str;
	const char *tmp;
	int i = 0, n;

	if ( ':' == *ptr && ':' == *(ptr + 1) ) /* 以::开头 */
	{
		ptr += 2;
	}
	else for ( ; i < 8; ++i )
	{
		tmp = parse_h16(ptr);
		if ( NULL == tmp )
		{
			if ( i > 0 && ':' == *ptr ) /* :: */
			{
				++ptr;
				break;
			}
                        else if ( 8 == i + 2 )  /* 可能是ipv4 */
			{
				return parse_ipv4(ptr, NULL);
			}
			return NULL;
		}
                if ( 8 == i + 1 )               /* 全部都是h16 */
		{
			return tmp;
		}
                if ( ':' != *tmp++ )
		{
			return NULL;
		}
		ptr = tmp;
	}
	n = 8 - i - 1;                          /* ::之后最多能出现n次h16 */
	for ( i = 0; i < n; ++i )
	{
		tmp = parse_h16(ptr);
		if ( NULL == tmp )
		{
			if ( i + 2 <= n )
			{
				tmp = parse_ipv4(ptr, NULL);
				if ( NULL != tmp ) /* ipv4 */
				{
					return tmp;
				}
			}
			if ( i > 0 )
			{
				--ptr;
			}
			return ptr;
		}
                if ( n - 1 == i )               /* 最后一个h16 */
		{
			return tmp;
		}
                if ( ':' != *tmp )
		{
			return tmp;
		}
		ptr = tmp + 1;
	}
	return NULL;
}

// IPvFuture     = "v" 1*HEXDIG "." 1*( unreserved / sub-delims / ":" )
static const char *parse_ip_future(const char *str)
{
	int cnt;
	if ( 'v' != *str++ )
	{
		return NULL;
	}
	for ( cnt = 0; IS_HEX(*str); ++cnt, ++str )
	{ }
	if ( 0 == cnt )
	{
		return NULL;
	}
	if ( '.' != *str++ )
	{
		return NULL;
	}
	for ( cnt = 0; IS_UNRESERVED(*str) || IS_SUBDELIMS(*str) || ':' == *str; ++cnt, ++str )
	{ }
	if ( 0 == cnt )
	{
		return NULL;
	}
	return str;
}

static const char *parse_ip_literal(const char *str)
{
	const char *ptr;
	if ( '[' != *str++ )
	{
		return NULL;
	}
	if ( NULL == (ptr = parse_ipv6(str))
			&& NULL == (ptr = parse_ip_future(str)) )
	{
		return NULL;
	}
	str = ptr;
	if ( ']' != *str++ )
	{
		return NULL;
	}
	return str;
}

// reg-name      = *( unreserved / pct-encoded / sub-delims )
static const char *parse_regname(const char *str)
{
	while (1)
	{
		if ( '%' == *str )
		{
			if ( IS_HEX(*(str + 1)) && IS_HEX(*(str + 2)) )
			{
				str += 3;
			}
			else
			{
				break;
			}
		}
		else if ( IS_UNRESERVED(*str) || IS_SUBDELIMS(*str) )
		{
			++str;
		}
		else
		{
			break;
		}
	}
	return str;
}

static const char *parse_host(const char *str, uri_t *uri)
{
	const char *ptr;
	if ( NULL != (ptr = parse_ip_literal(str)) )
	{
		if ( uri )
		{
			if ( 'v' == *(str + 2) )
			{
				uri->ipvf = strndup(str + 1, ptr - str - 2);
				uri->host_type = URI_HOST_IPVF;
			}
			else
			{
				uri->ipv6 = strndup(str + 1, ptr - str - 2);
				uri->host_type = URI_HOST_IPV6;
			}
		}
	}
	else if ( NULL != (ptr = parse_ipv4(str, NULL)) )
	{
		if ( uri )
		{
			uri->ipv4 = strndup(str, ptr - str);
			uri->host_type = URI_HOST_IPV4;
		}
	}
	else
	{
		ptr = parse_regname(str);
		if ( uri )
		{
			if ( ptr > str )
			{
				uri->regname = strndup(str, ptr - str);
			}
			uri->host_type = URI_HOST_REGNAME;
		}
	}
	return ptr;
}

static const char *parse_port(const char *str)
{
	while ( isdigit(*str) )
	{
		++str;
	}
	return str;
}

// userinfo      = *( unreserved / pct-encoded / sub-delims / ":" )
static const char *parse_userinfo(const char *str)
{
	while (1)
	{
		if ( '%' == *str )
		{
			if ( IS_HEX(*(str + 1)) && IS_HEX(*(str + 2)) )
			{
				str += 3;
			}
			else
			{
				break;
			}
		}
		else if ( ':' == *str || IS_UNRESERVED(*str) || IS_SUBDELIMS(*str) )
		{
			++str;
		}
		else
		{
			break;
		}
	}
	return str;
}

static const char *parse_authority(const char *str, uri_t *uri)
{
	const char *ptr = parse_userinfo(str);
	if ( '@' == *ptr )
	{
		if ( uri && ptr > str )
		{
			uri->userinfo = strndup(str, ptr - str);
		}
		str = ptr + 1;
	}
	str = parse_host(str, uri);
	if ( ':' == *str )
	{
		++str;
		ptr = parse_port(str);
		if ( uri && ptr > str )
		{
			uri->port = strndup(str, ptr - str);
		}
		str = ptr;
	}
	return str;
}

// pchar         = unreserved / pct-encoded / sub-delims / ":" / "@"
static const char *parse_pchars(const char *str)
{
	while (1)
	{
		if ( '%' == *str )
		{
			if ( IS_HEX(*(str + 1)) && IS_HEX(*(str + 2)) )
			{
				str += 3;
			}
			else
			{
				break;
			}
		}
		else if ( ':' == *str || '@' == *str || IS_UNRESERVED(*str) || IS_SUBDELIMS(*str) )
		{
			++str;
		}
		else
		{
			break;
		}
	}
	return str;
}

// path          = path-abempty    ; begins with "/" or is empty
//               / path-absolute   ; begins with "/" but not "//"
//               / path-noscheme   ; begins with a non-colon segment
//               / path-rootless   ; begins with a segment
//               / path-empty      ; zero characters
//
// path-abempty  = *( "/" segment )
// path-absolute = "/" [ segment-nz *( "/" segment ) ]
// path-noscheme = segment-nz-nc *( "/" segment )
// path-rootless = segment-nz *( "/" segment )
// path-empty    = 0<pchar>
//
// segment       = *pchar
// segment-nz    = 1*pchar
// segment-nz-nc = 1*( unreserved / pct-encoded / sub-delims / "@" )
//                 ; non-zero-length segment without any colon ":"

static const char *parse_segment_nz_nc(const char *str)
{
	const char *ptr = str;
	while (1)
	{
		if ( ':' == *ptr )
		{
			break;
		}
		else if ( '%' == *ptr )
		{
			if ( IS_HEX(*(ptr + 1)) && IS_HEX(*(ptr + 2)) )
			{
				ptr += 3;
			}
			else
			{
				break;
			}
		}
		else if ( '@' == *ptr || IS_UNRESERVED(*ptr) || IS_SUBDELIMS(*ptr) )
		{
			++ptr;
		}
		else
		{
			break;
		}
	}
	if ( ptr > str )
	{
		return ptr;
	}
	return NULL;
}

// hier-part     = "//" authority path-abempty
//               / path-absolute
//               / path-rootless
//               / path-empty
// relative-part = "//" authority path-abempty
//               / path-absolute
//               / path-noscheme
//               / path-empty
// hor: 0表示hier part，1表示relative part。
static const char *parse_hr_part(const char *str, uri_t *uri, int hor)
{
	const char *path = str;
	const char *ptr;

	if ( '/' == *str )
	{
		++str;
                if ( '/' == *str )              /* "//" authority path-abempty */
		{
			++str;
			if ( uri )
			{
				uri->parts |= URI_AUTHORITY;
			}
			str = parse_authority(str, uri);
			path = str;
		}
                else                            /* path-absolute */
		{
			ptr = parse_pchars(str);
			if ( ptr == str )       /* 没有可选部分 */
			{
				goto LOG;
			}
			str = ptr;
		}
	}
	else
	{
                if ( !hor )                     /* hier part */
		{
			ptr = parse_pchars(str);
                        if ( ptr == str )       /* path-empty */
			{
				goto LOG;
			}
			/* path-rootless */
		}
                else                            /* relative part */
		{
			ptr = parse_segment_nz_nc(str);
                        if ( NULL == ptr )      /* path-empty */
			{
				goto LOG;
			}
			/* path-noscheme */
		}
		str = ptr;
	}
	while ( '/' == *str )
	{
		++str;
		str = parse_pchars(str);
	}
LOG:
	if ( uri && str > path )
	{
		uri->path = strndup(path, str - path);
	}
	return str;
}

// query         = *( pchar / "/" / "?" )
// fragment      = *( pchar / "/" / "?" )
static const char *parse_query_or_fragment(const char *str)
{
	while (1)
	{
		str = parse_pchars(str);
		if ( '/' == *str || '?' == *str )
		{
			++str;
		}
		else
		{
			break;
		}
	}
	return str;
}

// URI           = scheme ":" hier-part [ "?" query ] [ "#" fragment ]
static const char *parse_uri(const char *str, uri_t *uri)
{
	const char *ptr;

	ptr = parse_scheme(str);
	if ( NULL == ptr )
	{
		return NULL;
	}
	if ( uri )
	{
		uri->parts |= URI_SCHEME;
		uri->scheme = strndup(str, ptr - str);
	}
	str = ptr;
	if ( ':' != *str++ )
	{
		if ( uri )
		{
			free(uri->scheme);
			uri->parts &= ~URI_SCHEME;
			uri->scheme = NULL;
		}
		return NULL;
	}
	str = parse_hr_part(str, uri, 0);
	if ( '?' == *str )
	{
		++str;
		ptr = parse_query_or_fragment(str);
		if ( uri )
		{
			uri->parts |= URI_QUERY;
			if ( ptr > str )
			{
				uri->query = strndup(str, ptr - str);
			}
		}
		str = ptr;
	}
	if ( '#' == *str )
	{
		++str;
		ptr = parse_query_or_fragment(str);
		if ( uri )
		{
			uri->parts |= URI_FRAGMENT;
			if ( ptr > str )
			{
				uri->fragment = strndup(str, ptr - str);
			}
		}
		str = ptr;
	}
	return str;
}

// relative-ref  = relative-part [ "?" query ] [ "#" fragment ]
static const char *parse_relative(const char *str, uri_t *uri)
{
	const char *ptr;

	str = parse_hr_part(str, uri, 1);
	if ( '?' == *str )
	{
		++str;
		ptr = parse_query_or_fragment(str);
		if ( uri )
		{
			uri->parts |= URI_QUERY;
			if ( ptr > str )
			{
				uri->query = strndup(str, ptr - str);
			}
		}
		str = ptr;
	}
	if ( '#' == *str )
	{
		++str;
		ptr = parse_query_or_fragment(str);
		if ( uri )
		{
			uri->parts |= URI_FRAGMENT;
			if ( ptr > str )
			{
				uri->fragment = strndup(str, ptr - str);
			}
		}
		str = ptr;
	}
	return str;
}

// URI-reference = URI / relative-ref
const char *parse_uri_ref(const char *str)
{
	const char *ptr;

	ptr = parse_uri(str, NULL);
	if ( NULL == ptr )
	{
		ptr = parse_relative(str, NULL);
	}
	return ptr;
}

uri_t *parse_uri_ref_2(const char *str)
{
	if ( NULL == str )
	{
		return NULL;
	}
	uri_t *uri = (uri_t *)calloc(1, sizeof(*uri));
	if ( NULL == uri )
	{
		return NULL;
	}
	uri->parts |= URI_PATH;
	if ( NULL == parse_uri(str, uri) )
	{
		parse_relative(str, uri);
	}
	normalize_uri(uri);                     /* 规范化 */
	return uri;
}

#define POP_BACK(p)	do {                                         \
	while ( (p) > path && '/' != *((p) - 1) )                    \
	{                                                            \
		--(p);                                               \
	}                                                            \
	if ( (p) > path && '/' == *((p) - 1) )                       \
	{                                                            \
		--(p);                                               \
	}                                                            \
} while(0)

void remove_dots(char *const path)
{
	if ( NULL == path )
	{
		return;
	}

	char *out = path, *in = path;
	while (*in)
	{
		if ( '.' == in[0] )
		{
                        if ( '\0' == in[1] )    /* . */
			{
				++in;
				continue;
			}
                        else if ( '/' == in[1] ) /* ./ */
			{
				in += 2;
				continue;
			}
			else if ( '.' == in[1] )
			{
				if ( '\0' == in[2] ) /* .. */
				{
					in += 2;
					continue;
				}
				else if ( '/' == in[2] ) /* ../ */
				{
					in += 3;
					continue;
				}
			}
		}
		else if ( '/' == in[0] )
		{
			if ( '.' == in[1] )
			{
				if ( '\0' == in[2] )  /* /. */
				{
					++in;
					*in = '/';
					continue;
				}
				else if ( '/' == in[2] ) /* /./ */
				{
					in += 2;
					continue;
				}
				else if ( '.' == in[2] )
				{
					if ( '\0' == in[3] ) /* /.. */
					{
						POP_BACK(out);
						in += 2;
						*in = '/';
						continue;
					}
					else if ( '/' == in[3] ) /* /../ */
					{
						POP_BACK(out);
						in += 3;
						continue;
					}
				}
			}
		}
		if ( out < in )
		{
			do
			{
				*out++ = *in++;
			} while (*in && '/' != *in);
		}
		else
		{
			do
			{
				++in;
			} while (*in && '/' != *in);
			out  = in;
		}
	}
	*out = '\0';
}

static char *merge_uri(uri_t *base, uri_t *rel)
{
	if ( (base->parts & URI_AUTHORITY)
			&& (!base->path || '\0' == *base->path) )
	{
		return new_strncat("/", rel->path, NULL);
	}
	else
	{
		if ( !base->path || '\0' == *base->path )
		{
			return rel->path ? strdup(rel->path) : NULL;
		}
		const char *rm = base->path + strlen(base->path);
		char *res;
		while (rm > base->path && '/' != *(rm - 1))
		{
			--rm;
		}
		size_t len = (rm - base->path) + (rel->path ? strlen(rel->path) : 0);
		res = (char *)malloc(len + 1);
		if ( !res )
		{
			return NULL;
		}
		strncpy(res, base->path, rm - base->path);
		if ( rel->path )
		{
			strcpy(res + (rm - base->path), rel->path);
		}
		res[len] = '\0';
		return res;
	}
}
#define CPY_FIELD(ta, src, f)	do { if (src->f) { ta->f = strdup(src->f); if ( !ta->f ) goto ERR; } } while(0)
#define FREE_FIELD(uri, f)	do { if (uri->f) { free(uri->f); uri->f = NULL; } } while(0)

uri_t *resolve_uri(uri_t *base, uri_t *rel)
{
	if ( !base || !rel )
	{
		return NULL;
	}
	uri_t *res;
	if ( rel->parts & URI_SCHEME )
	{
		res = uri_cpy(rel);
		if ( NULL == res )
		{
			return NULL;
		}
		remove_dots(res->path);
	}
	else
	{
		res = (uri_t *)calloc(1, sizeof *res);
		if ( NULL == res )
		{
			return NULL;
		}
		res->parts |= URI_PATH;

		if ( base->parts & URI_SCHEME )
		{
			res->parts |= URI_SCHEME;
			/* scheme */
			CPY_FIELD(res, base, scheme);
		}
		if ( rel->parts & URI_AUTHORITY )
		{
			res->parts |= URI_AUTHORITY;
			/* authority */
			CPY_FIELD(res, rel, userinfo);
			CPY_FIELD(res, rel, regname);
			CPY_FIELD(res, rel, port);
			/* path */
			CPY_FIELD(res, rel, path);
			remove_dots(res->path);
			/* query */
			if ( rel->parts & URI_QUERY )
			{
				res->parts |= URI_QUERY;
				CPY_FIELD(res, rel, query);
			}
		}
		else
		{
			if ( base->parts & URI_AUTHORITY )
			{
				res->parts |= URI_AUTHORITY;
				/* authority */
				CPY_FIELD(res, base, userinfo);
				CPY_FIELD(res, base, regname);
				CPY_FIELD(res, base, port);
			}
			if ( NULL == rel->path || '\0' == *rel->path )
			{
				CPY_FIELD(res, base, path);
				/* query */
				if ( rel->parts & URI_QUERY )
				{
					res->parts |= URI_QUERY;
					CPY_FIELD(res, rel, query);
				}
				else if ( base->parts & URI_QUERY )
				{
					res->parts |= URI_QUERY;
					CPY_FIELD(res, base, query);
				}
			}
			else
			{
				if ( rel->parts & URI_QUERY )
				{
					res->parts |= URI_QUERY;
					/* query */
					CPY_FIELD(res, rel, query);
				}
				if ( '/' == *rel->path )
				{
					CPY_FIELD(res, rel, path);
				}
				else
				{
					res->path = merge_uri(base, rel);
				}
				remove_dots(res->path);
			}
		}
		if ( rel->parts & URI_FRAGMENT )
		{
			res->parts |= URI_FRAGMENT;
			CPY_FIELD(res, rel, fragment);
		}
	}
	return res;
ERR:
	uri_free(res);
	return NULL;
}

void uri_free(uri_t *uri)
{
	if ( NULL == uri )
	{
		return;
	}
	uri->parts = 0;
	uri->host_type = 0;
	FREE_FIELD(uri, scheme);
	FREE_FIELD(uri, userinfo);
	FREE_FIELD(uri, regname);
	FREE_FIELD(uri, port);
	FREE_FIELD(uri, path);
	FREE_FIELD(uri, query);
	FREE_FIELD(uri, fragment);
	free(uri);
}

uri_t *uri_cpy(uri_t *src)
{
	if ( NULL == src )
	{
		return NULL;
	}
	uri_t *ta = (uri_t *)calloc(1, sizeof *ta);
	if ( NULL == ta )
	{
		return NULL;
	}
	ta->parts = src->parts;
	ta->host_type = src->host_type;
	if ( ta->parts & URI_SCHEME )
	{
		CPY_FIELD(ta, src, scheme);
	}
	if ( ta->parts & URI_AUTHORITY )
	{
		CPY_FIELD(ta, src, userinfo);
		CPY_FIELD(ta, src, regname);
		CPY_FIELD(ta, src, port);
	}
	ta->parts |= URI_PATH;
	CPY_FIELD(ta, src, path);
	if ( ta->parts & URI_QUERY ) 
	{
		CPY_FIELD(ta, src, query);
	}
	if ( ta->parts & URI_FRAGMENT ) 
	{
		CPY_FIELD(ta, src, fragment);
	}
	return ta;
ERR:
	uri_free(ta);
	return NULL;
}

char *new_strncat(const char *first, ...)
{
	char *res;
	const char *tmp;
	size_t len = 0;

	va_list lt;

	va_start(lt, first);
	while ( NULL != (tmp = va_arg(lt, const char *)) )
	{
		len += strlen(tmp);
	}
	va_end(lt);

	res = (char *)malloc(sizeof(char) * (len + 1));
	if ( NULL == res )
	{
		return NULL;
	}
	res[0] = '\0';
	va_start(lt, first);
	while ( NULL != (tmp = va_arg(lt, const char *)) )
	{
		strcat(res, tmp);
	}
	va_end(lt);

	return res;
}

size_t recomp_uri(uri_t *uri, char *buf, size_t len)
{
#define APPEND_STR(ptr)	do {                                         \
	const char *p = (const char *)(ptr);                         \
	if (p)                                                       \
	{                                                            \
		while (*p)                                           \
		{                                                    \
			if ( ret + 1 < len )                         \
			{                                            \
				buf[ret] = *p;                       \
			}                                            \
			++ret;                                       \
			++p;                                         \
		}                                                    \
	}                                                            \
} while(0)

	if ( NULL == uri || NULL == buf )
	{
		return -1;
	}
	size_t ret = 0;
	if ( uri->parts & URI_SCHEME )
	{
		APPEND_STR(uri->scheme);
		APPEND_STR(":");
	}
	if ( uri->parts & URI_AUTHORITY )
	{
		APPEND_STR("//");
		if ( uri->userinfo )
		{
			APPEND_STR(uri->userinfo);
			APPEND_STR("@");
		}
		APPEND_STR(uri->regname);
		if ( uri->port )
		{
			APPEND_STR(":");
			APPEND_STR(uri->port);
		}
	}
	APPEND_STR(uri->path);
	if ( uri->parts & URI_QUERY )
	{
		APPEND_STR("?");
		APPEND_STR(uri->query);
	}
	if ( uri->parts & URI_FRAGMENT )
	{
		APPEND_STR("#");
		APPEND_STR(uri->fragment);
	}
	if ( ret + 1 < len )
	{
		buf[ret] = '\0';
	}
	else
	{
		buf[len - 1] = '\0';
	}
	return ret;

#undef APPEND_STR
}

static void str2lower(char *str)
{
	if ( NULL == str)
	{
		return;
	}
	while(*str)
	{
		*str = tolower(*str);
		++str;
	}
}
static void clean_pchar(char *str)
{
	const char *hex = "0123456789ABCDEF";

	if ( NULL == str )
	{
		return;
	}
	char ch;
	char *frm = str, *to = frm;
	while (*frm)
	{
		if ( '%' == *frm )
		{
			frm[1] = toupper(frm[1]);
			frm[2] = toupper(frm[2]);

			ch = (((unsigned)(strchr(hex, frm[1]) - hex)) << 4)+ strchr(hex, frm[2]) - hex;
			if ( IS_UNRESERVED(ch) )
			{
				*to++ = ch;
			}
			else
			{
				*to++ = frm[0];
				*to++ = frm[1];
				*to++ = frm[2];
			}
			frm += 3;
		}
		else
		{
			*to++ = *frm++;
		}
	}
	*to = '\0';
}

void normalize_uri(uri_t *uri)
{
	if ( NULL == uri )
	{
		return;
	}
	if ( uri->scheme )
	{
		str2lower(uri->scheme);
	}
	if ( uri->regname )
	{
		str2lower(uri->regname);
	}
	if ( uri->path )
	{
		clean_pchar(uri->path);
		remove_dots(uri->path);
	}
	if ( uri->query )
	{
		clean_pchar(uri->query);
	}
	if ( uri->fragment )
	{
		clean_pchar(uri->fragment);
	}
}
