#include "Ipv4Address.h"

#include <assert.h>
#include <memory.h>
#include <stdio.h>

#include <netdb.h>

/**
    Construct an uninitialised address - IsValid() shall return false.
**/
Ipv4Address::Ipv4Address()
{
    sockaddr_in* addr = reinterpret_cast<sockaddr_in*>( &m_addr );
    addr->sin_family = AF_INET - 1;
}

/**
    Construct an address from the specified hostname and port number.

    If the hostname is resolved successfully then this object will hold
    the address of the specified host (and IsValid() will return true).
    If the hostname cannot be resolved then IsValid() will return false.
*/
Ipv4Address::Ipv4Address( const char* hostname, int portNumber )
{
    GetHostByName( hostname, portNumber );
}

/**
    Copy constructor - performs a memcpy on the underlying addresss structure.
*/
Ipv4Address::Ipv4Address( const Ipv4Address& other )
{
    memcpy( &m_addr, &other.m_addr, sizeof( m_addr ) );
}

/**
    Nothing to clean up.
*/
Ipv4Address::~Ipv4Address()
{
    
}

/**
    @return true if the current address is a valid IPv4 address, false otherwise.
*/
bool Ipv4Address::IsValid() const
{
    return m_addr.ss_family == AF_INET;
}

/**
    Resolve the host name from this Ipv4 address.

    The address must already be known to be valid or the string "(invalid address)" is returned.

    @param [out] host string representation of host name. Resolved name or numeric IP if it can't be resolved.
*/
void Ipv4Address::GetHostName( std::string& host ) const
{
    GetHostNameInfo( host, 0 );
}

/**
    Return numeric string representation of this address, for example "127.0.0.1".

    The address must already be known to be valid or the string "(invalid address)" is returned.

    @param [out] host string representation of numeric IP.
*/
void Ipv4Address::GetHostAddress( std::string& host ) const
{
    GetHostNameInfo( host, NI_NUMERICHOST );
}

uint16_t Ipv4Address::GetPort() const
{
    return ntohs( ( reinterpret_cast<const sockaddr_in*>(&m_addr) )->sin_port );
}

void Ipv4Address::SetPort( uint16_t port )
{
    sockaddr_in* addr = reinterpret_cast<sockaddr_in*>(&m_addr);
    addr->sin_port = htons( port );
}

/**
    Resolve an address from a host name and port number.

    If successful then this object will hold the address of the specified host (and IsValid() will return true).

    @todo gethostbyname is obsolete - should use getaddrinfo
*/
void Ipv4Address::GetHostByName( const char* hostname, int portNumber )
{
    memset( (void*)&m_addr, 0, sizeof(sockaddr_storage) );
    sockaddr_in* addr = reinterpret_cast<sockaddr_in*>(&m_addr);

    struct hostent* host = gethostbyname( hostname );

    if ( host == 0 )
    {
        addr->sin_family = AF_INET - 1;
    }
    else
    {
        addr->sin_family = AF_INET;
        memcpy( &addr->sin_addr.s_addr, host->h_addr, host->h_length );
        addr->sin_port = htons( portNumber );
    }
}

/**
    Private function which gets the host name info using getnameinfo().
*/
void Ipv4Address::GetHostNameInfo( std::string& host, int nameInfoFlags ) const
{
    const sockaddr* addr = reinterpret_cast<const sockaddr*>(&m_addr);

    int size = 16;
    char* buffer = new char[ size ];
    int err = getnameinfo( addr, sizeof(m_addr), buffer, size, 0, 0, nameInfoFlags );

    // If the buffer is too small keep growing and retrying until it is ok:
    while ( err == EAI_OVERFLOW && size < 512 )
    {
        size *= 2;
        delete [] buffer;
        buffer = new char[ size ];
        err = getnameinfo( addr, sizeof(m_addr), buffer, size, 0, 0, nameInfoFlags );
    }

    if ( err == 0 )
    {
        host = buffer;
    }

    delete [] buffer;
}

/**
    Private member for friend classes that need direct access to the sockaddr_in structure
    (e.g. Socket objects).
**/
const sockaddr_in* Ipv4Address::Get_sockaddr_in_Ptr() const
{
    return reinterpret_cast<const sockaddr_in*>(&m_addr);
}

/**
    Private member for friend classes that need direct access to the sockaddr_storage structure
    (e.g. Socket objects).
*/
const sockaddr_storage* Ipv4Address::Get_sockaddr_storage_Ptr() const
{
    return &m_addr;
}
