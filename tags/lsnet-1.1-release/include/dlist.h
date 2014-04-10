/*
 * =====================================================================================
 *
 *       Filename:  dlist.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2012年03月06日 20时40分37秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zilong.Zhu (), zilong.whu@gmail.com
 *        Company:  edu.whu
 *
 * =====================================================================================
 */

#ifndef __DLIST_H__
#define __DLIST_H__

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct __dlist
{
    struct __dlist *_prev;
    struct __dlist *_next;
} __dlist_t;

#define DLIST_EMPTY(list) ((list)->_next == (list))

#define DLIST_INIT(list) do {                                             \
    (list)->_prev = (list)->_next = (list);                               \
} while(0)

#define DLIST_PRE(list) ((list)->_prev)

#define DLIST_NEXT(list) ((list)->_next)

#define DLIST_INSERT_B(cur, next) do {                                    \
    (cur)->_next = (next);                                                \
    (cur)->_prev = (next)->_prev;                                         \
    (next)->_prev->_next = (cur);                                         \
    (next)->_prev = (cur);                                                \
} while(0)

#define DLIST_INSERT_A(pre, cur) do {                                     \
    (cur)->_next = (pre)->_next;                                          \
    (cur)->_prev = (pre);                                                 \
    (pre)->_next->_prev = (cur);                                          \
    (pre)->_next = (cur);                                                 \
} while(0)

#define DLIST_REMOVE(cur) do {                                            \
    (cur)->_prev->_next = (cur)->_next;                                   \
    (cur)->_next->_prev = (cur)->_prev;                                   \
    DLIST_INIT(cur);                                                      \
} while(0)

#define DLIST_COUNT(list, num) do {                                       \
    __dlist_t *tmp_cur;                                                   \
    (num) = 0;                                                            \
    for ( tmp_cur = DLIST_NEXT(list);                                     \
            tmp_cur != (list);                                            \
            tmp_cur = DLIST_NEXT(tmp_cur)                                 \
            )                                                             \
    {                                                                     \
        ++(num);                                                          \
    }                                                                     \
} while(0)

#ifdef __cplusplus
}
#endif

#endif
