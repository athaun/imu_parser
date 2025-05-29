import socket

PORT = 9000
BUFFER_SIZE = 1024

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(('0.0.0.0', PORT))

print(f"Listening on UDP port {PORT}...")

while True:
    data, addr = sock.recvfrom(BUFFER_SIZE)
    print(f"Received from {addr}: {data.decode()}")