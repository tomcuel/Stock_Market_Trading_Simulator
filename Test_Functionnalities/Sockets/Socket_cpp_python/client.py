import socket

HOST = "127.0.0.1"  # server address
PORT = 8080         # server port
BUFFER_SIZE = 1024

def main():
    # create TCP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    try:
        # connect to server
        sock.connect((HOST, PORT))
        print("Connected to the server!")

        while True:
            # get input from user
            message = input("Client > ")

            # send message
            sock.sendall(message.encode())

            # exit condition
            if message == "exit":
                break

            # receive response
            response = sock.recv(BUFFER_SIZE).decode()
            if not response:
                print("Connection closed by the server")
                break

            print("Server:", response)

    except Exception as e:
        print("Error:", e)

    finally:
        sock.close()

if __name__ == "__main__":
    main()
