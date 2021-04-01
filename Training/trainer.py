import socket
import threading
import traceback

client = None


def add_input():
    while True:
        inp = input()
        try:
            b = bytes(int(c, 16) for c in inp.split())
            print("Sending", b)
            client.send(b)
        except:
            print("Cannot send", inp)
            traceback.print_exc()


sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM, socket.IPPROTO_TCP)
sock.bind(("127.0.0.1", 12345))
sock.listen(5)
while True:
    client, remote = sock.accept()
    print(remote, "connected")
    try:
        result = False
        input_thread = threading.Thread(target=add_input)
        input_thread.daemon = True
        input_thread.start()

        while True:
            byte = client.recv(1000)
            if not byte:
                break
            if byte[0] == 8:
                result = not result
                if result:
                    client.send(b'\x09\0\0\0\0')
                else:
                    client.send(b'\x09\4\0\0\0')
            else:
                print("Received", byte)
    except:
        traceback.print_exc()
