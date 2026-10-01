#ifndef SOCKET_COMPAT_H
#define SOCKET_COMPAT_H

#include <cstdint>

#if defined(__linux__) || defined(__unix__)
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>

using socket_t = int;
constexpr socket_t INVALID_SOCKET_FD = -1;
constexpr int SOCKET_ERROR_VAL = -1;

inline bool initSocketLibrary() { return true; }
inline void cleanupSocketLibrary() {}
inline void closeSocketFd(socket_t s) {
    if (s >= 0) {
        close(s);
    }
}
inline void shutdownSocket(socket_t s) {
    if (s >= 0) {
        shutdown(s, SHUT_RDWR);
    }
}
inline int getLastSocketError() { return errno; }

#else

#include <winsock2.h>
#include <ws2tcpip.h>

using socket_t = SOCKET;
constexpr socket_t INVALID_SOCKET_FD = INVALID_SOCKET;
constexpr int SOCKET_ERROR_VAL = SOCKET_ERROR;

inline bool initSocketLibrary() {
    static bool initialized = false;
    if (!initialized) {
        WSADATA wsaData;
        int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
        initialized = (res == 0);
    }
    return initialized;
}

inline void cleanupSocketLibrary() {
    WSACleanup();
}

inline void closeSocketFd(socket_t s) {
    if (s != INVALID_SOCKET) {
        closesocket(s);
    }
}

inline void shutdownSocket(socket_t s) {
    if (s != INVALID_SOCKET) {
        shutdown(s, SD_BOTH);
    }
}

inline int getLastSocketError() {
    return WSAGetLastError();
}

#endif

#endif // SOCKET_COMPAT_H
