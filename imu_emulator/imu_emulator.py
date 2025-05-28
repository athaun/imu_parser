import os
import pty
import struct
import termios
import time

PACKET_SIGNATURE = 0x7FF01CAF
PACKET_SIZE = 20 # 4 byte signature + 4 byte count + 3*4 bytes floats

def build_packet(count: int, x: float, y: float, z: float) -> bytes:
    """
    Constructs a binary packet in big-endian format.
    Layout: [signature][count][x][y][z]
    """
    packet = struct.pack('>I', PACKET_SIGNATURE)
    packet += struct.pack('>I', count)
    packet += struct.pack('>f', x)
    packet += struct.pack('>f', y)
    packet += struct.pack('>f', z)
    return packet

def log_packet(slave_name: str, index: int, data: bytes):
    hex_data = " ".join(f"{b:02X}" for b in data)
    print(f"{slave_name} - {index} | {hex_data}")

def main():
    # Create a pseudoterminal pair 
    pty_host, pty_device = pty.openpty()
    pty_device_path = os.ttyname(pty_device)

    # Act like a raw serial port (no control characters or processing)
    attrs = termios.tcgetattr(pty_device)
    attrs[0] = 0            # iflag
    attrs[1] = 0            # oflag
    attrs[2] |= termios.CS8 # cflag: 8-bit characters
    attrs[3] = 0            # lflag
    termios.tcsetattr(pty_device, termios.TCSANOW, attrs)

    print(f"Virtual serial device: {pty_device_path}")

    # Open the host end for writing binary data
    with os.fdopen(pty_host, 'wb', buffering=0) as imu_output:
        packet_count = 0

        while True:
            packet = build_packet(
                count=packet_count,
                x=0.1 * packet_count,
                y=0.02 * packet_count,
                z=0.01 * packet_count
            )

            imu_output.write(packet)
            imu_output.flush()

            # log_packet(pty_device_path, packet_count, packet)
            packet_count = (packet_count + 1) % 1000
            time.sleep(0.08)

if __name__ == '__main__':
    main()
