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
     * @brief Constructs the config struct for the serial port.
     * 
     * @param baud Baud rate for the serial communication.
     * @param dev Device path for the serial port (e.g., "/dev/tty1").
     */
    Config::Config(long baud, const char* dev) : baud_rate(baud) {
        std::strncpy(device, dev, sizeof(device) - 1);
        device[sizeof(device) - 1] = '\0';
    }

    /**
     * @brief Initializes the serial port with the given device configuration.
     * 
     * @param config Reference to the Config struct containing device and baud rate.
     * @return true if the serial port was successfully opened and configured, false otherwise.
     */ 
    bool init(Config& config) {
        if (strlen(config.device) == 0) {
            printf("Invalid config passed to IMU parser");
            return false;
        }

        int serial_port = open(config.device, O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (serial_port < 0) {
            printf("Error opening port (%i - %s)\n", errno, strerror(errno));
            return false;
        }

        termios tty;

        if (tcgetattr(serial_port, &tty) != 0) {
            printf("Error creating TTY config (%i - %s)\n", errno, strerror(errno));
            close(serial_port);
            return false;
        }

        tty.c_cflag = 0;
        tty.c_cflag |= CS8;             // 8 bits per byte
        tty.c_cflag |= CREAD | CLOCAL;  // Enable read and ignore control lines

        tty.c_lflag &= ~ICANON;         // Disable canonical input (waiting for newlines)
        tty.c_iflag = 0;                // Disable all input processing (ie., character echoing and signal character processing)

        tty.c_cc[VMIN] = 1;             // Block for 1 byte
        tty.c_cc[VTIME] = 0;            // Immediate timeout

        cfsetspeed(&tty, config.baud_rate); // Set Input baud rate

        // Save tty settings
        if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
            printf("Error saving TTY config (%i - %s)\n", errno, strerror(errno));
            close(serial_port);
            return false;
        }

        tcflush(serial_port, TCIOFLUSH);

        config.serial_port = serial_port;

        return true;
    }

    /**
     * @brief Closes the serial port communication.
     * 
     * @param config Reference to the Config struct containing the serial port.
     */
    inline void cleanup(const Config& config) {
        close(config.serial_port);
    }

    /**
     * @brief Converts a 32-bit unsigned integer from network byte order (big-endian) to little-endian
     * 
     * @param data The 32-bit unsigned integer in network byte order
     * @return The 32-bit unsigned integer in little-endian byte order
     */
    uint32_t from_network_byte_order(uint32_t data) {
        return (data >> 24) |               // Swap LSB to right side
               ((data >> 8) & 0x0000FF00) | // Swap secondmost LSB with thirdmost
               ((data << 8) & 0x00FF0000) | // Swap thirdmost LSB with secondmost
               (data << 24);                // Swap MSB to left side
    }

    /**
     * @brief Searches for the packet signature in the buffer starting from a given offset
     * 
     * @param buffer The buffer containing the raw byte-data to search
     * @param buffer_size The size of the buffer
     * @param search_offset The offset in the buffer to start searching from
     * @return The index of the packet signature in the buffer, or -1 if not found
     */
    size_t find_packet_signature(const char* buffer, const size_t buffer_size, const size_t search_offset) {
        static constexpr size_t block_size = sizeof(uint32_t);

        for (size_t i = search_offset; i < buffer_size - block_size; i++) {
            uint32_t four_byte_block;

            std::memcpy(&four_byte_block, &buffer[i], block_size);

            if (from_network_byte_order(four_byte_block) == PACKET_SIGNATURE) return i;
        }

        return -1;
    }

    /**
     * @brief Parses a 4 byte chunk in network byte order (big-endian) into a 32-bit unsigned integer
     * 
     * @param read_buffer The buffer containing the raw byte-data read from the serial port
     * @param offset The offset in the buffer where the 4 byte chunk starts
     * @return The original 4 bytes now in little-endian byte order as a 32-bit unsigned integer
     */
    uint32_t parse_u32(const std::vector<char>& read_buffer, size_t offset) {
        uint32_t temp;
        std::memcpy(&temp, &read_buffer[offset], sizeof(uint32_t));
        return from_network_byte_order(temp);
    }

    /**
     * @brief Parses a 4 byte chunk in network byte order (big-endian) into an IEEE-754 float
     * 
     * @param read_buffer The buffer containing the raw byte-data read from the serial port
     * @param offset The offset in the buffer where the 4 byte chunk starts
     * @return The original 4 bytes now interpreted as a float
     */
    float parse_float(const std::vector<char>& read_buffer, size_t offset) {
        uint32_t temp = parse_u32(read_buffer, offset);
        float result;
        std::memcpy(&result, &temp, sizeof(result));
        return result;
    }

    /**
     * @brief Parses the packets from the read buffer and extracts the relevant data into a vector of Packets.
     * 
     * @param read_buffer The buffer containing the raw byte-data read from the serial port.
     * @param packets The vector to store the parsed Packet objects.
     */
    void parse_packets(std::vector<char>& read_buffer, std::vector<Packet>& packets) {

        size_t search_offset = 0;
        while (search_offset + PACKET_SIZE <= read_buffer.size()) {

            int signature_index = find_packet_signature(read_buffer.data(), read_buffer.size(), search_offset);
            
            // If no signature found or too few bytes remaining for a full packet, break
            if (signature_index == -1 || signature_index + PACKET_SIZE > read_buffer.size()) {
                break;
            }
    
            Packet packet;
            packet.packet_count = parse_u32(read_buffer, search_offset + 4);
            packet.X_rate_rdps = parse_float(read_buffer, search_offset + 8);
            packet.Y_rate_rdps = parse_float(read_buffer, search_offset + 12);
            packet.Z_rate_rdps = parse_float(read_buffer, search_offset + 16);
    
            packets.emplace_back(packet);
            search_offset = signature_index + PACKET_SIZE;
        }
    
        // Remove processed bytes
        if (search_offset > 0) {
            read_buffer.erase(read_buffer.begin(), read_buffer.begin() + search_offset);
        }
    }

    /**
     * @brief Reads data from the serial port and returns a vector of parsed Packets
     * 
     * @param config The configuration for the serial port
     * @return A vector of Packet objects containing the parsed data.
     */
    std::vector<Packet> read_from_device(const Config& config) {
        
        static std::vector<char> read_buffer;   
        static std::vector<Packet> packets;
        static char tmp_buffer[128];

        packets.clear();       

        while (true) {
            // Do a non-blocking read from the serial port into a small temporary buffer
            int bytes_read = read(config.serial_port, tmp_buffer, sizeof(tmp_buffer));
            
            if (bytes_read > 0) {                
                // Append the temporary buffer to the read buffer for later processing
                read_buffer.insert(read_buffer.end(), tmp_buffer, tmp_buffer + bytes_read);
            } else if (bytes_read == 0 || (bytes_read < 0 && errno == EAGAIN)) {
                // No more data available now
                break;
            } else {
                perror("Error reading from serial port\n");
                cleanup(config);
                return {};
            }
        }
        
        parse_packets(read_buffer, packets);

        return packets;
    }    
}