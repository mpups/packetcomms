#ifndef __TCP_SOCKET_H__
#define __TCP_SOCKET_H__

#include "Socket.h"

/**
    Class for creating TCP (stream) sockets.

    Inherits from the base class Socket which provides the generic socket functionality.
**/
class TcpSocket : public Socket
{
public:
    TcpSocket();
    virtual ~TcpSocket();

    bool Listen( int );
    TcpSocket* Accept();

    void SetNagleBufferingOn();
    void SetNagleBufferingOff();

private:
    explicit TcpSocket( int fd );
};

#endif /* __TCP_SOCKET_H__ */

