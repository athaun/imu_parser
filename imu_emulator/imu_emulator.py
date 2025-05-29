import os
import pty
import struct
import termios
import time
import socket
import threading
import json

PACKET_SIGNATURE = 0x7FF01CAF
PACKET_SIZE = 20  # 4 byte signature + 4 byte count + 3*4 bytes floats
TOTAL_PACKETS = 3000

sent_packets = {}
received_packets = set()
lock = threading.Lock()

def build_packet(count: int, x: float, y: float, z: float) -> bytes:
    """
    Constructs a binary packet in big-endian format
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
    Listens for UDP broadcast messages, parses and records them
    """
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(('0.0.0.0', port))

    while True:
        try:
            data, addr = sock.recvfrom(1024)
            packet = data.decode().strip()

            packet_json = json.loads(packet)
            print(f"[UDP] recv: {packet_json}")

            with lock:
                received_packets.add(packet_json["count"])
               
        except Exception as e:
            print(f"[UDP] Failed to parse packet: {e}")

def imu_emulator(pty_host):
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

            print(f"[IMU] sent: {packet_count}: X={0.1 * packet_count:.2f}, Y={0.02 * packet_count:.2f}, Z={0.01 * packet_count:.2f}")
            with lock:
                sent_packets[packet_count] = time.time()

            packet_count += 1
            time.sleep(0.01);

        with lock:
            missed = [p for p in sent_packets if p not in received_packets]
            print(f"\n--- Statistics ---")
            print(f"Total packets sent     : {TOTAL_PACKETS}")
            print(f"Total packets received : {len(received_packets)}")
            print(f"Total packets dropped  : {len(missed)}")
            print(f"Drop rate              : {len(missed) / TOTAL_PACKETS:.2%}")


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

    symlink_path = "/tmp/tty1"
    try:
        os.unlink(symlink_path)
    except FileNotFoundError:
        pass
    os.symlink(pty_device_path, symlink_path)

    print(f"[IMU] Virtual serial device created at: {pty_device_path}")
    print(f"[IMU] Symlinked to: {symlink_path}\n")

    # Launch both threads
    threading.Thread(target=udp_listener).start()
    imu_emulator(pty_host)

if __name__ == '__main__':
    main()
