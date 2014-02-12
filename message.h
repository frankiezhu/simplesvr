/**
 ***All Rights Reserved
 *
 *author frankiezhu
 *date 2013-03-22
*/

#ifndef __MESSAGE_HEADER__
#define  __MESSAGE_HEADER__

#include <sys/resource.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

typedef unsigned int u32;


typedef enum msg_type
{
    MSG_HEART_BEAT = 0,
    MSG_ONE,
    MSG_MAX
} msg_type_t;


struct msg_header
{
    u_short version;
    u_short msg_type;
    int length;
};


struct commu_msg
{
    u32 ip;
    u32 msg;
};


#define FILE_MSG_DIR "/dev/shm/simplesvr"
#define FILE_NAME_LEN 256

extern bool g_is_master;
extern int g_fifo_fd;
extern pid_t g_pid;

extern const char *g_msg_file_name[MSG_MAX];


#endif
