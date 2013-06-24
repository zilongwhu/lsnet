/*
 * =====================================================================================
 *
 *       Filename:  log.h
 *
 *    Description:  log
 *
 *        Version:  1.0
 *        Created:  2012年05月23日 18时41分01秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zilong.Zhu (), zilong.whu@gmail.com
 *        Company:  edu.whu
 *
 * =====================================================================================
 */

#ifndef __LOG_H__
#define __LOG_H__

enum
{
    LOG_LEVAL_DEBUG,
    LOG_LEVAL_TRACE,
    LOG_LEVAL_NOTICE,
    LOG_LEVAL_WARNING,
    LOG_LEVAL_FATAL,
};

void err_warn(int level, const char *format, ...);

#define LOG_HELPER(level, format, args...) do {                           \
    err_warn(level, "[%s:%s:%d] " format,                                 \
            __FILE__, __FUNCTION__, __LINE__, ##args);                    \
} while(0)

//#define DEBUG(format, args...)
#define DEBUG(format, args...)        LOG_HELPER(LOG_LEVAL_DEBUG, format, ##args)
#define TRACE(format, args...)        LOG_HELPER(LOG_LEVAL_TRACE, format, ##args)
#define NOTICE(format, args...)        LOG_HELPER(LOG_LEVAL_NOTICE, format, ##args)
#define WARNING(format, args...)    LOG_HELPER(LOG_LEVAL_WARNING, format, ##args)
#define FATAL(format, args...)        LOG_HELPER(LOG_LEVAL_FATAL, format, ##args)

#endif
