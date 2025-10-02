# 🧩 More advanced Socket Communication in C++

This project demonstrates a **client-server communication** system using C++ sockets (`<arpa/inet.h>`) with added **multithreading**, **file logging**, and **graceful shutdown** features, but without (for now / here) **mutex locks** to ensure thread-safe operations. 

---

## ⚙️ Features

### 🖥️ Server (`server.cpp`)
- Accepts multiple client connections via threads
- Each client is handled in a **detached thread** using `std::thread`  
- Uses an **atomic flag (`is_running`)** to safely coordinate shutdown between threads 
- Supports **graceful shutdown** by pressing the `'q'` key
- Logs all messages (server, client, system) in a text file: `conversation_log.txt` 
- Maintains a **map of integer key-value pairs** that clients can query (keys 0–9)
- Clears and reinitializes the log file at startup.

### 💬 Client (`client.cpp`)
- Connects to the server and sends messages (numbers between 0–9)
- Continuously monitors the server status in a background thread, the client exits automatically if the server stops  
- Logs connection events and messages to the same file (`conversation_log.txt`)
- Graceful exit using the `"exit"` command  

---

## 🧵 Multithreading & Synchronization

The system uses several **independent threads**:
1. **Server main thread:** Accepts new connections  
2. **Client threads:** Each client connection runs in its own thread via `handle_client()`  
3. **Shutdown thread:** Monitors `'q'` key input to safely stop the server  
4. **Client monitor thread:** Continuously checks if the server is still active  

Synchronization between threads is achieved using an **atomic boolean**:
```cpp
std::atomic<bool> is_running(true);
```
This ensures all threads can detect when the system is shutting down and exit cleanly, avoiding race conditions.

---

## 📂 Logging
All interactions (client messages, server responses, and system events) are written to `conversation_log.txt`.

Example:
```yaml
SYSTEM: Server launched
Client: 2
Server: Response for the key 2 : 78
SYSTEM: Server stopped
```

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

Client side:
```yaml
Client launched...
Serveur detected. Enter a number between 0 and 9 (exit to quit):
Client > 5
Serveur : Response for the key 5 : 89
Client > exit
Client deconnected!
```
Server side:
```yaml
Server launched... (Press 'q' to quit)
Server waiting for connexion on the port 8080...
Client connected!
Client: 5
Server: Response for the key 5 : 89
[Server] Stop asked...
```
