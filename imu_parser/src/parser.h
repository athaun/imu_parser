#pragma once

#include <cstdint>
#include <termios.h>
#include <vector>

namespace IMUParser {

    struct Packet {
        uint32_t packet_count;
        float X_rate_rdps, Y_rate_rdps, Z_rate_rdps;
    };

    struct Config {
        speed_t baud_rate;
        char device[20];
        int serial_port;
    
        Config(long baud, const char* dev);
    };

    /**
     * Takes in a serial port config and opens a serial port.
     * Modifies the config by adding the port ID.
     * Returns true if successful.
     */
    bool init(Config& config);

    /**
     * Closes the serial communication port
     */
    void cleanup(const Config& config); 

    /**
     * Takes in a serial port config and returns a vector of IMU packets
     */
    std::vector<Packet> read_from_device(const Config& config);
}