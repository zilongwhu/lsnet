/*
 * =====================================================================================
 *
 *       Filename:  log.c
 *
 *    Description:  log impl
 *
 *        Version:  1.0
 *        Created:  2012年05月23日 18时50分02秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zilong.Zhu (), zilong.whu@gmail.com
 *        Company:  edu.whu
 *
 * =====================================================================================
 */

#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "log.h"

static const char *const s_level_name[] =
{
    "[DEBUG] ",
    "[TRACE] ",
    "[NOTICE] ",
    "[WARNING] ",
    "[FATAL] ",
};

static log_conf_t g_log_conf = { "./lsnet", 4096 };
static struct tm g_last_cut_time;

int init_log(const log_conf_t *conf)
{
    if (NULL == conf || conf->_max_log_length < 33)
    {
        return -1;
    }
    time_t now = time(NULL);
    if (NULL == localtime_r(&now, &g_last_cut_time))
    {
        return -1;
    }
    g_log_conf = *conf;

    NOTICE("init log ok");
    WARNING("init log ok");

    return 0;
}

struct __log_sp
{
    char *_buffer;
    time_t _last_tm;
    pthread_t _tid;
    int _prefix_len;
};

static struct __log_sp *create_log_sp()
{
    struct __log_sp *st = (struct __log_sp *)malloc(sizeof(struct __log_sp));
    if (NULL == st)
    {
        fprintf(stderr, "failed to alloc mem for __log_sp.");
        return NULL;
    }
    st->_buffer = (char *)malloc(g_log_conf._max_log_length);
    if (NULL == st->_buffer)
    {
        free(st);
        fprintf(stderr, "failed to alloc mem for __log_sp._buffer[%u].", g_log_conf._max_log_length);
        return NULL;
    }
    st->_last_tm = 0;
    st->_tid = pthread_self();
    st->_prefix_len = 0;
    return st;
}

static void free_log_sp(void *tmp)
{
    struct __log_sp *st = (struct __log_sp *)tmp;
    if (st)
    {
        if (st->_buffer)
        {
            free(st->_buffer);
        }
        free(st);
    }
}

static pthread_key_t g_log_key;

static void create_log_key()
{
    int ret = pthread_key_create(&g_log_key, free_log_sp);
    if (0 != ret)
    {
        fprintf(stderr, "pthread_key_create error[%d].", ret);
        exit(-1);
    }
}

void err_warn(int level, const char *format, ...)
{
    static pthread_once_t s_log_once = PTHREAD_ONCE_INIT;
    static pthread_mutex_t s_log_mutex = PTHREAD_MUTEX_INITIALIZER;
    static char s_pathfile_1[4096];
    static char s_pathfile_2[4096];
    static int s_log_fd = -1;
    static int s_log_warn_fd = -1;

    if ( level > LOG_LEVEL_FATAL || level < LOG_LEVEL_DEBUG )
    {
        return ;
    }
    pthread_once(&s_log_once, create_log_key);
    struct __log_sp *st = (struct __log_sp *)pthread_getspecific(g_log_key);
    if ( NULL == st )
    {
        st = create_log_sp();
        if ( NULL == st )
        {
            fprintf(stderr, "failed to create log_sp_t.");
            return ;
        }
        int ret = pthread_setspecific(g_log_key, st);
        if ( 0 != ret )
        {
            free_log_sp(st);
            fprintf(stderr, "pthread_setspecific error[%d].", ret);
            return ;
        }
    }
    time_t now = time(NULL);
    if ( now > st->_last_tm )
    {
        st->_last_tm = now;
        struct tm result;
        if (localtime_r(&now, &result))
        {
            strftime(st->_buffer, g_log_conf._max_log_length, "[%Y-%m-%d %H:%M:%S] ", &result);
        }
        else
        {
            strcpy(st->_buffer, "[0000-00-00 00:00:00] "); /* strlen=22 */
        }
        st->_buffer[22] = '\0';
        snprintf(st->_buffer + 22, g_log_conf._max_log_length - 22, "[%lu]", (unsigned long)st->_tid);
        st->_prefix_len = strlen(st->_buffer);

        if (s_log_fd < 0 || /* init fds */
                (g_last_cut_time.tm_year != result.tm_year
                 || g_last_cut_time.tm_mon != result.tm_mon
                 || g_last_cut_time.tm_mday != result.tm_mday
                 || g_last_cut_time.tm_hour != result.tm_hour) /* cut by hour */
                )
        {
            pthread_mutex_lock(&s_log_mutex);
            if (s_log_fd < 0 ||
                    (g_last_cut_time.tm_year != result.tm_year
                     || g_last_cut_time.tm_mon != result.tm_mon
                     || g_last_cut_time.tm_mday != result.tm_mday
                     || g_last_cut_time.tm_hour != result.tm_hour)
                    ) /* check again */
            {
                if (s_log_fd < 0) /* init */
                {
                    g_last_cut_time = result;
                }
                else /* cut by hour */
                {
                    snprintf(s_pathfile_1, sizeof s_pathfile_1, "%s.log", g_log_conf._path_prefix);
                    snprintf(s_pathfile_2, sizeof s_pathfile_2,
                            "%s.%04d_%02d_%02d.%02d",
                            s_pathfile_1,
                            g_last_cut_time.tm_year + 1900,
                            g_last_cut_time.tm_mon + 1,
                            g_last_cut_time.tm_mday,
                            g_last_cut_time.tm_hour);
                    rename(s_pathfile_1, s_pathfile_2);

                    snprintf(s_pathfile_1, sizeof s_pathfile_1, "%s.log.wf", g_log_conf._path_prefix);
                    snprintf(s_pathfile_2, sizeof s_pathfile_2,
                            "%s.%04d_%02d_%02d.%02d",
                            s_pathfile_1,
                            g_last_cut_time.tm_year + 1900,
                            g_last_cut_time.tm_mon + 1,
                            g_last_cut_time.tm_mday,
                            g_last_cut_time.tm_hour);
                    rename(s_pathfile_1, s_pathfile_2);

                    g_last_cut_time = result;
                }

                if (s_log_fd >= 0)
                {
                    close(s_log_fd);
                    s_log_fd = -1;
                }
                snprintf(s_pathfile_1, sizeof s_pathfile_1, "%s.log", g_log_conf._path_prefix);
                s_log_fd = open(s_pathfile_1, O_APPEND | O_WRONLY | O_CREAT, 0640);
                if (s_log_fd < 0)
                {
                    fprintf(stderr, "failed to open log file[%s] error[%d].", s_pathfile_1, errno);
                    exit(-1);
                }

                if (s_log_warn_fd >= 0)
                {
                    close(s_log_warn_fd);
                    s_log_warn_fd = -1;
                }
                snprintf(s_pathfile_1, sizeof s_pathfile_1, "%s.log.wf", g_log_conf._path_prefix);
                s_log_warn_fd = open(s_pathfile_1, O_APPEND | O_WRONLY | O_CREAT, 0640);
                if (s_log_warn_fd < 0)
                {
                    fprintf(stderr, "failed to open log file[%s] error[%d].", s_pathfile_1, errno);
                    exit(-1);
                }
            }
            pthread_mutex_unlock(&s_log_mutex);
        }
    }
    va_list vl;

    strcpy(st->_buffer + st->_prefix_len, s_level_name[level]);
    int len = strlen(st->_buffer);

    va_start(vl, format);
    int ret = vsnprintf(st->_buffer + len, g_log_conf._max_log_length - len, format, vl);
    va_end(vl);

    if ( ret < 0 )
    {
        ret = 0;
    }
    if ( ret >= g_log_conf._max_log_length - len )
    {
        ret = g_log_conf._max_log_length - len - 1;
    }
    st->_buffer[len + ret] = '\n';

    if ( level < LOG_LEVEL_WARNING )
    {
        write(s_log_fd, st->_buffer, len + ret + 1);
    }
    else
    {
        write(s_log_warn_fd, st->_buffer, len + ret + 1);
    }
}
