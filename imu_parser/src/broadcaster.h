#pragma once

#include <cstddef>
#include <cstdint>

namespace Broadcaster {

    bool init(const char* broadcast_ip, int port);

    /**
     * Sends raw binary data over the socket
    */
    bool send(const void* data, size_t len);

    /**
     * Sends a null-terminated string over the socket
     */
    bool send_str(const char* msg);

    void cleanup();
}
