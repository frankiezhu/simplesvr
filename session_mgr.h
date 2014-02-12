/**
 ***All Rights Reserved
 *
 *author frankiezhu
 *date 2013-03-22
*/

#ifndef __SESSION_MGR_HEADER__
#define __SESSION_MGR_HEADER__
#include <map>
#include <unistd.h>
#include "session.h"
#include "timer.h"

using namespace std;

class SessionMgr: public TimerObj
{
public:
    SessionMgr();
    ~SessionMgr();
    Session *get_session(u32 nip,int64_t s_life);
    void add_session(Session *session);
    void init_timer(TimerList *timelist);
    void destroy_all_session();
    int handle_time_out();

private:
    map<u32, Session *> m_session_map;
    TimerList *m_timer_list;
    int m_session_time_out;
    list<Session *> m_timeout_list;
};

#endif
