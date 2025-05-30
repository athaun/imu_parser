#pragma once

namespace Scheduler {
    void init(const int loop_time_ns = 80'000'000);

    void update();
}