/*
 * =====================================================================================
 *
 *       Filename:  ip4addr.c
 *
 *    Description:  ipv4 address parser
 *
 *        Version:  1.0
 *        Created:  2012年05月30日 22时50分14秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zilong.Zhu (), zilong.whu@gmail.com
 *        Company:  edu.whu
 *
 * =====================================================================================
 */
#include <ctype.h>
#include <stdint.h>
#include "ip4addr.h"

#ifndef NULL
#define NULL	0
#endif

// dec-octet     = DIGIT                 ; 0-9
//               / %x31-39 DIGIT         ; 10-99
//               / "1" 2DIGIT            ; 100-199
//               / "2" %x30-34 DIGIT     ; 200-249
//               / "25" %x30-35          ; 250-255
const char *parse_dec_octet(const char *str, int *pval)
{
	if ( !isdigit(*str) )
	{
		return NULL;
	}
	int val = *str - '0';
        if ( '0' == *str )                      /* 0 */
	{
		++str;
	}
        else if ( '1' == *str )                 /* 1 | 10-19 | 100-199 */
	{
		++str;
		if ( isdigit(*str) )            /* 10-19 | 100-199 */
		{
			val = val * 10 + (*str - '0');
			++str;
			if ( isdigit(*str) )    /* 100-199 */
			{
				val = val * 10 + (*str - '0');
				++str;
			}
		}
	}
        else if ( '2' == *str )                 /* 2 | 20-29 | 200-255 */
	{
		++str;
		if ( isdigit(*str) )            /* 20-29 | 200-255 */
		{
			val = val * 10 + (*str - '0');
			++str;
			if ( isdigit(*str) )    /* 200-255 */
			{
				val = val * 10 + (*str - '0');
				if ( val <= 255 )
				{
					++str;
				}
				else
				{
					val /= 10;
				}
			}
		}
	}
        else                                    /* 3-9 | 30-99 */
	{
		++str;
		if ( isdigit(*str) )            /* 30-99 */
		{
			val = val * 10 + (*str - '0');
			++str;
		}
	}
	if ( NULL != pval )
	{
		*pval = val;
	}
	return str;
}

// IPv4address   = dec-octet "." dec-octet "." dec-octet "." dec-octet
const char *parse_ipv4(const char *str, uint32_t *pval)
{
	int tval;
	uint32_t val = 0;

	int i = 0;
	while (1)
	{
		str = parse_dec_octet(str, &tval);
		if ( NULL == str )
		{
			return NULL;
		}
		val |= (((uint32_t)tval) << ((3 - i) << 3));
		if ( 3 == i )
		{
			break;
		}
		if ( '.' != *str )
		{
			return NULL;
		}
		++str;
		++i;
	}
	if ( NULL == pval )
	{
		*pval = val;
	}
	return str;
}
