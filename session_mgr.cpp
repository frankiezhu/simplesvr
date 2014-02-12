/**
 ***All Rights Reserved
 *
 *author frankiezhu
 *date 2013-03-22
*/

#include "log.h"
#include "session_mgr.h"
#include "config.h"
#include <time.h>
#include <string>

using namespace std;

SessionMgr::SessionMgr()
{
    m_timer_list = NULL;
}

SessionMgr::~SessionMgr()
{
    destroy_all_session();
}

Session *SessionMgr::get_session(u32 nip,int64_t s_life)
{
    map<u32, Session *>::iterator it;
    it = m_session_map.find(nip);
    if (it != m_session_map.end())
    {
        return it->second;
    }
    else
    {
        Session *session = new(std::nothrow) Session(s_life);
        if (!session)
        {
            logerr("New sesion failed!\n");
            return NULL;
        }
        session->set_cli_addr(nip);
        add_session(session);
        return session;
    }
}

void SessionMgr::init_timer(TimerList *time_list)
{
    m_timer_list = time_list;
    m_session_time_out = glb_config()->get_keep_alive() * 1000;
    add_timer(m_session_time_out, m_timer_list);
}

void SessionMgr::add_session(Session *session)
{
    m_session_map.insert(pair<u32, Session *>(session->get_cli_addr(), session));
}

void SessionMgr::destroy_all_session()
{
    map<u32, Session *>::iterator it;
    for (it = m_session_map.begin(); it != m_session_map.end(); ++it)
    {
        delete it->second;
    }
    m_session_map.clear();
    return;
}

int SessionMgr::handle_time_out()
{
    add_timer(m_session_time_out, m_timer_list);
    map<u32, Session *>::iterator it;
    int64_t expired = get_cur_time_ms() -
                      m_session_time_out * glb_config()->get_asure_count();
    m_timeout_list.clear();

    for (it = m_session_map.begin(); it != m_session_map.end();)
    {
        Session *session = it->second;
        if (session->get_active_time() < expired)
        {
            logdbg("session %s timeout, expect:%ld current:%ld\n",
                   session->get_string_ip().c_str(), expired, session->get_active_time());
            if (session->is_expired())
            {
                 logdbg("session %s expired, active:%ld current:%ld lifespan:%ld\n",
                   session->get_string_ip().c_str(),session->get_active_time(),
                   get_cur_time_ms(),session->get_life_span());
                 delete session;
                 m_session_map.erase(it++);
            }
            else
            {
                m_timeout_list.push_back(session);
                ++it;
            }
        }
        else
        {
            ++it;
        }
    }

    if (!m_timeout_list.empty())
    {
        char file_name[FILE_NAME_LEN];
        string str_time;
        get_string_time(get_cur_time_s(), str_time);
        snprintf(file_name, FILE_NAME_LEN, "%s/%s/dead_%s.tmp",
                 FILE_MSG_DIR, g_msg_file_name[MSG_HEART_BEAT],
                 str_time.c_str());
        FILE *filp = fopen(file_name, "a+");
        if (!filp)
        {
            logerr("open file %s failed!\n", file_name);
            return -1;
        }

        while (!m_timeout_list.empty())
        {
            Session *session = m_timeout_list.back();
            m_timeout_list.pop_back();
            fprintf(filp, "%s\n", session->get_string_ip().c_str());
        }

        fclose(filp);

        char new_file_name[FILE_NAME_LEN];
        snprintf(new_file_name, FILE_NAME_LEN, "%s/%s/dead_%s",
                 FILE_MSG_DIR, g_msg_file_name[MSG_HEART_BEAT],
                 str_time.c_str());
        int ret = rename(file_name, new_file_name);
        if (ret < 0)
        {
            logerr("rename file failed!%s->%s\n", file_name, new_file_name);
            return -1;
        }
    }
    return 0;
}



