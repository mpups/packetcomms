#include "UdpSocket.h"

#include <assert.h>
#include <stdio.h>

#ifdef WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <memory.h>
#else
  #include <unistd.h>
  #include <errno.h>
  #include <memory.h>
  #include <netinet/in.h>
  #include <netdb.h>
  #include <netinet/tcp.h>
  #include <fcntl.h>
#endif

#include "WinsockInit.h"

#include "Ipv4Address.h"

/**
    Initialise the base class with an invlaid socket ID then create
    a datagram socket.
**/
UdpSocket::UdpSocket ()
{
#ifdef WIN32
    initWinsock();
#endif
    m_socket = socket( AF_INET, SOCK_DGRAM, 0 );
    assert( m_socket != -1 );
}

/**
    Nothing to do, the socket is closed in the base class' destructor.
**/
UdpSocket::~UdpSocket ()
{
}

/**
    Send a datagram to an unconnected socket at the specified IPv4 address.

    @param [in] addr A valid IPv4 address you want to send to.
    @param [in] Pointer to data you want to send.
    @param [in] size Length of message to send in bytes.

    It is an error to supply an invalid address object and -1 is returned in this case.

    @return Number of bytes sent or -1 on error. If the call would block and the socket is non-blocking 0 can be returned.
**/
int UdpSocket::SendTo( const Ipv4Address& addr, const char* message, size_t size )
{
    if ( addr.IsValid() == false )
    {
        return -1;
    }

#ifdef WIN32
    int n = sendto( m_socket, message, size, 0, (struct sockaddr*)addr.Get_sockaddr_in_Ptr(), sizeof(struct sockaddr_in) );
    if ( n == -1 )
    {
        int error = WSAGetLastError();
        if ( error == WSAEWOULDBLOCK )
        {
            n = 0;
        }
    }
#else
    int n = sendto( m_socket, message, size, MSG_NOSIGNAL, (struct sockaddr*)addr.Get_sockaddr_in_Ptr(), sizeof(struct sockaddr_in) );
    if ( n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK) )
    {
        n = 0;
    }
#endif
    
    return n;
}

/**
    Receive a datagram from an unconnected socket at the specified IPv4 address.

    @param [in] Pointer to buffer for storing the message.
    @param [in] size Length of buffer.
    @param [out] addr if not null the address of the message source will be filled in here.

    It is an error to supply an invalid address object and -1 is returned in this case.

    @return Number of received or -1 on error. If the call would block and the socket is non-blocking 0 can be returned.
**/
int UdpSocket::ReceiveFrom( char* message, size_t size, Ipv4Address* addr )
{
    sockaddr* sockAddr = 0;
    socklen_t length = 0;
    if ( addr != 0 )
    {
        if ( addr->IsValid() == false )
        {
            return -1;
        }
        sockAddr = (sockaddr*)addr->Get_sockaddr_in_Ptr();
        length = sizeof(sockaddr_in);
    }

#ifdef WIN32
    int n = recvfrom( m_socket, message, size, 0, sockAddr, &length );
    if ( n == -1 )
    {
        int error = WSAGetLastError();
        if ( error == WSAEWOULDBLOCK )
        {
            n = 0;
        }
    }
#else
    int n = recvfrom( m_socket, message, size, MSG_NOSIGNAL, sockAddr, &length );
    if ( n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK) )
    {
        n = 0;
    }
#endif
    
    return n;
}

