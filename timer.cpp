/**
 ***All Rights Reserved
 *
 *author frankiezhu
 *date 2013-03-22
*/

#include "log.h"
#include "timer.h"

struct timeval g_cur_tv;

void TimerObj::add_timer(int time_out, TimerList *tl)
{
    m_expected_time = get_cur_time_ms() + time_out;
    tl->add_time_obj(this, m_expected_time);
}

void TimerList::add_time_obj(TimerObj *obj, int64_t expected_time)
{
    list<TimerObj *>::iterator it;
    for (it = m_timer_list.begin(); it != m_timer_list.end(); ++it)
    {
        if ((*it)->get_expected_time() >= expected_time)
        {
            break;
        }
    }
    m_timer_list.insert(it, obj);
}

int TimerList::check_expired()
{
    list<TimerObj *>::iterator it;
    int n = 0;

    for (it = m_timer_list.begin(); it != m_timer_list.end();)
    {
        if ((*it)->get_expected_time() <= get_cur_time_ms())
        {
            (*it)->handle_time_out();
            it = m_timer_list.erase(it);
            ++n;
        }
        else
        {
            ++it;
        }
    }

    return n;
}

