/*
 * =====================================================================================
 *
 *       Filename:  utils.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2012年05月23日 13时44分08秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zilong.Zhu (), zilong.whu@gmail.com
 *        Company:  edu.whu
 *
 * =====================================================================================
 */

#include <unistd.h>
#include <fcntl.h>
#include "utils.h"

int set_nonblock(int fd)
{
    if ( fd < 0 )
    {
        return -1;
    }
    int flags = fcntl(fd, F_GETFL);
    if ( flags < 0 )
    {
        return -1;
    }
    flags |= O_NONBLOCK;
    if ( fcntl(fd, F_SETFL, flags) < 0 )
    {
        return -1;
    }
    return 0;
}
