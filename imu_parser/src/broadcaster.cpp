#include <cstdio>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "broadcaster.h"

namespace Broadcaster {

    static int sockfd = -1;
    static sockaddr_in broadcast_addr = {};

    bool init(const char* broadcast_ip, int port) {
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            printf("Failed to create socket (%d - %s)\n", errno, strerror(errno));
            return false;
        }

        int broadcast_enable = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) < 0) {
            printf("Failed to enable broadcast (%d - %s)\n", errno, strerror(errno));
            close(sockfd);
            sockfd = -1;
            return false;
        }

        memset(&broadcast_addr, 0, sizeof(broadcast_addr));
        broadcast_addr.sin_family = AF_INET;
        broadcast_addr.sin_port = htons(port);
        broadcast_addr.sin_addr.s_addr = inet_addr(broadcast_ip);

        return true;
    }

    bool send(const void* data, size_t len) {
        ssize_t sent = sendto(sockfd, data, len, 0, (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr));
        if (sent < 0) {
            printf("Broadcast failed to send (%d - %s)\n", errno, strerror(errno));
            return false;
        }

        return true;
    }

    bool send_str(const char* msg) {
        return send(msg, strlen(msg));
    }

    inline void cleanup() {
        if (sockfd >= 0) {
            close(sockfd);
            sockfd = -1;
        }
    }

}
