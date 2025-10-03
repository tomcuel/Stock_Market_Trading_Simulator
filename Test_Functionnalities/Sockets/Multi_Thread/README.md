# 🧵 Multithreaded C++ Client–Server Communication

This project demonstrates a **fully native C++ client–server system** using **TCP sockets** and **multithreading**, without external libraries.  
It includes:
- A **multi-client server** that handles connections concurrently  
- A **multi-threaded client** capable of sending multiple message sequences in parallel  
- Logging and synchronization to verify **FIFO order** and **thread consistency**
- Clean shutdown via a monitored keypress (`q`) or server disconnection detection

---

## 🧩 Architecture Overview

### **Server (`server.cpp`)**
The server:
- Listens on port **8080** for incoming TCP connections
- Spawns a **thread per client** using `std::thread` and `detach()` to allow concurrent handling
- Uses an **atomic flag** (`is_running`) to coordinate shutdown between threads
- Listens asynchronously for the **'q'** key to terminate gracefully
- Logs every message and response to `conversation_log.txt` for inspection

#### **Key Server Flow**
1. **Socket setup** — `socket()`, `bind()`, `listen()`
2. **Threaded accept loop** — `accept()` spawns a new thread for each client
3. **Threaded client handler**:
   - Parses each message (`client_number + message`)
   - Processes requests (mapping numbers 0–9)
   - Sends a structured response  
4. **Exit thread** monitors for `'q'` press and closes all sockets

#### **Thread Synchronization**
- `std::atomic<bool> is_running` ensures threads exit cleanly when the server shuts down
- Each client thread runs independently — `detach()` allows full concurrency

---

### **Client (`client.cpp`)**
The client:
- Launches **multiple concurrent threads**, each representing a separate client sequence
- Each thread sends a series of messages (`client_number + space-separated values`)
- Every message triggers a **new TCP connection**, simulating multiple client exchanges
- Logs message activity to the same `conversation_log.txt`

#### **Key Client Flow**
1. **Monitor thread** continuously checks if the server is still active
2. **Message threads** send predefined message sets concurrently:
   ```cpp
   std::vector<std::string> messages = {"1 1 2 3", "2 4 5 6", "3 7 8 9"};
   ```
- Client `1` sends values `1, 2, 3`
- Client `2` sends values `4, 5, 6`
- Client `3` sends values `7, 8, 9`
3. Each message thread:
- Opens a socket
- Sends a formatted message (`client_number + key`)
- Waits for the server response
- Prints and logs it
4. Once all threads finish, the client terminates

#### **Server Monitoring**
A separate thread (`monitor_server`) runs concurrently to:
- Periodically check if the server is still reachable
- Stop all activity and exit if the connection fails

--- 

## 🧮 Multithreading Summary
| Component	| Thread Type | Responsibility |
|------------|-----------|-----------|
|`main()` (server) | Main thread | Creates listening socket, accepts clients |
|`handle_client()` | Worker thread per client | Processes messages concurrently |
|`shutdown_server()` | Exit thread | Monitors 'q' keypress for clean stop |
|`monitor_server()` | Client watchdog | Detects server disconnection |
|`send_message()` |Client message thread | Sends multiple messages per client ID |
This design ensures:
- Full parallelism between clients
- Safe shutdown coordination
- FIFO message consistency
- Separate connection lifecycle per message

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
To stop the server, press `q` in its terminal.

💬 Example Session (order FIFO for each client)

Client side:
```yaml
Client launched...
Server detected. Enter a number between 0 and 9 (exit to quit):
Client number 3 > 7
Client number 1 > 1
Client number 2 > 4
Serveur : Response for the key 7 : 67
Client number 3 > 8
Serveur : Response for the key 4 : 56
Client number 2 > 5
Serveur : Response for the key 1 : 45
Client number 1 > 2
Serveur : Response for the key 8 : 90
Serveur : Response for the key 5 : 89
Client number 3 > 9
Client number 2 > 6
Serveur : Response for the key 2 : 78
Client number 1 > 3
Serveur : Response for the key 9 : 10
Serveur : Response for the key 6 : 34
Serveur : Response for the key 3 : 23
```
Server side:
```yaml
Server launched... (press 'q' to quit)
Server waiting for connexion on the port 8080...
Client connected!
Client connected!
Client connected!
Client 3 : 7
Client connected!
Client connected!
Client 2 : 4
Client 1 : 1
Client connected!
Client 3 : 8
Client connected!
Client 2 : 5
Client connected!
Client 1 : 2
Client connected!
Client connected!
Client 3 : 9
Client 2 : 6
Client connected!
Client 1 : 3
q
[Server] Stop asked...
```
