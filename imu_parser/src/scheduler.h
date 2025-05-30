#pragma once

namespace Scheduler {
    /**
     * @brief Initializes the scheduler with a specified loop time in nanoseconds.
     * @param loop_time_ns The desired loop time in nanoseconds. Default is 80,000,000 ns (80 ms).
     */
    void init(const int loop_time_ns = 80'000'000);

    /**
     * @brief Updates the scheduler, blocking until the next time slot.
     * This function should be called at the end of a loop to maintain the desired timing.
     */
    void wait();
}