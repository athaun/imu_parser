#include <iostream>
#include <time.h>
#include <sched.h>
#include <unistd.h>
#include <sstream>

#include "broadcaster.h"
#include "parser.h"

int main() {
    constexpr int LOOP_TIME_NS = 80'000'000; // 80ms

    // Set the scheduler priority near the top (80 out of 1-99)
    // This requires sudo but it will still run without it.
    sched_param sch_params;
    sch_params.sched_priority = 80;
    if (sched_setscheduler(0, SCHED_FIFO, &sch_params) == -1) {
        perror("sched_setscheduler failed");
    }

    timespec next_time;
    clock_gettime(CLOCK_MONOTONIC, &next_time);

    if (!Broadcaster::init("127.255.255.255", 9000)) {
        return 1;
    }

    auto device_cfg = IMUParser::Config(B921600, "/tmp/tty1");
    if (!IMUParser::init(device_cfg)) {
        return 1;
    }

    while (true) {
        
        auto packets = IMUParser::read_from_device(device_cfg);

        for (auto& p : packets) {
            char message[128];
            snprintf(message, sizeof(message),
                "{ \"count\": %u, \"X\": %.3f, \"Y\": %.3f, \"Z\": %.3f }",
                p.packet_count, p.X_rate_rdps, p.Y_rate_rdps, p.Z_rate_rdps);
            Broadcaster::send(message);
        }

        // Calculate next absolute time
        next_time.tv_nsec += LOOP_TIME_NS;
        next_time.tv_sec += next_time.tv_nsec / 1'000'000'000;
        next_time.tv_nsec = next_time.tv_nsec % 1'000'000'000;

        // Sleep until the next 80ms slot
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_time, nullptr);
    }

    Broadcaster::cleanup();
    IMUParser::cleanup(device_cfg);

    return 0;
}
