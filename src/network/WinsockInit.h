#ifndef WINSOCK_INIT_H
#define WINSOCK_INIT_H

#ifdef WIN32
#include <winsock2.h>
#include <cstdlib>

// Simple WSA initialization - done once on first socket creation
inline void initWinsock() {
    static bool wsaInitialized = false;
    if (!wsaInitialized) {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2,2), &wsaData);
        wsaInitialized = true;
        std::atexit([]() { WSACleanup(); });
    }
}
#endif

#endif // WINSOCK_INIT_H
