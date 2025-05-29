import os
import pty
import struct
import termios
import time
import socket
import threading

PACKET_SIGNATURE = 0x7FF01CAF
PACKET_SIZE = 20  # 4 byte signature + 4 byte count + 3*4 bytes floats

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

def udp_listener(port=9000):
    """
    Listens for UDP broadcast messages and prints them.
    """
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(('0.0.0.0', port))

    print(f"[UDP] Listening on port {port}...\n")
    while True:
        data, addr = sock.recvfrom(1024)
        print(f"[UDP] Received: {data.decode().strip()}")

def imu_emulator(pty_host, packet_interval=0.08):
    """
    Sends binary packets to the pseudoterminal.
    """
    with os.fdopen(pty_host, 'wb', buffering=0) as imu_output:
        packet_count = 0
        while True:
            packet = build_packet(
                count = packet_count,
                x = 0.1 * packet_count,
                y = 0.02 * packet_count,
                z = 0.01 * packet_count
            )

            imu_output.write(packet)
            imu_output.flush()

            print(f"[IMU] Sent packet {packet_count}: X={0.1 * packet_count:.2f}, Y={0.02 * packet_count:.2f}, Z={0.01 * packet_count:.2f}")
            packet_count = (packet_count + 1) % 1000
            time.sleep(packet_interval)

def main():
    # Create a pseudoterminal pair
    pty_host, pty_device = pty.openpty()
    pty_device_path = os.ttyname(pty_device)

    # Configure as raw serial port
    attrs = termios.tcgetattr(pty_device)
    attrs[0] = 0            # iflag
    attrs[1] = 0            # oflag
    attrs[2] |= termios.CS8 # cflag: 8-bit chars
    attrs[3] = 0            # lflag
    termios.tcsetattr(pty_device, termios.TCSANOW, attrs)

    print(f"[IMU] Virtual serial device created at: {pty_device_path}\n")

    # Launch both threads
    threading.Thread(target=udp_listener).start()
    imu_emulator(pty_host)

if __name__ == '__main__':
    main()
