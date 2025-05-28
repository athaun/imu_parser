#pragma once

#include <cstdint>
#include <termios.h>

namespace IMUParser {
    struct Packet {
        uint32_t packet_count;
        float X_rate_rdps, Y_rate_rdps, Z_rate_rdps;
    };

    struct Config {
        speed_t baud_rate;
        char device[20];
    
        Config(long baud, const char* dev);
    };
    
    Packet read_from_device(const Config& config);
}