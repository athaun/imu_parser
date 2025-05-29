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
     * @brief Initializes the serial port with the given device configuration.
     * 
     * @param config Reference to the Config struct containing device and baud rate.
     * @return true if the serial port was successfully opened and configured, false otherwise.
     */ 
    bool init(Config& config);

    /**
     * @brief Closes the serial port communication.
     * 
     * @param config Reference to the Config struct containing the serial port.
     */
    void cleanup(const Config& config); 

    /**
     * @brief Reads data from the serial port and returns a vector of parsed Packets
     * 
     * @param config The configuration for the serial port
     * @return A vector of Packet objects containing the parsed data.
     */
    std::vector<Packet> read_from_device(const Config& config);
}