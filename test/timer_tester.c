/*
 * =====================================================================================
 *
 *       Filename:  timer_tester.c
 *
 *    Description:  timer tester
 *
 *        Version:  1.0
 *        Created:  2012年05月21日 23时58分17秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zilong.Zhu (), zilong.whu@gmail.com
 *        Company:  edu.whu
 *
 * =====================================================================================
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "timer.h"

int main(int argc, char *argv[])
{
	mstimer_t *timer = timer_new();
	int i, j, f;
	int cnt = 0;
	struct __timer_unit *unit;
	for ( i = 0; i < 2; ++i )
	{
		for ( j = 0; j < 30000000; ++j )
		{
			unit = (struct __timer_unit *)malloc(sizeof(struct __timer_unit));
			if ( NULL == unit )
			{
				break;
			}
			if ( timer_add2(timer, unit, rand() % 10000000) < 0 )
			{
				free(unit);
			}
			else
			{
				++cnt;
			}
		}
		printf("count: %d\n", cnt);
		printf("timer count: %u\n", timer->_count);
		while ( timer->_count > 0 )
		{
			usleep(10);
			if ( 0 == i && timer->_count < 10000 )
			{
				printf("retry: %u\n", timer->_count);
				break;
			}
			timer_timeout(timer);
			f = 1;
			while ( NULL != (unit = timer_next(timer)) )
			{
				if ( f )
				{
					printf("--------------------------------------------------------------------------------\n");
					printf("now: %lu, count: %u\n", timer->_now, timer->_count + 1);
					f = 0;
				}
				printf("#    %lu\n", (uint64_t)TV2MS(unit->_tv));
				free(unit);
			}
		}
	}
	timer_destroy(timer);
	return 0;
}
