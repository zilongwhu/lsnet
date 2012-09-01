/*
 * =====================================================================================
 *
 *       Filename:  timer.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2012年03月16日 23时08分24秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zilong.Zhu (), zilong.whu@gmail.com
 *        Company:  edu.whu
 *
 * =====================================================================================
 */
#ifndef __TIMER_H__
#define __TIMER_H__

#include <stdint.h>
#include <sys/time.h>
#include "dlist.h"

#ifndef NULL
#define NULL 0
#endif

#define TV2MS(tv) (((uint64_t)(tv.tv_sec)) * 1000 + tv.tv_usec / 1000)

struct __timer_unit
{
	__dlist_t _list;
	struct timeval _tv;
	uintptr_t _user_value;
};

typedef struct __timer_in
{
	uint64_t _last;
	uint64_t _now;
	uint32_t _count;

	__dlist_t _ms[1000];
	__dlist_t _sec[60];
	__dlist_t _min[60];
	__dlist_t _other;
} mstimer_t;

mstimer_t *timer_new(void);
void timer_destroy(mstimer_t *tm);

int timer_add(mstimer_t *tm, struct __timer_unit *unit);
int timer_add2(mstimer_t *tm, struct __timer_unit *unit, int ms);
int timer_del(mstimer_t *tm, struct __timer_unit *unit);

void timer_timeout(mstimer_t *tm);
struct __timer_unit *timer_next(mstimer_t *tm);

#endif
