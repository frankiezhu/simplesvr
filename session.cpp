/**
 ***All Rights Reserved
 *
 *author frankiezhu
 *date 2013-03-22
*/

#include "log.h"
#include "session.h"
#include "network.h"
#include "timer.h"

Session::Session(int64_t life_span): m_sockfd(-1),
    m_cli_addr(0), m_cli_port(0),m_life_span(life_span)
{
    m_msg_len = 0;
    m_buff = NULL;
    m_has_head = false;
    m_n_read = 0;
}

Session::~Session()
{
    reset_buffer();
    if (m_sockfd > 0)
    {
        close(m_sockfd);
    }
}

void Session::reset_buffer()
{
    m_has_head = false;
    m_n_read = 0;
    m_msg_len = 0;
    if (m_buff)
    {
        glb_mempool()->put(m_buff);
        m_buff = NULL;
    }
}


int Session::on_connect()
{
    logdbg("session connect:%s, pid:%d\n",
           get_string_ip().c_str(), g_pid);
    m_start_time = get_cur_time_ms();
    m_active_time = m_start_time;
    return 0;
}

int Session::write_msg(u_char *buffer, int buf_len)
{
    char file_name[FILE_NAME_LEN];
    string str_time;
    get_string_time(get_cur_time_s(), str_time);
    string ip = get_string_ip();
    snprintf(file_name, FILE_NAME_LEN, "%s/%s/%s_%s.tmp",
             FILE_MSG_DIR, g_msg_file_name[m_msg_type],
             ip.c_str(), str_time.c_str());
    FILE *filp = fopen(file_name, "a+");
    if (!filp)
    {
        logerr("open file %s failed!\n", file_name);
        return -1;
    }
    fwrite(buffer, buf_len, 1, filp);
    fclose(filp);

    char new_file_name[FILE_NAME_LEN];
    snprintf(new_file_name, FILE_NAME_LEN, "%s/%s/%s_%s.txt",
             FILE_MSG_DIR, g_msg_file_name[m_msg_type],
             ip.c_str(), str_time.c_str());
    int ret = rename(file_name, new_file_name);
    if (ret < 0)
    {
        logerr("rename failed!%s->%s\n", file_name, new_file_name);
        return -1;
    }
    return 0;
}

int Session::read_head()
{
    struct msg_header tmp;
    int n_read = readn(m_sockfd, (u_char*)&tmp, sizeof(struct msg_header));
    if (n_read == 0)
    {
        return SESSION_RET_NO_DATA;
    }

    if (n_read < (int)sizeof(struct msg_header))
    {
        return -1;
    }

    if (tmp.version != 0x02 || tmp.msg_type > MSG_MAX || tmp.length > 16 * 1024)
    {
        logerr("invalid header, version:%u, msg_type:%u, length:%d\n",
               tmp.version, tmp.msg_type, tmp.length);
        return -1;
    }

    logdbg("ip:%s msg type:%d, len:%d, pid:%d\n",
           get_string_ip().c_str(), tmp.msg_type, tmp.length, g_pid);
    m_msg_type = (msg_type_t)tmp.msg_type;
    m_msg_len = tmp.length;
    return 0;
}

int Session::notify_master(struct commu_msg &msg)
{
    int n = writen(g_fifo_fd, (u_char *)&msg, sizeof(msg));
    if (n != sizeof(msg))
    {
        logerr("write fifo failed!");
        return -1;
    }
    logdbg("slave notify master\n");
    return 0;
}

int Session::on_read_event()
{
    int ret;
    while (true)
    {
        if (!m_has_head)
        {
            ret = read_head();
            if (ret < 0)
            {
                return -1;
            }
            if (ret == SESSION_RET_NO_DATA)
            {
                return 0;
            }
            if (m_msg_type == MSG_HEART_BEAT)
            {
                if (g_is_master)
                {
                    update_active_time();
                }
                else
                {
                    struct commu_msg msg;
                    msg.ip = get_cli_addr();
                    msg.msg = MSG_HEART_BEAT;
                    notify_master(msg);
                }
                continue;
            }
            m_has_head = true;
            m_buff = (u_char *)glb_mempool()->get(m_msg_len);
        }
        int n_read = readn(m_sockfd, &m_buff[m_n_read], m_msg_len - m_n_read);
        if (n_read < 0)
        {
            logerr("socket broken\n");
            m_msg_len = 0;
            m_has_head = false;
            return -1;
        }
        m_n_read += n_read;
        if (m_n_read == m_msg_len)
        {
            write_msg(m_buff, m_msg_len);
            reset_buffer();
        }
    }
    return 0;

}

int Session::on_write_event()
{
    return 0;
}

int Session::on_close()
{
    logdbg("session close:%s, pid:%d\n",
           get_string_ip().c_str(), g_pid);
    reset_buffer();
    close(m_sockfd);
    m_sockfd = -1;

    return 0;
}

