#include <iostream>
#include "parser.h"

int main() {
    auto device_cfg = IMUParser::Config(B921600, "/dev/pts/4");
    auto packet = IMUParser::read_from_device(device_cfg);

    if (packet.has_value()) {
        IMUParser::Packet& p = packet.value();
        printf("Parsed packet: count: %u, X: %.3f, Y: %.3f, Z: %.3f\n",
            p.packet_count, p.X_rate_rdps, 
            p.Y_rate_rdps, p.Z_rate_rdps);
    } else {
        // Failed to read a packet
    }

    return 0;
}