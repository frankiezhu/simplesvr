/**
 ***All Rights Reserved
 *
 *author frankiezhu
 *date 2013-03-22
*/

#ifndef __TIMER_HEADER__
#define __TIMER_HEADER__

#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <string>
#include <list>

#define TIME_STR_LEN 64

using namespace std;


extern struct timeval g_cur_tv;

inline void get_string_time(time_t time, string &str_time)
{
    struct tm t;
    char buf[TIME_STR_LEN];

    localtime_r(&time, &t);
    snprintf(buf, TIME_STR_LEN, "%d-%02d-%02d_%02d:%02d:%02d",
             t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
    str_time.assign(buf);
}

inline void do_update_tv(void)
{
    gettimeofday(&g_cur_tv, NULL);
}

inline int64_t get_cur_time_ms(void)
{
    return (int64_t)g_cur_tv.tv_sec * 1000 + g_cur_tv.tv_usec / 1000;
}

inline time_t get_cur_time_s(void)
{
    return g_cur_tv.tv_sec;
}

class TimerList;

class TimerObj
{
public:
    TimerObj() {}
    ~TimerObj() {}
    virtual int handle_time_out()
    {
        return 0;
    }

    void add_timer(int time_out, TimerList *timer);
    int64_t get_expected_time()
    {
        return m_expected_time;
    }

private:
    int64_t m_expected_time;
};


class TimerList
{
public:
    TimerList() {}
    ~TimerList() {}
    int check_expired();
    void add_time_obj(TimerObj *obj, int64_t expected_time);
    friend class TimerObj;
private:
    list<class TimerObj *> m_timer_list;
};

#endif
