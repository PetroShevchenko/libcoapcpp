#ifndef _SOCKET_H
#define _SOCKET_H

class socket
{
public:
    socket(){}
    ~socket(){}
    socket(const socket &) = delete;
    socket(socket &&) = delete;
    socket & operator=(const socket &) = delete;
    socket & operator=(socket &&) = delete;
};

#endif
