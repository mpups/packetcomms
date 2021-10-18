#ifndef ROBOLIB_UDP_SOCKET_H
#define ROBOLIB_UDP_SOCKET_H

#include "Socket.h"

class Ipv4Address;

/**
    Class for creating UDP (datagram) sockets.

    Inherits from the base class Socket which provides the generic socket functionality.

    UDP sockets allows connection-less communication between sockets using SendTo().
    Alternatively the socket can be connected and datagrams sent using Read() and Write().
**/
class UdpSocket : public Socket
{
public:
    UdpSocket ();
    virtual ~UdpSocket ();

    int SendTo( const Ipv4Address& addr, const char* message, size_t size );
    int ReceiveFrom( char* message, size_t size, Ipv4Address* addr = 0 );
};

#endif /* ROBOLIB_UDP_SOCKET_H */

