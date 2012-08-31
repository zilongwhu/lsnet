/*
 * =====================================================================================
 *
 *       Filename:  slist.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2012年03月09日 00时12分35秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zilong.Zhu (), zilong.whu@gmail.com
 *        Company:  edu.whu
 *
 * =====================================================================================
 */

#ifndef __SLIST_H__
#define __SLIST_H__

typedef struct __slist
{
	struct __slist *_next;
} __slist_t;

#define SLIST_EMPTY(list) (NULL == (list)->_next)

#define SLIST_INIT(list) do {                                             \
	(list)->_next = NULL;                                             \
} while(0)

#define SLIST_NEXT(list) ((list)->_next)

#define SLIST_INSERT_A(pre, cur) do {                                     \
	(cur)->_next = (pre)->_next;                                      \
	(pre)->_next = (cur);                                             \
} while(0)

#define SLIST_REMOVE(pre, cur) do {                                       \
	(pre)->_next = (cur)->_next;                                      \
	SLIST_INIT(cur);                                                  \
} while(0)

#endif
