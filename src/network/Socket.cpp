#include "Socket.h"

#include <assert.h>

#include <unistd.h>
#include <errno.h>
#include <memory.h>

#include <netinet/tcp.h>
#include <fcntl.h>
#include <poll.h>

#include "Ipv4Address.h"

#include <iostream>

/**
    Initialises the internal socket to an invalid value.
**/
Socket::Socket()
:
    m_socket (-1)
{
}

/**
    Closes the socket connection.
**/
Socket::~Socket()
{
    if ( m_socket != -1 )
    {
        close( m_socket );
    }
}

bool Socket::IsValid() const
{
    return m_socket >= 0;
}

/**
    Bind this socket to the specified port-number on any INET address.
**/
bool Socket::Bind( int portNumber )
{
    struct sockaddr_in addr;
    memset( (void*)&addr, 0, sizeof(sockaddr_in) );
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl( INADDR_ANY );
    addr.sin_port        = htons( portNumber );
    
    int err = bind( m_socket, (struct sockaddr*)&addr, sizeof(addr) );
    if ( err == -1 )
    {
        std::clog <<  __FILE__ << ": Error " << strerror(errno) << std::endl;
    }
    
    return err != -1;
}

/**
    Shutdown this socket's connections.
**/
void Socket::Shutdown()
{
    int err = shutdown( m_socket, SHUT_RDWR );
    if ( err == -1 )
    {
        std::clog <<  __FILE__ << ": Error in Shutdown() " << strerror(errno) << std::endl;
    }
}

/**
    Connect this socket to a server at specified address.

    @param [in] hostname name of host to connect to. Can be numeric IP or host name.
    @param [in] portNumber integer port number to connect to.
    @return True if the connection was successful.
**/
bool Socket::Connect( const char* hostname, int portNumber )
{
    return Connect( Ipv4Address( hostname, portNumber ) );
}

/**
    Connect this socket by directly providing a Ipv4Address object.

    @param [in] address an Ipv4Address object.
    @return true if the connection was successful, false otherwise.
*/
bool Socket::Connect( const Ipv4Address& addr )
{
    if ( addr.IsValid() == false )
    {
        return false;
    }

    int err = connect( m_socket, (struct sockaddr*)addr.Get_sockaddr_in_Ptr(), sizeof(struct sockaddr_in) );
    if ( err == -1 )
    {
        std::clog <<  __FILE__ << ": Error " << strerror(errno) << std::endl;
    }
    
    return err != -1;
}

/**
    Blocking read from this socket.
    
    In the case of non-blocking IO Read returns 0 if no bytes were
    immediately available.

    @note  On error (return of -1) errno will still be set, but EAGAIN and EWOULDBLOCK are
    never returned as errors - instead they return 0 bytes read).

    @param message Storage for received bytes.
    @return Number of bytes read or -1 if there was an error.
**/
int Socket::read( char* message, size_t maxBytes )
{
    int n = recv( m_socket, message, maxBytes, MSG_NOSIGNAL );

    if ( n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK) )
    {
        // For non-blocking IO just return zero bytes were available.
        // Otherwise we return the error (-1)
        n = 0;
    }
    
    return n;
}

/**
    In the case of non-blocking IO Write returns 0 if the write would have
    caused the process to block.

    @note  On error (return of -1) errno will still be set, but EAGAIN and EWOULDBLOCK are
    never returned as errors - instead they return 0 bytes read).

    @param message Bytes to send, must be at least size bytes in the buffer.
    @return Number of bytes written or -1 if there was an error.
**/
int Socket::write( const char* message, size_t size )
{
    int n = send( m_socket, message, size, MSG_NOSIGNAL );
    if ( n < 0 )
    {
        //fprintf( stderr, "socket error: %s\n", strerror(errno) );
    }

    if ( n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK) )
    {
        n = 0;
    }
    
    return n;
}

/**
    @param block True set socket to blocking mode, false sets socket to non-blocking.
**/
void Socket::setBlocking( bool block )
{
    if ( block )
    {
        fcntl( m_socket, F_SETFL, O_ASYNC );
    }
    else
    {
        fcntl( m_socket, F_SETFL, O_NONBLOCK );
    }
}

/**
    Get the IPV4 address of the peer connected to this socket.

    Will fail (returning false) if the socket is not connected or the peer does not have an Ipv4 address.

    @param address the address of the connected peer if return value is true, undefined otherwise.
    @return true if an Ipv4 address was retrieved successfully, false otherwise.
*/
bool Socket::GetPeerAddress( Ipv4Address& address )
{
    const sockaddr_storage* addr_storage = address.Get_sockaddr_storage_Ptr();
    sockaddr* addr = const_cast<sockaddr*>( reinterpret_cast< const sockaddr* >( addr_storage ) );
    socklen_t length = sizeof( sockaddr_storage );
    int err = getpeername( m_socket, addr, &length );

    if ( err == 0 || length > sizeof( sockaddr_storage ) )
    {
        if ( addr_storage->ss_family == AF_INET )
        {
            return true;
        }
    }

    return false;
}

/**
    Wait (sleep) until data is available for reading from the socket.

    @param timeout Maximum time to wait in milliseconds (default value leads to no timeout).
    @return true if data is ready, false if the wait timed-out or there was an error.
*/
bool Socket::readyForReading( int timeoutInMilliseconds ) const
{
    return WaitForSingleEvent( POLLIN, timeoutInMilliseconds );
}

/**
    Wait (sleep) until a write to the socket will not block.

    @param timeout Maximum time to wait in milliseconds (default value leads to no timeout).
    @return true if write will not block, false if the wait timed-out or there was an error.
*/
bool Socket::ReadyForWriting( int timeoutInMilliseconds ) const
{
    return WaitForSingleEvent( POLLOUT, timeoutInMilliseconds );
}

/**
    Use system call poll() to wait on the socket's file descriptor
    for a single poll event.

    @param pollEvent The event to wait and check for - only a single flag may be set.
    @return true if the event
*/
bool Socket::WaitForSingleEvent( const short pollEvent, int timeoutInMilliseconds ) const
{
    struct pollfd pfds;
    pfds.fd = m_socket;
    pfds.events = pollEvent;
    pfds.revents = 0;
    int val = poll( &pfds, 1, timeoutInMilliseconds );

    if (val == -1)
    {
        std::clog << __FILE__ << ": Error from poll() - " << strerror(errno) << std::endl;
    }

    if( (pfds.revents & POLLERR) )
    {
        std::clog << __FILE__ << ": POLLERR!\n";
    }

    if( (pfds.revents & POLLHUP) )
    {
        std::clog << __FILE__ << ": POLLHUP!\n";
        /// @todo - need return to indicate hang-up for robust shutdown.
    }

    if( (pfds.revents & POLLNVAL) )
    {
        std::clog << __FILE__ << ": POLLNVAL!\n";
    }

    // If successful then val should be one because
    // we were only waiting on one file descriptor.
    if ( val == 1 && (pfds.revents & pollEvent) )
    {
        return true;
    }
    else
    {
        return false;
    }
}
