# 🔌 First Socket Programming Test in C++

This project demonstrates a **basic TCP client-server communication** setup in **C++** using the **POSIX socket API** (`<sys/socket.h>`, `<arpa/inet.h>`, `<unistd.h>`).

It consists of two files:
- 🖥️ `server.cpp` — the TCP server  
- 💻 `client.cpp` — the TCP client  

---

## ⚙️ Overview

The server waits for a connection on **port 8080**, accepts a client, and then enters a loop:
- Reads messages sent by the client
- Displays them on the console
- Sends responses back to the client

The client connects to the server on **localhost (127.0.0.1)**, sends messages, and displays server replies.

Communication continues until the client sends the command `exit`.

⚠️ Notes
- Communication is synchronous and blocking (both sides wait for each other)
- The code handles only one client connection at a time

---

## 🧩 Program Flow

### Server (`server.cpp`)

1. Creates a TCP socket  
2. Binds it to port **8080**  
3. Listens for an incoming connection  
4. Accepts the first client  
5. Enters a message loop:
   - Reads client messages  
   - Prints them to the console  
   - Sends a manual response (entered by the server user)
6. Closes the connection when the client disconnects or sends no data

### Client (`client.cpp`)

1. Creates a TCP socket  
2. Connects to `127.0.0.1:8080`  
3. Enters a message loop:
   - Sends user input to the server  
   - Waits for and displays the server’s response  
4. Terminates when the user types `exit`

---

## 🛠️ Compilation

Compile both programs using:
```bash
g++ -std=c++17 -Wall -Wfatal-errors server.cpp -o server.x
g++ -std=c++17 -Wall -Wfatal-errors client.cpp -o client.x
```

---

## ▶️ Usage

Start the server in one terminal:
```bash
./server.x
```
Start the client in another terminal:
```bash
./client.x
```
You can now type messages in the client window, and respond to them in the server window.

💬 Example Session

Client terminal:
```yaml
Connected to the server!
Client > Hello server!
Server: Hello client!
Client > exit
```
Server terminal:
```yaml
Server waiting for connexion on the port 8080...
Client connected!
Client: Hello server!
Server > Hello client!
Client deconnected
```
