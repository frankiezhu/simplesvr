#include "server.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include "mempool.h"
#include "network.h"
#include "config.h"


const static int DEFAULT_BACKLOG = 500;
const char *fifo_path = "/tmp/simplesvr.fifo";

Server::Server(int port,int64_t session_life):
 m_listen_port(port), m_worker_num(1),m_session_life(session_life)
{
    m_session_mgr = NULL;
    m_timer_list = NULL;
    m_ep_events = NULL;
    m_ep_max_poll = 500;
}

Server::~Server()
{
    destroy_server();
}

int setnonblocking(int fd)
{
    int flags;
    flags = fcntl(fd, F_GETFL);

    if (flags < 0)
    {
        return -1;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        return -1;
    }
    return 0;
}


int Server::init_listen()
{
    struct sockaddr_in server_addr;
    int ret;

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(m_listen_port);

    m_listen_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (m_listen_fd < 0)
    {
        logerr("create socket fail!");
        return -1;
    }

    ret = ::setnonblocking(m_listen_fd);
    if (ret < 0)
    {
        return -1;
    }

    int opt = 1;
    setsockopt(m_listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(m_listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)))
    {
        logerr("server bind Port [%d] fail\n", m_listen_port);
        return -1;
    }

    if (listen(m_listen_fd, DEFAULT_BACKLOG))
    {
        logerr("server listen fail!");
        return -1;
    }

    return 0;
}

int Server::fork_childs()
{
    int ret = mkfifo(fifo_path, 0666);
    if (ret < 0 && errno != EEXIST)
    {
        logerr("create fifo[%s] failed!\n", fifo_path);
        return -1;
    }
    g_fifo_fd = open(fifo_path, O_RDONLY | O_NONBLOCK, 0666);
    if (g_fifo_fd < 0)
    {
        logerr("master open fifo %s failed!\n", fifo_path);
        return -1;
    }
    g_pid = getpid();
    logdbg("master pid:%d\n", g_pid);
    for (int i = 0; i < m_worker_num - 1; i++)
    {
        pid_t pid = fork();
        if (pid < 0)
        {
            logerr("fork child failed!\n");
            return -1;
        }
        else if (pid > 0)
        {
            continue;
        }
        else
        {
            g_is_master = false;
            g_pid = getpid();
            close(g_fifo_fd);
            g_fifo_fd = open(fifo_path, O_WRONLY | O_NONBLOCK, 0666);
            if (g_fifo_fd < 0)
            {
                logerr("child open fifo %s failed!\n", fifo_path);
                return -1;
            }
            return 0;
        }
    }

    return 0;
}

int Server::init_server()
{
    int ret;
    ret = ::glb_mempool()->init();
    if (ret < 0)
    {
        logerr("init mem pool mgr failed!\n");
        return -1;
    }

    m_timer_list = new(std::nothrow) TimerList();
    if (!m_timer_list)
    {
        logerr("init timer list failed!\n");
        return -1;
    }

    m_session_mgr = new(std::nothrow) SessionMgr();
    if (!m_session_mgr)
    {
        logerr("init session mgr failed!\n");
        return -1;
    }

    ret = init_listen();
    if (ret < 0)
    {
        logerr("init listen port failed!\n");
        return -1;
    }

    ret = fork_childs();
    if (ret < 0)
    {
        logerr("fork childs failed!\n");
        return -1;
    }

    m_ep_fd = epoll_create(m_ep_max_poll);
    m_ep_events = (struct epoll_event *)calloc(m_ep_max_poll, sizeof(struct epoll_event));
    if (!m_ep_events)
    {
        logerr("alloc events failed!\n");
        return -1;
    }

    if (g_is_master)
    {
        do_update_tv();
        m_session_mgr->init_timer(m_timer_list);
        ret = epoll_add(g_fifo_fd, EPOLLIN, &g_fifo_fd);
        if (ret < 0)
        {
            logerr("add fifo fd to epoll:%d %d\n", ret, errno);
            return -1;
        }
    }
    return 0;
}


void Server::destroy_server()
{
    if (m_listen_fd >= 0)
    {
        close(m_listen_fd);
    }
    if (m_ep_events)
    {
        free(m_ep_events);
    }
    if (m_session_mgr)
    {
        delete m_session_mgr;
    }
    if (m_timer_list)
    {
        delete m_timer_list;
    }
    close(m_ep_fd);
    unlink(fifo_path);
    return;
}

int Server::on_connect(int sockfd, struct sockaddr_in *client_addr)
{
    int addr = htonl(client_addr->sin_addr.s_addr);
    int port = htons(client_addr->sin_port);
    ::setnonblocking(sockfd);

    Session *session = m_session_mgr->get_session(addr,this->m_session_life);
    if (!session)
    {
        close(sockfd);
        return -1;
    }
    session->set_cli_addr(addr);
    session->set_cli_port(port);
    session->set_sockfd(sockfd);
    session->on_connect();
    epoll_add(sockfd, EPOLLIN, session);
    return 0;
}

int Server::on_slave_notify()
{
    struct commu_msg msg;

    while (true)
    {
        int n = readn(g_fifo_fd, (u_char *)&msg, sizeof(msg));
        if (n == 0)
        {
            break;
        }
        else if (n < 0 || n != sizeof(msg))
        {
            logerr("read fifo failed!\n");
            return -1;
        }
        logdbg("master on slave notify\n");

        if (msg.msg == MSG_HEART_BEAT)
        {
            Session *session = m_session_mgr->get_session(msg.ip,this->m_session_life);
            if (!session)
            {
                logerr("get session failed!\n");
                return -1;
            }
            session->update_active_time();
            logdbg("master on slave notify:%s, updateto:%ld\n",
                   session->get_string_ip().c_str(),
                   session->get_active_time());
        }
        else
        {
            logerr("unknown msg:%d\n", msg.msg);
            return -1;
        }
    }
    return 0;
}

int Server::on_read(Session *session)
{
    int ret = session->on_read_event();
    if (ret < 0)
    {
        on_close(session);
    }
    return ret;
}

int Server::on_write(Session *session)
{
    session->on_write_event();
    return 0;
}

int Server::on_close(Session *session)
{
    epoll_del(session->get_sockfd(), EPOLLIN);
    session->on_close();
    return 0;
}

int Server::run()
{
    epoll_add(m_listen_fd, EPOLLIN | EPOLLPRI, &m_listen_fd);

    for (;;)
    {
        int nfds = epoll_wait(m_ep_fd, m_ep_events, m_ep_max_poll, 1000);
        for (int i = 0; i < nfds; i++)
        {
            if (m_ep_events[i].data.ptr == &m_listen_fd)
            {
                struct sockaddr_in client_addr;
                socklen_t addr_len = sizeof(struct sockaddr_in);
                int sockfd = accept(m_listen_fd, (struct sockaddr *)(&client_addr),
                                    &addr_len);
                if (sockfd > 0)
                {
                    on_connect(sockfd, &client_addr);
                }
            }
            else if (m_ep_events[i].events & EPOLLIN)
            {
                if (g_is_master && m_ep_events[i].data.ptr == &g_fifo_fd)
                {
                    on_slave_notify();
                }
                else
                {
                    on_read((Session *)m_ep_events[i].data.ptr);
                }
            }
            else if (m_ep_events[i].events & EPOLLOUT)
            {
                on_write((Session *)m_ep_events[i].data.ptr);
            }
            else if (m_ep_events[i].events & (EPOLLHUP | EPOLLERR))
            {
                if (m_ep_events[i].data.ptr == &g_fifo_fd)
                {
                    logerr("error on fifo fd:%d\n", g_is_master);
                }
                else
                {
                    logerr("error on fifo fd on_close\n");
                    on_close((Session *)m_ep_events[i].data.ptr);
                }
            }
            else
            {
                logerr("error else on epoll:on_close\n");
                on_close((Session *)m_ep_events[i].data.ptr);
            }
        }
        do_update_tv();
        m_timer_list->check_expired();
    }
    return 0;
}

