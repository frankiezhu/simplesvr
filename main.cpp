/**
 ***All Rights Reserved
 *
 *author frankiezhu
 *date 2013-03-22
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sstream>

#include "log.h"
#include "config.h"
#include "server.h"

#define SVR_LOCK_FILE "/var/run/simplesvr.pid"
#define LOCKMODE (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)
#define SVR_CONF_FILE "/etc/simplesvr_conf.xml"

static int daemonize()
{
    return daemon(0, 0);
}

static void set_core_limit(void)
{
    struct rlimit rlim, rlim_new;

    if (getrlimit(RLIMIT_CORE, &rlim) == 0)
    {
        rlim_new.rlim_cur = rlim_new.rlim_max = RLIM_INFINITY;
        if (setrlimit(RLIMIT_CORE, &rlim_new) != 0)
        {
            /* failed. try raising just to the old max */
            rlim_new.rlim_cur = rlim_new.rlim_max = rlim.rlim_max;
            setrlimit(RLIMIT_CORE, &rlim_new);
        }
    }
}

int lockfile (int fd)
{
    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    return fcntl(fd, F_SETLK, &fl);
}

static int already_running(const char *lock_file_name)
{
    int fd;
    char buf[32];

    fd = open(lock_file_name, O_RDWR | O_CREAT, LOCKMODE);
    if (fd < 0)
    {
        logerr("failed to open:%s\n", lock_file_name);
        return -1;
    }
    if (lockfile(fd) < 0)
    {
        logerr("proc is already running\n");
        close(fd);
        return -1;
    }
    ftruncate(fd, 0);
    sprintf(buf, "%ld", (long)getpid());
    write(fd, buf, strlen(buf) + 1);

    return 0;
}

static void sig_hup(int signo)
{
    int ret;
    ret = ::glb_config()->read_config(SVR_CONF_FILE);
    if (ret)
    {
        logerr("reload config:%s failed.\n", SVR_CONF_FILE);
    }
    else
    {
        logerr("reload config:%s success.\n", SVR_CONF_FILE);
    }
    return;
}

static void sig_child(int signo)
{

    pid_t pid = waitpid(-1, NULL, WNOHANG);
    if (pid > 0)
    {
        logerr("SIGCHLD, pid:%d\n", pid);
    }
    return;
}

static int setup_signal()
{
    struct sigaction sa;
    int sig_ignore_array[] = {SIGQUIT, SIGPIPE, SIGINT};
    unsigned int i;

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sa.sa_handler = &sig_child;
    if (sigaction(SIGCHLD, &sa, NULL) < 0)
    {
        return -1;
    }
    sa.sa_handler = &sig_hup;
    if (sigaction(SIGHUP, &sa, NULL) < 0)
    {
        return -1;
    }
    sa.sa_handler = SIG_IGN;
    for (i = 0; i < sizeof(sig_ignore_array) / sizeof(int); i++)
    {
        if (sigaction(sig_ignore_array[i], &sa, NULL) < 0)
        {
            return -1;
        }
    }

    return 0;
}

static void print_usage_and_exit(void)
{
    fprintf(stderr, "usage: -f no daemon -h print this\n");
    exit(-1);
    return ;
}

static int run_main()
{
    int ret;
    ret = ::glb_config()->read_config(SVR_CONF_FILE);
    if (ret < 0)
    {
        logerr("read server config file:%s failed!\n", SVR_CONF_FILE);
        return -1;
    }
    Server *svr = new(std::nothrow) Server(::glb_config()->get_listen_port(),::glb_config()->get_lifetime());
    if (!svr)
    {
        logerr("alloc svr failed!\n");
        return -1;
    }
    svr->set_worker_num(::glb_config()->get_worker_num());
    ret = svr->init_server();
    if (ret < 0)
    {
        logerr("init_server failed!\n");
        delete svr;
        return -1;
    }
    svr->run();
    return 0;
}

static int mkdirs()
{
    int count = sizeof(g_msg_file_name) / sizeof(const char *);
    if (count != MSG_MAX)
    {
        logerr("MSG not fit file_name array!%d->%d\n", MSG_MAX, count);
        return -1;
    }

    stringstream ss;
    ss<<"mkdir -p "<<FILE_MSG_DIR<<" && cd "<<FILE_MSG_DIR <<" && mkdir -p ";
    for (int i = 0; i < MSG_MAX; i++)
    {
        ss<<g_msg_file_name[i]<<" ";
    }

    int rc = system(ss.str().c_str());
    if (rc != 0)
    {
        logerr("system cmd:%s failed!\n", ss.str().c_str());
        return -1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    int ret;
    int opt;
    int be_daemon = 0;

    while ((opt = getopt(argc, argv, "dh:")) != -1)
    {
        switch (opt)
        {
        case 'd':
            be_daemon = 1;
            break;
        case 'h':
            print_usage_and_exit();
            break;
        default:
            print_usage_and_exit();
            break;
        }
    }
    set_core_limit();

    if (be_daemon)
    {
        ret = daemonize();
        if (ret < 0)
        {
            logerr("daemonize failed!\n");
            return -1;
        }
    }
    openlog("Server2", LOG_ODELAY, LOG_USER);

    ret = already_running(SVR_LOCK_FILE);
    if (ret != 0)
    {
        return -1;
    }

    if (be_daemon)
    {
        ret = setup_signal();
        if (ret < 0)
        {
            logerr("setup signal failed\n");
            return -1;
        }
    }

    ret = mkdirs();
    if (ret < 0)
    {
        return -1;
    }
    ret = run_main();
    if (ret < 0)
    {
        logerr("run main ret error!\n");
        return -1;
    }

    return 0;
}
