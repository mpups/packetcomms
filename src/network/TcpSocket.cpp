#include "TcpSocket.h"


#include <assert.h>

#include <unistd.h>
#include <errno.h>
#include <memory.h>
#include <stdio.h>

#include <netinet/in.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <fcntl.h>

/**
    Initialise the base class with an invlaid socket ID then create
    a stream socket.
**/
TcpSocket::TcpSocket ()
{
    m_socket = socket( AF_INET, SOCK_STREAM, 0 );
    assert( m_socket != -1 );
}

/**
    Internal private constructor for creating socket deirectly from a specified file descriptor.

    The specified socket will be closed in the destructor of this object.

    If the file descriptor is invalid no error occurs - the resulting object will simply refer
    to an invlaid socket (Socket::IsValid() will return false).

    @param [in] socket A file descriptor that refers to a socket.
**/
TcpSocket::TcpSocket( int socket )
{
    m_socket = socket;
}

/**
    Nothing to do, the socket is closed in the base class' destructor.
**/
TcpSocket::~TcpSocket ()
{
}

/**
    Listen for connections.
    
    @param queueSize Max number of pending connections.
**/
bool TcpSocket::Listen( int queueSize )
{
    int err = listen( m_socket, queueSize );
    return err == 0;
}

/**
    Accept a connection from a bound socket.

    This socket must be bound or null will be returned.

    @note It is the caller's responsibility to delete the returned object.

    @return New client socket connection - or null on error or timeout.
**/
std::unique_ptr<TcpSocket> TcpSocket::Accept()
{
  std::unique_ptr<TcpSocket> connection;

  int reuse = 1;
  setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));

  struct sockaddr_in addr;
  memset((void*)&addr, 0, sizeof(sockaddr_in));

  socklen_t clientSize = sizeof(addr);
  int socket = accept(m_socket, (struct sockaddr*)&addr, &clientSize);
  if (socket != -1) {
    connection.reset(new TcpSocket(socket));
  }

  return connection;
}


/**
    Enable TCP Nagle buffering.
**/
void TcpSocket::SetNagleBufferingOn()
{
    int flag = 0;
    int result = setsockopt( m_socket,           // socket affected
                            IPPROTO_TCP,    // set option at TCP level
                            TCP_NODELAY,    // name of option
                            (char *) &flag, // the cast is historical cruft
                            sizeof(int)     // length of option value
                           );
    if ( result < 0 )
    {
        fprintf( stderr, "SetSocketOpt failed: %s\n", strerror(errno) );
    }
}

/**
    Disable TCP Nagle buffering.
**/
void TcpSocket::SetNagleBufferingOff()
{
    int flag = 1;
    int result = setsockopt( m_socket,           // socket affected
                            IPPROTO_TCP,    // set option at TCP level
                            TCP_NODELAY,    // name of option
                            (char *) &flag, // the cast is historical cruft
                            sizeof(int)     // length of option value
                           );
    if ( result < 0 )
    {
        fprintf( stderr, "SetSocketOpt failed: %s\n", strerror(errno) );
    }
}

