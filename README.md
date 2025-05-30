## IMU Serial Parser and Emulator

This project consists of two main components: an IMU Emulator and an IMU Parser. The emulator simulates an Inertial Measurement Unit (IMU) and transmits phaux data over a virtual serial port. The parser reads the serial data, extracts the packet fields, and broadcasts it as JSON over UDP.

---

### Running the IMU emulator
From the repository root
> NOTE: Ensure the emulator is running before the parser, and that it is killed first. If it is not killed first, it will hang.
```
cd imu_emulator
python3 imu_emulator.py
```

### Running the parser
From the repository root
```
cd imu_parser
./build.sh [run|test|help]
```
you may have to make build.sh executable using `chmod +x ./build.sh` if it fails to run.

---

### Features:

IMU Emulator:
- Simulates IMU output on a virtual serial port (/tmp/tty1)
- Sends binary packets including signature, counter, and XYZ gyro data
- Listens for UDP broadcasts on port 9000
- Logs packet statistics and reception rates

IMU Parser:
- Reads binary data from /tmp/tty1
- Parses packets into structured data
- Broadcasts data as JSON over UDP to 127.255.255.255:9000

---

### Dependencies:

IMU Emulator:
- Python 3.x
- Standard Python library

IMU Parser:
- C++17 compiler or higher
- CMake 3.10 or higher
- POSIX libraries: unistd.h, termios.h, etc.
- Linux (for non-standard baud rate)
- Google Test 
