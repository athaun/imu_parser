#include <iostream>
#include "parser.h"

int main() {
    auto device_cfg = IMUParser::Config(921600, "/dev/tty1");
    auto packet = IMUParser::read_from_device(device_cfg);

    return 0;
}