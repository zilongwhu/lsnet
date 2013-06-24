/*
 * =====================================================================================
 *
 *       Filename:  timer.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2012年03月16日 23时42分08秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zilong.Zhu (), zilong.whu@gmail.com
 *        Company:  edu.whu
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <time.h>
#include "log.h"
#include "timer.h"
#include "utils.h"

mstimer_t *timer_new(void)
{
    mstimer_t *res =  (mstimer_t *)calloc(1, sizeof(mstimer_t));
    if ( NULL == res )
    {
        WARNING("no mem left, calloc ret NULL");
        return NULL;
    }
    struct timeval tv;
    gettimeofday(&tv, NULL);
    res->_last = res->_now = TV2MS(tv);

    int i;
    for ( i = 0; i < 1000; ++i )
    {
        DLIST_INIT(&res->_ms[i]);
    }
    for ( i = 0; i < 60; ++i )
    {
        DLIST_INIT(&res->_sec[i]);
        DLIST_INIT(&res->_min[i]);
    }
    DLIST_INIT(&res->_other);

    return res;
}

void timer_destroy(mstimer_t *tm)
{
    if ( NULL == tm )
    {
        return ;
    }
    __dlist_t *ptr;
    __dlist_t *nxt;
    int i;
    for ( i = 0; i < 1000; ++i )
    {
        ptr = &tm->_ms[i];
        while ( !DLIST_EMPTY(ptr) )
        {
            nxt = DLIST_NEXT(ptr);
            DLIST_REMOVE(nxt);
        }
    }
    for ( i = 0; i < 59; ++i )
    {
        ptr = &tm->_sec[i];
        while ( !DLIST_EMPTY(ptr) )
        {
            nxt = DLIST_NEXT(ptr);
            DLIST_REMOVE(nxt);
        }
        ptr = &tm->_min[i];
        while ( !DLIST_EMPTY(ptr) )
        {
            nxt = DLIST_NEXT(ptr);
            DLIST_REMOVE(nxt);
        }
    }
    ptr = &tm->_other;
    while ( !DLIST_EMPTY(ptr) )
    {
        nxt = DLIST_NEXT(ptr);
        DLIST_REMOVE(nxt);
    }
    free(tm);
}

int timer_add(mstimer_t *tm, struct __timer_unit *unit)
{
    if ( NULL == tm || NULL == unit )
    {
        return -1;
    }
    const uint64_t ms = TV2MS(unit->_tv);
    if ( ms < tm->_last )
    {
        WARNING("unit timeout time[%lu] < timer base time[%lu]", ms, tm->_last);
        return -1;
    }
    ++tm->_count;
    DLIST_INIT(&unit->_list);

    int off;

    const uint64_t sec_base = tm->_last / 1000;
    const uint64_t sec_des = ms / 1000;
    if ( sec_base == sec_des )                  /* 同一秒钟 */
    {
        off = ms % 1000;                        /* [0, 999] */
        DLIST_INSERT_B(&unit->_list, &tm->_ms[off]);
        return 0;
    }
    const uint64_t min_base = sec_base / 60;
    const uint64_t min_des = sec_des / 60;
    if ( min_base == min_des )                  /* 同一分钟 */
    {
        off = sec_des % 60;                     /* [1, 59] */
        DLIST_INSERT_B(&unit->_list, &tm->_sec[off]);
    }
    else if ( min_des - min_base < 60 )         /* 一小时内 */
    {
        off = min_des - min_base;               /* [1, 59] */
        DLIST_INSERT_B(&unit->_list, &tm->_min[off]);
    }
    else
    {
        DLIST_INSERT_B(&unit->_list, &tm->_other);
    }
    return 0;
}

int timer_add2(mstimer_t *tm, struct __timer_unit *unit, int ms)
{
    if ( NULL == unit )
    {
        return -1;
    }
    gettimeofday(&unit->_tv, NULL);
    unit->_tv.tv_sec += ms / 1000;
    unit->_tv.tv_usec += ms % 1000 * 1000;
    if ( unit->_tv.tv_usec >= 1000000 )
    {
        ++unit->_tv.tv_sec;
        unit->_tv.tv_usec -= 1000000;
    }
    return timer_add(tm, unit);
}

int timer_del(mstimer_t *tm, struct __timer_unit *unit)
{
    if ( NULL == unit )
    {
        return -1;
    }
    --tm->_count;
    DLIST_REMOVE(&unit->_list);
    return 0;
}

void timer_timeout(mstimer_t *tm)
{
    if ( NULL == tm )
    {
        return ;
    }
    struct timeval tv;

    gettimeofday(&tv, NULL);
    tm->_now = TV2MS(tv);
}

static void timer_adjust_min(mstimer_t *tm)
{
    if ( tm->_last % 60000 != 0 )
    {
        WARNING("tm->_last[%lu] %% 60000 != 0, need not to adjust on minutes", tm->_last);
        return ;
    }
#ifdef TIMER_CHECK
    {
        int pos;
        for (pos = 0; pos < 60; ++pos)
        {
            if (!DLIST_EMPTY(&tm->_sec[pos]))
            {
                WARNING("tm->_sec[%d] should be empty, error", pos);
                return ;
            }
        }
    }
#endif

    __dlist_t *ptr;
    int pos;
    struct __timer_unit *unit;
    while ( !DLIST_EMPTY(&tm->_min[1]) )
    {
        ptr = DLIST_NEXT(&tm->_min[1]);
        DLIST_REMOVE(ptr);
        unit = GET_OWNER(ptr, struct __timer_unit, _list);
        pos = (TV2MS(unit->_tv) / 1000) % 60;
        DLIST_INSERT_B(ptr, &tm->_sec[pos]);
    }

    int i;
    for ( i = 2; i < 60; ++i )
    {
        DLIST_INSERT_A(&tm->_min[i], &tm->_min[i - 1]);
        DLIST_REMOVE(&tm->_min[i]);
    }

    __dlist_t *next;
    for ( ptr = DLIST_NEXT(&tm->_other); ptr != &tm->_other; ptr = next )
    {
        next = DLIST_NEXT(ptr);
        unit = GET_OWNER(ptr, struct __timer_unit, _list);
        if ( tm->_last / 60000 + 59 >= TV2MS(unit->_tv) / 60000 )
        {
#ifdef TIMER_CHECK
            if ( tm->_last / 60000 + 59 != TV2MS(unit->_tv) / 60000 )
            {
                WARNING("timer panic error");
            }
#endif
            DLIST_REMOVE(ptr);
            DLIST_INSERT_B(ptr, &tm->_min[59]);
        }
    }
}

static void timer_adjust_sec(mstimer_t *tm)
{
    if ( tm->_last % 1000 != 0 )
    {
        WARNING("tm->_last[%lu] %% 1000 != 0, need not to adjust on seconds", tm->_last);
        return ;
    }
#ifdef TIMER_CHECK
    {
        int pos;
        for (pos = 0; pos < 1000; ++pos)
        {
            if (!DLIST_EMPTY(&tm->_ms[pos]))
            {
                WARNING("tm->_ms[%d] should be empty, error", pos);
                return ;
            }
        }
    }
#endif
    /* 步入下一秒 */
    int off = (tm->_last / 1000) % 60;
    if ( 0 == off )
    {
        timer_adjust_min(tm);
    }
    while ( off < 60 )
    {
        if  ( DLIST_EMPTY(&tm->_sec[off]) )
        {
            if ( tm->_last + 1000 > tm->_now )
            {
                tm->_last = tm->_now;
                return ;
            }
            tm->_last += 1000;
            ++off;
        }
        else
        {
            __dlist_t *ptr;
            int pos;
            struct __timer_unit *unit;
            while ( !DLIST_EMPTY(&tm->_sec[off]) )
            {
                ptr = DLIST_NEXT(&tm->_sec[off]);
                DLIST_REMOVE(ptr);
                unit = GET_OWNER(ptr, struct __timer_unit, _list);
                pos = TV2MS(unit->_tv) % 1000;
                DLIST_INSERT_B(ptr, &tm->_ms[pos]);
            }
            return ;
        }
    }
}

static struct __timer_unit *timer_next_int(mstimer_t *tm)
{
    int off;
    for ( off = tm->_last % 1000; off < 1000; )
    {
        if ( tm->_last >= tm->_now )    /* 尚未超时 */
        {
            return NULL;
        }
        if  ( DLIST_EMPTY(&tm->_ms[off]) )
        {
            ++tm->_last;
            ++off;
        }
        else
        {
            __dlist_t *ptr = DLIST_NEXT(&tm->_ms[off]);
            DLIST_REMOVE(ptr);
            return GET_OWNER(ptr, struct __timer_unit, _list);
        }
    }
    /* 跨秒 */
    timer_adjust_sec(tm);
    return timer_next_int(tm);
}

struct __timer_unit *timer_next(mstimer_t *tm)
{
    struct __timer_unit *res = timer_next_int(tm);
    if ( NULL != res )
    {
        --tm->_count;
    }
    return res;
}
