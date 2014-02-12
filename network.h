/**
 ***All Rights Reserved
 *
 *author frankiezhu
 *date 2013-03-22
*/

#ifndef __NETWORK_HEADER__
#define  __NETWORK_HEADER__

#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

typedef unsigned char u_char;
typedef unsigned short u_short;

int readn(int fd, u_char *ptr, int nbytes);

int writen(int fd, u_char *ptr, int nbytes);

int timeout_connect(int fd, struct sockaddr *serv_addr, int timeout);

#endif

