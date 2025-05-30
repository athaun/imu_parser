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
PAUSE_TIME = 0.0005

sent_packets = {}
received_packets = set()
first_received_count = None

lock = threading.Lock()

start_time = None
end_time = None

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

    # Create a UDP socket and listen on all incoming interfaces
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(('0.0.0.0', port))
    sock.settimeout(1.0)

    while not shutdown_event.is_set():
        try:
            # Receive data from the socket, 
            data, addr = sock.recvfrom(128)
            packet = json.loads(data.decode())

            print(f"[UDP] recv: {packet}")
            with lock:
                received_packets.add(packet["count"])

                # Set the first packet count recieved back, used as a starting point for statistics
                global first_received_count, start_time
                if first_received_count is None:
                    start_time = time.time()
                    first_received_count = packet["count"]

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
            time.sleep(PAUSE_TIME);  

def handle_sigint(signum, frame):
    shutdown_event.set()

def print_statistics():
    with lock:
        # Build filtered_sent with only packets sent after first packet was received
        filtered_sent = {}
        for count, timestamp in sent_packets.items():
            if count >= first_received_count:
                filtered_sent[count] = timestamp

        # Build missed packet list
        missed = []
        for count in filtered_sent:
            if count not in received_packets:
                missed.append(count)

        duration = end_time - start_time

        print("\n--- Statistics ---")

        print(f"Runtime (s)             : {duration:.2f}")
        print(f"Avg sending rate (Hz)   : {len(filtered_sent) / duration:.2f}")
        print(f"Avg receive rate (Hz)   : {len(received_packets) / duration:.2f}")
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

    # Create a symlink to the pty device for consistent access
    symlink_path = "/tmp/tty1"
    try:
        os.unlink(symlink_path)
    except FileNotFoundError:
        pass
    os.symlink(pty_device_path, symlink_path)

    print(f"[IMU] Virtual serial device created at: {pty_device_path}")
    print(f"[IMU] Symlinked to: {symlink_path}\n")

    # Start UDP listener and IMU emulator threads
    udp_thread = threading.Thread(target=udp_listener)
    imu_thread = threading.Thread(target=imu_emulator, args=(pty_host,))
    udp_thread.start()
    imu_thread.start()

    # Finish threads then print statistics 
    try:
        imu_thread.join()
        udp_thread.join()
    finally:
        global end_time
        end_time = time.time()
        print_statistics()


if __name__ == '__main__':
    main()
