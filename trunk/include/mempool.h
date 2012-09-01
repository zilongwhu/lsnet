/*
 * =====================================================================================
 *
 *       Filename:  mempool.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2012年03月10日 00时44分34秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zilong.Zhu (), zilong.whu@gmail.com
 *        Company:  edu.whu
 *
 * =====================================================================================
 */
#ifndef __MEMPOOL_H__
#define __MEMPOOL_H__

#include <stddef.h>

typedef void *mempool_t;

mempool_t mp_init(size_t elem_size);
void *mp_alloc(mempool_t mp);
void mp_free(mempool_t mp, void *ptr);
void mp_renew(mempool_t mp);
void mp_close(mempool_t mp);

#endif
