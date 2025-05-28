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
    constexpr uint32_t PACKET_SIGNATURE = 0x7FF01CAF;
    constexpr size_t PACKET_SIZE = 20; // 4 byte signature + 4 byte count + 3*4 bytes floats

    /**
     * Constructs the config struct for the serial port.
     */
    Config::Config(long baud, const char* dev) : baud_rate(baud) {
        std::strncpy(device, dev, sizeof(device) - 1);
        device[sizeof(device) - 1] = '\0';
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
    int find_packet_signature(const char* buffer, const int buffer_size) {
        static constexpr int block_size = sizeof(uint32_t);

        for (int i = 0; i < buffer_size - block_size; i++) {
            uint32_t four_byte_block;

            std::memcpy(&four_byte_block, &buffer[i], block_size);

            if (from_network_byte_order(four_byte_block) == PACKET_SIGNATURE) return i;
        }

        return -1;
    }

    /**
     * Takes in a serial port config and returns the latest full IMU packet from the device as an optional (may fail)
     */
    std::optional<Packet> read_from_device(const Config& config) {
        Packet new_packet;

        int serial_port = open(config.device, O_RDWR | O_NOCTTY);
        if (serial_port < 0) {
            printf("Error opening port. (%i - %s)\n", errno, strerror(errno));
            return std::nullopt;
        }

        termios tty;

        if (tcgetattr(serial_port, &tty) != 0) {
            printf("Error creating TTY config. (%i - %s)\n", errno, strerror(errno));
            close(serial_port);
            return std::nullopt;
        }

        tty.c_cflag = 0;
        tty.c_cflag |= CS8;             // 8 bits per byte
        tty.c_cflag |= CREAD | CLOCAL;  // Enable read and ignore control lines

        tty.c_lflag &= ~ICANON;         // Disable canonical input (waiting for newlines)
        tty.c_iflag = 0;                // Disable all input processing (ie., character echoing and signal character processing)

        tty.c_cc[VMIN] = 1;             // Blocking read for at least 1 byte
        tty.c_cc[VTIME] = 5;            // 0.5 second timeout for following bytes

        cfsetspeed(&tty, config.baud_rate); // Set Input baud rate

        // Save tty settings
        if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
            printf("Error saving TTY config. (%i - %s)\n", errno, strerror(errno));
            close(serial_port);
            return std::nullopt;
        }

        tcflush(serial_port, TCIOFLUSH);

        char read_buffer[3 * PACKET_SIZE];
        size_t total_bytes_read = 0;

        while (total_bytes_read < sizeof(read_buffer)) {
            int bytes_read = read(serial_port, read_buffer + total_bytes_read, sizeof(read_buffer) - total_bytes_read);
            
            if (bytes_read < 0) {
                std::cerr << "Error reading from serial port: " << strerror(errno) << "\n";
                close(serial_port);
                return std::nullopt;
            } else if (bytes_read == 0) {
                std::cerr << "Timeout when reading from serial port.\n";
                close(serial_port);
                return std::nullopt;
            }

            total_bytes_read += bytes_read;
        }

        close(serial_port);

        int signature_location = find_packet_signature(read_buffer, total_bytes_read);
        if (signature_location == -1) {
            printf("No valid packet signature found.\n");
            return std::nullopt;
        }
        
        if (signature_location + PACKET_SIZE > total_bytes_read) {
            printf("Incomplete packet found\n");
            return std::nullopt;
        }

        constexpr int COUNT_OFFSET = 4;
        constexpr int X_OFFSET = 8;
        constexpr int Y_OFFSET = 12;
        constexpr int Z_OFFSET = 16;

        uint32_t temp_u32;
    
        // Packet count
        std::memcpy(&temp_u32, &read_buffer[signature_location + COUNT_OFFSET], sizeof(uint32_t));
        new_packet.packet_count = from_network_byte_order(temp_u32);

        // X_rate_rdps
        std::memcpy(&temp_u32, &read_buffer[signature_location + X_OFFSET], sizeof(uint32_t));
        new_packet.X_rate_rdps = parse_float(temp_u32);

        // Y_rate_rdps
        std::memcpy(&temp_u32, &read_buffer[signature_location + Y_OFFSET], sizeof(uint32_t));
        new_packet.Y_rate_rdps = parse_float(temp_u32);

        // Z_rate_rdps
        std::memcpy(&temp_u32, &read_buffer[signature_location + Z_OFFSET], sizeof(uint32_t));
        new_packet.Z_rate_rdps = parse_float(temp_u32);

        return new_packet;
    }
}