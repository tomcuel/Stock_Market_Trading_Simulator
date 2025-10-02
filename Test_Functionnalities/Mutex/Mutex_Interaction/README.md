# 🧵 C++ Server–Client Mutex Interaction

This project demonstrates a **simple multithreaded C++ server** that synchronizes access to shared data using a **mutex (`std::mutex`)**.  
It also includes a **client** that connects via TCP sockets to send commands such as viewing or modifying shared data.

---

## ⚙️ Features

### 🖥️ `server.cpp`
- Listens for multiple client connections on **port 8080**
- Each client runs in its **own thread** and can:
  - View shared values (`view`)
  - Modify the shared list (`modify <number>`)
- A **mutex (`mtx`)** ensures thread-safe access to the shared data
- The flag `modifying` prevents concurrent modifications, ensuring one operation at a time
```cpp
std::vector<int> values = {10, 20, 30, 40, 50};
std::mutex mtx;
bool modifying = false;
```
- `values`: shared list accessible to all clients
- `mtx`: used to prevent data races during modification
- `modifying`: boolean flag indicating if a modification is currently in progress
When a client sends:
- `view` → the server returns the current list of values
- `modify <value>` → if no other modification is happening, the value is added after a **10-second delay**, simulating a long critical section

### 💻 client.cpp
The client connects to the server and allows interactive commands:
```yaml
Command (modify <value> / view / exit) >
```
- view — shows the shared list from the server
- modify 99 — adds a new value (if no one else is modifying)
- exit — closes the client connection
Example interaction:
```yaml
Command (modify <value> / view / exit) > view
Server: Liste of values: 10 20 30 40 50
```
```yaml
Command (modify <value> / view / exit) > modify 60
Server: Value added: 60
```
If another client sends a modify command during that 10-second delay:
```yaml
Server: Modification happening, try again later!
```

---

## 🔧 Mutex and Synchronization
```cpp
if (!modifying) {
    modifying = true;
    mtx.lock();
    std::this_thread::sleep_for(std::chrono::seconds(10));
    values.push_back(newValue);
    mtx.unlock();
    modifying = false;
}
```
- The mutex ensures that only one thread modifies `values` at a time.
- The 10-second delay highlights how concurrent clients must **wait their turn** to modify the data.
- If another client tries to modify during that time, it receives:
```yaml
Modification happening, try again later!
```

## 🧩 Key Concepts
| Concept | Description |
|------------|-----------|
| `std::mutex` | Synchronizes access to shared data between threads
| `std::lock_guard` or `lock()` / `unlock()` | Prevents concurrent modifications
| `std::thread` | Enables concurrent client handling
| Socket communication | Handles TCP message exchange
| Blocking section (`sleep_for`) | Simulates long modification time
| Graceful concurrency | Ensures FIFO-like fairness for client operations

---

## 🛠️ Compilation

Compile both programs using:
```bash
g++ -std=c++17 -Wall -Wfatal-errors server.cpp -o server.x
g++ -std=c++17 -Wall -Wfatal-errors client.cpp -o client.x
```

---

## 🔄 Concurrency Demonstration

1. Run the server:
```bash
./server.x
```
Output:
```yaml
Server waiting for connexion on the port 8080...
```

2. Open two terminals and run the client in each:
```bash
./client.x
```
In Client 1:
```yaml
Command (modify <value> / view / exit) > modify 99
```
(Waits for 10 seconds, then adds the value.)
In Client 2 (during that time):
```yaml
Command (modify <value> / view / exit) > modify 77
Server: Modification happening, try again later!
```
After Client 1 finishes, Client 2 can try again:
```yaml
Command (modify <value> / view / exit) > modify 77
Server: Value added: 77
```
