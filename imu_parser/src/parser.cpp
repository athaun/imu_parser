#include "parser.h"

#include <cstring> 
#include <fcntl.h>
#include <errno.h>  
#include <termios.h>
#include <unistd.h>
#include <iostream>

namespace IMUParser {
    // This is the expected Big Endian form
    constexpr uint32_t PACKET_SIGNATURE = 0x7FF01CAF;

    Config::Config(long baud, const char* dev) : baud_rate(baud) {
        std::strncpy(device, dev, sizeof(device) - 1);
        device[sizeof(device) - 1] = '\0';
    }

    Packet read_from_device(const Config& config) {
        Packet new_data;

        int serial_port = open(config.device, O_RDONLY);
        if (serial_port < 0) {
            printf("Error opening port\n");
            return { 0, -1, -1, -1 }; // TODO come up with a better invalid packet
        }

        termios tty;

        if (tcgetattr(serial_port, &tty) != 0) {
            printf("Error creating TTY config. (%i - %s)\n", errno, strerror(errno));
            close(serial_port);
            return { 0, -1, -1, -1 };
        }

        tty.c_cflag &= ~PARENB;         // Disable parity bit
        tty.c_cflag &= ~CSTOPB;         // Clear stop field, only one stop bit used
        tty.c_cflag &= ~CSIZE;          // Clear all the size bits
        tty.c_cflag |= CS8;             // 8 bits per byte
        tty.c_cflag &= ~CRTSCTS;        // Disable RTS/CTS hardware flow control 
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
            return { 0, -1, -1, -1 };
        }

        char read_buffer[256];

        int bytes_read = read(serial_port, &read_buffer, sizeof(read_buffer));

        close(serial_port);

        return new_data;
    }
}