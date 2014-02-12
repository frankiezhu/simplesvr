/**
 ***All Rights Reserved
 *
 *author frankiezhu
 *date 2013-03-22
*/

#include "network.h"
#include "log.h"

int readn(int fd, u_char *ptr, int nbytes)
{
    int nleft;
    int nread;

    nleft = nbytes;

    while (nleft > 0)
    {
        nread = read(fd, ptr, nleft);

        if (nread < 0)
        {
            if (errno == EINTR || errno == EINPROGRESS)
            {
                continue;
            }
            else if (errno == EWOULDBLOCK ||  errno == EAGAIN)
            {
                return (nbytes - nleft);
            }
            else
            {
                return -1;
            }
        }
        else if (nread == 0)
        {
            return -1;
        }

        nleft -= nread;
        ptr += nread;
    }

    return nbytes - nleft;
}

int writen(int fd, u_char *ptr, int nbytes)
{
    int nleft;
    int nwritten;

    nleft = nbytes;

    while (nleft > 0)
    {
        nwritten = write(fd, ptr, nleft);

        if (nwritten <= 0)
        {
            if (errno == EINTR || errno == EINPROGRESS)
            {
                continue;
            }
            else if (errno == EWOULDBLOCK ||  errno == EAGAIN)
            {
                return (nbytes - nleft);
            }
            else
            {
                return -1;
            }

        }
        nleft -= nwritten;
        ptr += nwritten;
    }

    return nbytes - nleft;
}

int timeout_connect(int fd, struct sockaddr *serv_addr, int timeout)
{
    int flags;
    fd_set writefds;
    struct timeval tv;
    int error = 0;
    int len = sizeof(error);

    /* put socket in non blocking mode for timeout support */
    flags = fcntl(fd, F_GETFL, 0);
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK))
    {
        logerr("set nonblock failed!\n");
        return -1;
    }
    if (connect(fd, serv_addr, sizeof(*serv_addr)) < 0)
    {
        if (errno != EINPROGRESS)
        {
            goto ERR;
        }
    }
    else
    {
        return 0;
    }

    /* set timeout seconds */
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    /* set fds for select */
    FD_ZERO(&writefds);
    FD_SET(fd, &writefds);

    /* check if socket is ready for read or write */
    if (select(fd + 1, NULL, &writefds, NULL, &tv) > 0)
    {
        //check if error on sock
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&len) < 0)
        {
            logerr("get socket opt failed!\n");
            error = -1;
        }
    }
    else  //time out or error
    {
        logerr("select failed!\n");
        error = -1;
    }

ERR:
    fcntl(fd, F_SETFL, flags);
    if (error)
    {
        return -1;
    }

    return 0;
}

