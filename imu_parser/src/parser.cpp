#include "parser.h"

#include <cstring> 
#include <fcntl.h>
#include <errno.h>  
#include <termios.h>
#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <limits>

namespace IMUParser {
    static constexpr uint32_t PACKET_SIGNATURE = 0x7FF01CAF;
    static constexpr size_t PACKET_SIZE = 20; // 4 byte signature + 4 byte count + 3*4 bytes floats

    /**
     * Constructs the config struct for the serial port.
     */
    Config::Config(long baud, const char* dev) : baud_rate(baud) {
        std::strncpy(device, dev, sizeof(device) - 1);
        device[sizeof(device) - 1] = '\0';
    }

    bool init (Config& config) {
        int serial_port = open(config.device, O_RDWR | O_NOCTTY);
        if (serial_port < 0) {
            printf("Error opening port. (%i - %s)\n", errno, strerror(errno));
            return false;
        }

        termios tty;

        if (tcgetattr(serial_port, &tty) != 0) {
            printf("Error creating TTY config. (%i - %s)\n", errno, strerror(errno));
            close(serial_port);
            return false;
        }

        tty.c_cflag = 0;
        tty.c_cflag |= CS8;             // 8 bits per byte
        tty.c_cflag |= CREAD | CLOCAL;  // Enable read and ignore control lines

        tty.c_lflag &= ~ICANON;         // Disable canonical input (waiting for newlines)
        tty.c_iflag = 0;                // Disable all input processing (ie., character echoing and signal character processing)

        tty.c_cc[VMIN] = 0;             // Dont block for any bytes
        tty.c_cc[VTIME] = 1;            // 0.1 second timeout for following bytes

        cfsetspeed(&tty, config.baud_rate); // Set Input baud rate

        // Save tty settings
        if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
            printf("Error saving TTY config. (%i - %s)\n", errno, strerror(errno));
            close(serial_port);
            return false;
        }

        tcflush(serial_port, TCIOFLUSH);

        config.serial_port = serial_port;

        return true;
    }

    inline void cleanup(const Config& config) {
        close(config.serial_port);
    }

    /**
     * Takes a 4 byte chunk of memory and swaps from Big Endian (network byte order) to Little Endian
     */
    uint32_t from_network_byte_order(uint32_t data) {
        return (data >> 24) |               // Swap LSB to right side
               ((data >> 8) & 0x0000FF00) | // Swap secondmost LSB with thirdmost
               ((data << 8) & 0x00FF0000) | // Swap thirdmost LSB with secondmost
               (data << 24);                // Swap MSB to left side
    }

    /**
     * Parse a IEEE 754 floating point number from a network byte ordered 32 bit chunk
     */
    float parse_float(uint32_t be_data) {
        uint32_t le_data = from_network_byte_order(be_data);
        float result;
        std::memcpy(&result, &le_data, sizeof(result));
        return result;
    }

    /**
     * Finds the index in the read buffer of the first packet signature
     */
    int find_packet_signature(const char* buffer, const size_t buffer_size, const size_t search_offset) {
        static constexpr size_t block_size = sizeof(uint32_t);

        for (size_t i = search_offset; i < buffer_size - block_size; i++) {
            uint32_t four_byte_block;

            std::memcpy(&four_byte_block, &buffer[i], block_size);

            if (from_network_byte_order(four_byte_block) == PACKET_SIGNATURE) return i;
        }

        return -1;
    }

    /**
     * Takes in a serial port config and returns the latest full IMU packet from the device as an optional (may fail)
     */
    std::vector<Packet> read_from_device(const Config& config) {
        std::vector<Packet> packets;

        char read_buffer[300 * PACKET_SIZE];
        size_t total_bytes_read = 0;

        while (total_bytes_read < sizeof(read_buffer)) {
            int bytes_read = read(config.serial_port, read_buffer + total_bytes_read, sizeof(read_buffer) - total_bytes_read);
            
            if (bytes_read < 0) {
                printf("Error reading from serial port. (%d - %s)\n", errno, strerror(errno));
                cleanup(config);
                return {};
            } else if (bytes_read == 0) {
                printf("Timeout when reading from serial port.\n");
                cleanup(config);
                return {};
            }

            total_bytes_read += bytes_read;
        }

        size_t search_offset = 0;
        while (search_offset < total_bytes_read) {
            int signature_index = find_packet_signature(read_buffer, total_bytes_read, search_offset);
            if (signature_index == -1 || signature_index + PACKET_SIZE > total_bytes_read) {
                break;
            }
    
            Packet packet;
            uint32_t temp_u32;
    
            std::memcpy(&temp_u32, &read_buffer[signature_index + 4], sizeof(uint32_t));
            packet.packet_count = from_network_byte_order(temp_u32);
    
            std::memcpy(&temp_u32, &read_buffer[signature_index + 8], sizeof(uint32_t));
            packet.X_rate_rdps = parse_float(temp_u32);
    
            std::memcpy(&temp_u32, &read_buffer[signature_index + 12], sizeof(uint32_t));
            packet.Y_rate_rdps = parse_float(temp_u32);
    
            std::memcpy(&temp_u32, &read_buffer[signature_index + 16], sizeof(uint32_t));
            packet.Z_rate_rdps = parse_float(temp_u32);
    
            packets.push_back(packet);
    
            search_offset = signature_index + PACKET_SIZE;
        }
    
        if (packets.empty()) {
            printf("No complete valid packets found.\n");
        }
    
        return packets;
    }
}