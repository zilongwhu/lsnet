/*
 * =====================================================================================
 *
 *       Filename:  utils.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2012年03月06日 23时25分53秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zilong.Zhu (), zilong.whu@gmail.com
 *        Company:  edu.whu
 *
 * =====================================================================================
 */
#ifndef __UTILS_H__
#define __UTILS_H__

#define SAFE_CLOSE(fd)    do { } while ( fd >= 0 && close(fd) < 0 && EINTR == errno )

#define GET_OWNER(ptr, type, mem) ((type *)(((char *)(ptr)) - (int)&((type *)(0xF0))->mem) + 0xF0)

int set_nonblock(int fd);

#endif

