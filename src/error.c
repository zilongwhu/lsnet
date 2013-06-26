/*
 * =====================================================================================
 *
 *       Filename:  error.c
 *
 *    Description:  thread specific error getter
 *
 *        Version:  1.0
 *        Created:  2012年05月27日 21时19分04秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zilong.Zhu (), zilong.whu@gmail.com
 *        Company:  edu.whu
 *
 * =====================================================================================
 */

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "log.h"
#include "error.h"

static pthread_key_t g_error_key;

static void create_error_key()
{
    int ret = pthread_key_create(&g_error_key, free);
    if ( 0 != ret )
    {
        WARNING("pthread_key_create error[%d].", ret);
        exit(-1);
    }
}

const char *strerror_t(int errnum)
{
    static pthread_once_t s_error_once = PTHREAD_ONCE_INIT;

    pthread_once(&s_error_once, create_error_key);

    char *str = (char *)pthread_getspecific(g_error_key);
    if ( NULL == str )
    {
        str = (char *)malloc(256);
        if ( NULL == str )
        {
            WARNING("failed to alloc mem.");
            return "unknown error";
        }
        int ret = pthread_setspecific(g_error_key, str);
        if ( 0 != ret )
        {
            free(str);
            WARNING("pthread_setspecific error[%d].", ret);
            return "unknown error";
        }
    }
#if (_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && ! _GNU_SOURCE
    if ( strerror_r(errnum, str, 256) < 0 )
#else
    if ( (str = strerror_r(errnum, str, 256)) == NULL )
#endif
    {
        WARNING("strerror_r error[%d].", errno);
        return "unknown error";
    }
    return str;
}
