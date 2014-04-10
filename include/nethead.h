/*
 * =====================================================================================
 *
 *       Filename:  nethead.h
 *
 *    Description:  net package header
 *
 *        Version:  1.0
 *        Created:  2013年06月25日 15时54分57秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zilong.Zhu (), zilong.whu@gmail.com
 *        Company:  edu.whu
 *
 * =====================================================================================
 */

#ifndef __NET_HEADER_H__
#define __NET_HEADER_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#define MAGIC_NUM   (0x11190309)

typedef struct net_head_s
{
    char _service[8];
    uint32_t _magic_num;
    uint32_t _body_len;
    uint32_t _reserved;
} net_head_t;

#ifdef __cplusplus
}
#endif

#endif
