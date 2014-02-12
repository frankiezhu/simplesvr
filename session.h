/**
 ***All Rights Reserved
 *
 *author frankiezhu
 *date 2013-03-22
*/

#ifndef __SESSION_HEADER__
#define __SESSION_HEADER__
#include "mempool.h"
#include "message.h"
#include <time.h>
#include <unistd.h>
#include "timer.h"
#include <string>

using namespace std;

typedef enum session_ret
{
    SESSION_RET_FAILED = -1,
    SESSION_RET_SUCCESS = 0,
    SESSION_RET_NO_DATA = 1,
    SESSION_RET_NOTIFY_MASTER = 2,
    SESSION_RET_MAX,
} session_ret_t;

class Session
{
public:
    Session(int64_t life_span);
    virtual ~Session();

    virtual int on_connect();
    virtual int on_read_event();
    virtual int on_write_event();
    virtual int on_close();

    u32 get_cli_addr() const
    {
        return m_cli_addr;
    }

    void set_cli_addr(u32 ip)
    {
        m_cli_addr = ip;
        struct in_addr addr;
        addr.s_addr = htonl(m_cli_addr);
        m_string_ip = string(inet_ntoa(addr));
    }

    int get_cli_port() const
    {
        return m_cli_port;
    }

    void set_cli_port(u32 port)
    {
        m_cli_port = port;
    }

    int get_sockfd() const
    {
        return m_sockfd;
    }

    void set_sockfd(int sockfd)
    {
        m_sockfd = sockfd;
    }

    int64_t get_active_time() const
    {
        return m_active_time;
    }

    void update_active_time()
    {
        m_active_time = get_cur_time_ms();
    }

    string& get_string_ip()
    {
        return m_string_ip;
    }

    int64_t get_life_span() const
    {
        return m_life_span;
    }

    void set_life_span(int64_t lifetime)
    {
        m_life_span = lifetime;
    }

    bool is_expired()
    {
        return ((get_cur_time_ms() - m_active_time) > (m_life_span*1000)); 
    }

protected:
    int write_msg(u_char *buffer, int buf_len);
    int read_head();
    int notify_master(struct commu_msg &msg);
    void reset_buffer();

protected:
    int m_sockfd;
    u32 m_cli_addr;
    u32 m_cli_port;
    string m_string_ip;
    int64_t m_start_time;
    int64_t m_active_time;
    u_char *m_buff;
    int m_n_read;
    int m_msg_len;
    bool m_has_head;
    msg_type_t m_msg_type;
    machine_type_t m_machine_type;
    int64_t m_life_span;
};

#endif
