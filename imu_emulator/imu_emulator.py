import os
import pty
import struct
import termios
import time
import socket
import threading
import json
import signal

PACKET_SIGNATURE = 0x7FF01CAF
PACKET_SIZE = 20  # 4 byte signature + 4 byte count + 3*4 bytes floats
TOTAL_PACKETS = 3000

sent_packets = {}
received_packets = set()
first_received_count = None

lock = threading.Lock()
shutdown_event = threading.Event()

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
    sock.settimeout(1.0)

    while not shutdown_event.is_set():
        try:
            data, addr = sock.recvfrom(1024)
            packet = data.decode().strip()
            packet_json = json.loads(packet)
            print(f"[UDP] recv: {packet_json}")
            with lock:
                received_packets.add(packet_json["count"])

                # Set the first packet count recieved back, used as a starting point for statistics
                global first_received_count
                if first_received_count is None:
                    first_received_count = packet_json["count"]

        except socket.timeout:
            continue
        except Exception as e:
            print(f"[UDP] Failed to parse packet: {e}")
    sock.close()

def imu_emulator(pty_host):
    with os.fdopen(pty_host, 'wb', buffering=0) as imu_output:
        packet_count = 0
        while not shutdown_event.is_set():
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
            time.sleep(0.001);

        

def handle_sigint(signum, frame):
    shutdown_event.set()

def print_statistics():
    with lock:
        # Build filtered_sent with only packets sent after first received
        filtered_sent = {}
        for count, timestamp in sent_packets.items():
            if count >= first_received_count:
                filtered_sent[count] = timestamp

        # Build missed packet list
        missed = []
        for count in filtered_sent:
            if count not in received_packets:
                missed.append(count)

        print("\n--- Statistics ---")
        print(f"First received count    : {first_received_count}")
        print(f"Total packets sent      : {len(filtered_sent)}")
        print(f"Total packets received  : {len(received_packets)}")
        print(f"Total packets dropped   : {len(missed)}")
        print(f"Drop rate               : {len(missed) / len(filtered_sent):.2%}")


def main():
    signal.signal(signal.SIGINT, handle_sigint)

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

    udp_thread = threading.Thread(target=udp_listener)
    imu_thread = threading.Thread(target=imu_emulator, args=(pty_host,))
    udp_thread.start()
    imu_thread.start()

    try:
        imu_thread.join()
        udp_thread.join()
    finally:
        print_statistics()


if __name__ == '__main__':
    main()
