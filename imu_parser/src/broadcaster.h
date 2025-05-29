#pragma once

#include <cstddef>
#include <cstdint>

namespace Broadcaster {

    /**
     * @brief Initializes the UDP socket for broadcasting.
     * 
     * @param broadcast_ip The IP address to broadcast to (e.g., "127.255.255.255")
     * @param port The port number to use for broadcasting
     * @return true if the socket was successfully created and configured, false otherwise
     */
    bool init(const char* broadcast_ip, int port);

    /**
     * @brief Sends a string of bytes/chars as a broadcast message.
     * 
     * @param message The message to broadcast
     * @return true if the message was successfully sent, false otherwise
     */
    bool send(const char* message);

    /**
     * @brief Cleans up the broadcaster by closing the socket.
     */
    void cleanup();
}
