/**
 ***All Rights Reserved
 *
 *author frankiezhu
 *date 2013-03-22
*/

#include "message.h"

bool g_is_master = true;
int g_fifo_fd = -1;
pid_t g_pid = -1;


const char *g_msg_file_name[MSG_MAX] =
{
    "heart_beat",
    "msg_one",
};

