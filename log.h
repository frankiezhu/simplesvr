/**
 ***All Rights Reserved
 *
 *author frankiezhu
 *date 2013-03-22
*/

#ifndef __LOG_HEADER__
#define __LOG_HEADER__

#include<stdio.h>
#include<syslog.h>
#include<string.h>
#include<errno.h>

#define ERROR_RETURN(op) \
    do { if ((op) < 0) return(-__LINE__); } while(0)

#define NULL_RETURN(op) \
    do { if ((op) == NULL) return(-__LINE__); } while(0)

#define LOG_ENABLE

#ifdef  LOG_ENABLE
#define logerr(fmt, args...)   do { syslog(LOG_ERR, "logerr %s,%d,%d:" fmt, __FILE__,__LINE__,errno, ##args);\
                                    fprintf(stderr, "logerr %s,%d,%d:" fmt, __FILE__,__LINE__,errno, ##args);\
                                  }while (0)
#define logdbg(fmt, args...)   fprintf(stdout, "logdbg %s,%d:" fmt, __FILE__,__LINE__, ##args)
#else
#define logerr(fmt, args...)
#define logdbg(fmt, args...)
#endif

#endif
