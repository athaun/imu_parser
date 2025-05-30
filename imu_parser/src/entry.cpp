#include <iostream>
#include <csignal>

#include "broadcaster.h"
#include "parser.h"
#include "scheduler.h"

bool running = true;

void sigint_handler(int _) {
    running = false;
}

int main() {
    std::signal(SIGINT, sigint_handler);

    // Initialize the IMU parser, UDP broadcaster, and scheduler
    auto device_cfg = IMUParser::Config(B921600, "/tmp/tty1");

    Scheduler::init();
    int broadcast_init = Broadcaster::init("127.255.255.255", 9000);
    int parser_init = IMUParser::init(device_cfg);

    if (!broadcast_init || !parser_init) {
        perror("Failed to initialize IMU parser or UDP broadcast");
        return 1;
    }

    // Read data from the IMU device and broadcast it via UDP until shutdown
    std::vector<IMUParser::Packet> packets;
    while (running) {
        packets = IMUParser::read_from_device(device_cfg);

        for (auto& p : packets) {
            // Package the data into JSON and broadcast
            char message[128];
            snprintf(message, sizeof(message),
                "{ \"count\": %u, \"X\": %.3f, \"Y\": %.3f, \"Z\": %.3f }",
                p.packet_count, p.X_rate_rdps, p.Y_rate_rdps, p.Z_rate_rdps);
            Broadcaster::send(message);
        }

        Scheduler::update();
    }

    Broadcaster::cleanup();
    IMUParser::cleanup(device_cfg);

    return 0;
}
