import socket
import threading

def listen_for_messages(client_socket):
    while True:
        try:
            response = client_socket.recv(1024)
            if not response:
                print("Server closed connection.")
                break
            data = {"dists": [], "time": []}
            for i in range(0, len(response), 8):
                data["dists"].append(int.from_bytes(response[i:i+4], signed=True, byteorder='little'))
                data["time"].append(int.from_bytes(response[i+4:i+8], signed=True, byteorder='little'))
            print(data)
        except Exception as e:
            print("Error receiving message: ", str(e))
            break

def main():
    # Define server address and port
    server_address = '172.20.10.5'  # Replace with the IP address of your ESP32
    server_port = 8080

    # Create a TCP/IP socket
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    try:
        # Connect to the server
        client_socket.connect((server_address, server_port))
        print("Connected to server at {}:{}".format(server_address, server_port))

        # Start a new thread to listen for messages from the server
        threading.Thread(target=listen_for_messages, args=(client_socket,)).start()

        while True:
            # Get user input
            message = input()

            # Send message to the server
            client_socket.sendall(message.encode('utf-8'))

            # Break the loop if the user types '/quit'
            if message == '/quit':
                print("Disconnecting from server...")
                break
    
    except Exception as e:
        print("Error: ", str(e))
    
    finally:
        # Close the socket connection
        client_socket.close()
        print("Connection closed.")

if __name__ == "__main__":
    main()
