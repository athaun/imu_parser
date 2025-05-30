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

    /**
     * @brief Initializes the UDP socket for broadcasting.
     * 
     * @param broadcast_ip The IP address to broadcast to (e.g., "127.255.255.255")
     * @param port The port number to use for broadcasting
     * @return true if the socket was successfully created and configured, false otherwise
     */
    bool init(const char* broadcast_ip, int port) {
        // Create a UDP socket 
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            printf("Failed to create socket (%d - %s)\n", errno, strerror(errno));
            return false;
        }

        // Configure the socket to broadcast data
        int enable_broadcast = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &enable_broadcast, sizeof(enable_broadcast)) < 0) {
            printf("Failed to enable broadcast (%d - %s)\n", errno, strerror(errno));
            close(sockfd);
            sockfd = -1;
            return false;
        }

        memset(&broadcast_addr, 0, sizeof(broadcast_addr));
        broadcast_addr.sin_family = AF_INET; // IPv4
        broadcast_addr.sin_port = htons(port); // Convert to network byte order
        broadcast_addr.sin_addr.s_addr = inet_addr(broadcast_ip); 

        return true;
    }

    /**
     * @brief Sends a string of bytes/chars as a broadcast message.
     * 
     * @param message The message to broadcast
     * @return true if the message was successfully sent, false otherwise
     */
    bool send(const char* message) {
        size_t sent = sendto(sockfd, message, strlen(message), 0, (sockaddr*)&broadcast_addr, sizeof(broadcast_addr));
        if (sent < 0) {
            printf("Broadcast failed to send (%d - %s)\n", errno, strerror(errno));
            return false;
        }

        return true;
    }

    /**
     * @brief Cleans up the broadcaster by closing the socket.
     */
    void cleanup() {
        if (sockfd >= 0) {
            close(sockfd);
            sockfd = -1;
        }
    }

}
