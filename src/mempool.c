/*
 * =====================================================================================
 *
 *       Filename:  mempool.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2012年03月10日 00时44分44秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zilong.Zhu (), zilong.whu@gmail.com
 *        Company:  edu.whu
 *
 * =====================================================================================
 */

#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "dlist.h"
#include "slist.h"
#include "utils.h"
#include "mempool.h"

static long __mp_g_page_size;
static pthread_once_t __mp_g_init_page_size_once = PTHREAD_ONCE_INIT;

static void __mp_init_page_size(void)
{
    __mp_g_page_size = 10 * sysconf(_SC_PAGESIZE);
}

typedef struct __page_head
{
    __dlist_t _list;
    uint16_t _cur_cnt;
    uint16_t _total_cnt;
} __page_head_t;

typedef struct __elem_head
{
    __page_head_t *_page;
    __slist_t _list;
} __elem_head_t;

struct __mempool_in
{
    size_t _elem_size;
    __dlist_t _pages;
    __slist_t _free_list;
};

mempool_t mp_init(size_t elem_size)
{
    pthread_once(&__mp_g_init_page_size_once, __mp_init_page_size);
    if ( 0 == elem_size )
    {
        return NULL;
    }
    struct __mempool_in *res;
    res = (struct __mempool_in *)calloc(1, sizeof(struct __mempool_in));
    if ( NULL == res )
    {
        return NULL;
    }
    res->_elem_size = elem_size + sizeof(void *); /* extra _page pointer */
    if ( res->_elem_size % sizeof(void *) != 0 ) /* need padding */
    {
        res->_elem_size /= sizeof(void *);
        res->_elem_size += 1;
        res->_elem_size *= sizeof(void *);
    }
    DLIST_INIT(&res->_pages);
    return res;
}

void *mp_alloc(mempool_t mp)
{
    if ( NULL == mp )
    {
        return NULL;
    }
    struct __mempool_in *h = (struct __mempool_in *)mp;
    if ( !SLIST_EMPTY(&h->_free_list) )
    {
        __slist_t *cur = SLIST_NEXT(&h->_free_list);
        SLIST_REMOVE(&h->_free_list, cur);
        ++GET_OWNER(cur, __elem_head_t, _list)->_page->_cur_cnt;
        return cur;
    }

    int num;
    size_t size;
    if ( h->_elem_size > __mp_g_page_size - sizeof(__page_head_t) )
    {
        num = 1;
        size = h->_elem_size + sizeof(__page_head_t);
    }
    else
    {
        num = (__mp_g_page_size - sizeof(__page_head_t)) / h->_elem_size;
        size = __mp_g_page_size;
    }

    void *page = malloc(size);
    if ( NULL == page )
    {
        return NULL;
    }

    __page_head_t *page_head = (__page_head_t *)page;
    DLIST_INIT(&page_head->_list);
    page_head->_cur_cnt = 0;
    page_head->_total_cnt = num;

    DLIST_INSERT_A(&h->_pages, &page_head->_list);

    int i;
    uint8_t *pb = (uint8_t *)(page_head + 1);
    __elem_head_t *pe;
    for ( i = 0; i < num; ++i )
    {
        pe = (__elem_head_t *)(pb + i * h->_elem_size);
        pe->_page = page_head;
        SLIST_INIT(&pe->_list);

        SLIST_INSERT_A(&h->_free_list, &pe->_list);
    }

    return mp_alloc(mp);
}

void mp_free(mempool_t mp, void *ptr)
{
    if ( NULL == ptr )
    {
        return ;
    }
    if ( NULL == mp )
    {
        return ;
    }
    struct __mempool_in *h = (struct __mempool_in *)mp;
    __elem_head_t *pe = GET_OWNER(ptr, __elem_head_t, _list);
    SLIST_INIT(&pe->_list);

    SLIST_INSERT_A(&h->_free_list, &pe->_list);

    __page_head_t *page = pe->_page;
    if ( 0 == --page->_cur_cnt )
    {
        __slist_t *pre = &h->_free_list;
        __slist_t *cur = SLIST_NEXT(pre);
        while (cur)
        {
            if ( GET_OWNER(cur, __elem_head_t, _list)->_page == page )
            {
                SLIST_REMOVE(pre, cur);
            }
            else
            {
                pre = cur;
            }
            cur = SLIST_NEXT(pre);
        }
        DLIST_REMOVE(&page->_list);

        free(page);
    }
}

void mp_renew(mempool_t mp)
{
    if ( NULL == mp )
    {
        return ;
    }
    struct __mempool_in *h = (struct __mempool_in *)mp;
    __dlist_t *next;
    while ( !DLIST_EMPTY(&h->_pages) )
    {
        next = DLIST_NEXT(&h->_pages);
        DLIST_REMOVE(next);
        free(GET_OWNER(next, __page_head_t, _list));
    }
    SLIST_INIT(&h->_free_list);
}

void mp_close(mempool_t mp)
{
    if ( NULL == mp )
    {
        return ;
    }
    mp_renew(mp);
    struct __mempool_in *h = (struct __mempool_in *)mp;
    bzero(h, sizeof(*h));
    free(h);
}
