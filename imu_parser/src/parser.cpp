#include "parser.h"

#include <cstring> 
#include <fcntl.h>
#include <errno.h>  
#include <termios.h>
#include <unistd.h>

namespace IMUParser {
    // This is the expected Big Endian form
    constexpr uint32_t PACKET_SIGNATURE = 0x7FF01CAF;

    Config::Config(long baud, const char* dev) : baud_rate(baud) {
        std::strncpy(device, dev, sizeof(device) - 1);
        device[sizeof(device) - 1] = '\0';
    }

    Packet read_from_device(const Config& config) {
        Packet new_data;

        

        return new_data;
    }
}