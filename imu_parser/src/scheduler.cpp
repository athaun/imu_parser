#include <ctime>
#include <sched.h>
#include <unistd.h>

#include "scheduler.h"

namespace Scheduler {
    int loop_time;
    timespec next_time;

    void init(const int loop_time_ns) {
        loop_time = loop_time_ns;
        clock_gettime(CLOCK_MONOTONIC, &next_time);
    }

    void update() {
        // Calculate next absolute time
        next_time.tv_nsec += loop_time;
        next_time.tv_sec += next_time.tv_nsec / 1'000'000'000;
        next_time.tv_nsec = next_time.tv_nsec % 1'000'000'000;

        // Sleep until the next time slot
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_time, nullptr);
    }
}