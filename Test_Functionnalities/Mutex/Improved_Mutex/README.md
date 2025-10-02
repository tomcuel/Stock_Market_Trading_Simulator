# 🧵 Advanced C++ Mutex Interaction — Reader/Writer Server with Write Queue

This project demonstrates a **multithreaded C++ server** using **modern synchronization primitives**:
- `std::shared_mutex` for **reader–writer access control**
- `std::mutex` + `std::condition_variable` for **coordinated write operations**
- `std::queue` for **FIFO ordering of modification requests**

Clients can connect to the server via TCP and:
- View current values (`view`)
- Queue new modifications (`modify <value>`)

The server processes write requests sequentially in a background **writer thread**, ensuring consistency while allowing **concurrent reads**.

---

## 🧩 Architecture Overview

### 🔹 Server (`server.cpp`)
- Accepts multiple concurrent client connections  
- Uses **`std::shared_mutex`** to allow:
  - Multiple simultaneous readers (`view`)
  - A single exclusive writer (`modify`)
- Uses **`std::queue` + `std::condition_variable`** to:
  - Store and process write requests in **FIFO** order
  - Wake up the writer thread when new data arrives

### 🔹 Client (`client.cpp`)
- Connects via TCP to the server on port `8080`
- Sends commands:
  - `view` → get the current list of integers
  - `modify <value>` → queue a new number to be added
  - `exit` → close the connection
- Displays server responses in real time

---

## ⚙️ Key Synchronization Mechanisms

### 1. **Shared Read Access**
Multiple clients can run `view` simultaneously:
```cpp
std::shared_lock<std::shared_mutex> rlock(rw_mtx);
```
This enables high concurrency for read-only operations.

### 2. Exclusive Write Access
When a modification is ready, only one writer holds the lock:
```cpp
std::unique_lock<std::shared_mutex> wlock(rw_mtx);
```
This ensures data consistency during updates.

### 3. Queued Modifications (FIFO)
All incoming modify requests are placed in a queue:
```cpp
{
    std::lock_guard<std::mutex> lock(queue_mtx);
    write_queue.push(newValue);
}
cv.notify_one();
```
- `queue_mtx` protects the queue from concurrent access
- `cv` signals the writer thread that work is available

### 4. Background Writer Thread
The writer runs in the background and processes queued updates one at a time:
```cpp
void writer_thread_func() {
    while (true) {
        std::unique_lock<std::mutex> lock(queue_mtx);
        cv.wait(lock, [] { return !write_queue.empty(); });

        int newValue = write_queue.front();
        write_queue.pop();
        lock.unlock();

        std::unique_lock<std::shared_mutex> wlock(rw_mtx);
        std::this_thread::sleep_for(std::chrono::seconds(2)); // simulate work
        values.push_back(newValue);
        std::cout << "[Writer] Value added: " << newValue << std::endl;
    }
}
```
This guarantees:
- **Strict FIFO order** of writes
- **Non-blocking** client responses (they return immediately after queuing)

---

## 🧠 Design Advantages
| Mechanism	| Purpose | Benefit |
|------------|-----------|-----------|
| `std::shared_mutex` | Allows concurrent reads and exclusive writes | Maximizes read performance |
| `std::mutex + std::condition_variable` |Synchronizes queue access | Prevents busy waiting |
| `std::queue` | FIFO storage of pending modifications | Ensures order consistency |
| Background thread	| Handles all write operations | Prevents blocking client threads |
| Thread per client | Handles each connection concurrently | Scales easily to many clients |

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

---

## 🔄 Typical Client Interaction
Client 1:
```yaml
Command (modify <value> / view / exit) > modify 100
Server: Your modify request was queued: 100
```
Client 2:
```yaml
Command (modify <value> / view / exit) > modify 200
Server: Your modify request was queued: 200
```
Meanwhile, the server console shows:
```yaml
[Writer] Value added: 100
[Writer] Value added: 200
```
Client 3:
```yaml
Command (modify <value> / view / exit) > view
Server: Values: 10 20 30 40 50 100 200
```
