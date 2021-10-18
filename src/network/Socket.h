#ifndef ROBOLIB_SOCKET_H
#define ROBOLIB_SOCKET_H

#ifdef __linux

#include <sys/types.h>
#include <sys/socket.h>

#endif

class Ipv4Address;

#include "AbstractSocket.h"

/**
    Wrapper object for sockets API.

    @todo Socket error messages should say 'Warning' not 'error'

    @todo TcpSocket should be derived from this base class instead of embedded in it.
**/
class Socket : public AbstractWriter, public AbstractReader
{
public:
    Socket();
    virtual ~Socket();

    bool IsValid() const;

    bool Bind( int );
    void Shutdown();
    bool Connect( const char*, int );
    bool Connect( const Ipv4Address& address );

    int Read( char* message, size_t maxBytes );
    int Write( const char* message, size_t size );

    void SetBlocking( bool );

    bool GetPeerAddress( Ipv4Address& address );

    bool ReadyForReading( int timeoutInMilliseconds = -1 ) const;
    bool ReadyForWriting( int timeoutInMilliseconds = -1 ) const;

protected:
    int m_socket;

    bool WaitForSingleEvent( const short pollEvent, int timeoutInMilliseconds ) const;

private:
};

#endif // ROBOLIB_SOCKET_H

