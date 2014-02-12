/**
 ***All Rights Reserved
 *
 *author frankiezhu
 *date 2013-03-22
*/

#ifndef __SERVER_HEADER__
#define  __SERVER_HEADER__

#include <sys/resource.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "session_mgr.h"
#include "timer.h"

class Server
{
public:
    Server(int port,int64_t session_life);
    virtual ~Server();

    int run();
    int init_server();
    void destroy_server();

    void set_worker_num(int num)
    {
        m_worker_num = num;
    }

protected:
    virtual int on_connect(int sockfd, struct sockaddr_in *client_addr);
    virtual int on_read(Session *session);
    virtual int on_write(Session *session);
    virtual int on_close(Session *session);

private:
    int init_listen();
    int create_mem_pools();
    int fork_childs();
    int notify_master(struct commu_msg &msg);
    int on_slave_notify();
    int set_sock_timeout(int sockfd);

    int epoll_add(int fd, int event, void *ptr)
    {
        struct epoll_event ev;
        ev.data.ptr = ptr;
        ev.events = event;
        return epoll_ctl(m_ep_fd, EPOLL_CTL_ADD, fd, &ev);
    }

    int epoll_del(int fd, int event)
    {
        struct epoll_event ev;
        return epoll_ctl(m_ep_fd, EPOLL_CTL_DEL, fd, &ev);
    }

    int epoll_mod(int fd, int event, void *ptr)
    {
        struct epoll_event ev;
        ev.data.ptr = ptr;
        ev.events = event;
        return epoll_ctl(m_ep_fd, EPOLL_CTL_MOD, fd, &ev);
    }

private:
    struct epoll_event *m_ep_events;
    SessionMgr *m_session_mgr;
    Session *m_fifo_session;
    TimerList *m_timer_list;
    int m_ep_fd;
    int m_ep_max_poll;
    int m_listen_fd;
    int m_listen_port;
    int m_worker_num;
    int64_t m_session_life;
};

#endif
