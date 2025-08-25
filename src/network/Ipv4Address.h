#ifndef __IPV4_ADDRESS_H__
#define __IPV4_ADDRESS_H__

#ifdef WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
#else
  #include <netinet/in.h>
#endif

#include <string>
#include <cstdint>

/**
    Class for storing and manipulating IPv4 addresses.
**/
class Ipv4Address
{
    friend class Socket;
    friend class UdpSocket;

public:
    Ipv4Address();
    Ipv4Address( const char* hostname, int portNumber );
    Ipv4Address( const Ipv4Address& );
    virtual ~Ipv4Address();

    bool IsValid() const;

    void GetHostName( std::string& ) const;
    void GetHostAddress( std::string& ) const;
    uint16_t GetPort() const;

    void SetPort( uint16_t );

private:
    sockaddr_storage m_addr;

    void GetHostNameInfo( std::string&, int nameInfoFlags ) const;
    void GetHostByName( const char* hostname, int portNumber );

    const sockaddr_in* Get_sockaddr_in_Ptr() const;
    const sockaddr_storage* Get_sockaddr_storage_Ptr() const;
};

#endif /* __IPV4_ADDRESS_H__ */

