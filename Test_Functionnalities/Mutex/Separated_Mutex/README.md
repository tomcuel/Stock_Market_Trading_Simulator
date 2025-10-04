# 🧩 Multi-Action Mutex Trading Server (C++)

This project demonstrates a **multi-threaded C++ trading server** using **per-action mutex separation** for concurrency control.  
Each action (e.g. `AAPL`, `GOOG`, `TSLA`) has its **own writer thread** and **dedicated mutex/condition variable**, allowing safe concurrent writes without global contention.

---

## ⚙️ Key Features

- **Per-action synchronization**:  
  Each stock (`action_id`) has its own queue, mutex, and condition variable (`ActionBook` struct).  
  → Only orders for the same symbol are serialized.

- **Shared state protection**:  
  A `std::shared_mutex` allows multiple readers (clients viewing data) and exclusive writers (order updates).

- **Automatic thread management**:  
  When a new stock is encountered, a dedicated writer thread is created on-the-fly.

- **Client-server communication via TCP sockets**:  
  Multiple clients can connect simultaneously, send orders, and view state.

- **Thread-safe FIFO processing**:  
  Orders are queued per symbol and processed sequentially by their writer thread.

---

## 🧠 Synchronization Model

### Mutex Hierarchy

| Component | Type | Purpose |
|------------|------|----------|
| `state_mtx` | `std::shared_mutex` | Protects access to the global `market_state` map. Allows concurrent reads (view) and exclusive writes (order execution). |
| `ActionBook::queue_mtx` | `std::mutex` | Guards the FIFO queue of pending orders for one symbol. |
| `ActionBook::cv` | `std::condition_variable` | Signals the writer thread when a new order arrives. |

### Mutex Separation Principle
Instead of one global lock serializing all actions, each ActionBook isolates its workload:
```cpp
AAPL: queue + mutex + thread
GOOG: queue + mutex + thread
TSLA: queue + mutex + thread
```
This fine-grained locking drastically improves concurrency and mimics real-world trading engines that manage order books per symbol.

---

## 🧵 Thread Overview
| Thread | Role	| Mutexes Used |
|------------|------|----------|
| `main` | Accepts new clients | none |
| `handle_client` | One per client connection | `state_mtx` (read/write) |
| `writer_thread_func(action_id)` | One per action | `queue_mtx`, `state_mtx` |

Each action runs an independent processing thread, so:
- AAPL orders don’t block GOOG orders
- Concurrent clients can view data freely

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

### Example Output
Server terminal:
```yaml
Server listening on port 8080...
Client connected!
Client: BUY 10 AAPL LIMIT 145.5 2025-10-10 14:30
[Writer-AAPL] Processed order: BUY 10 AAPL
Client: view AAPL
```
Client terminal:
```yaml
Connected to server on port 8080!
Your order was queued: BUY 10 AAPL LIMIT 145.5 2025-10-10 14:30
[Server] Order executed: BUY AAPL
[Server] State for AAPL: Price=150; BUY 10; 
```

### Data Flow

1. A client sends an order such as
```yaml
BUY 10 AAPL LIMIT 145.5 2025-10-10 14:30
``` 
2. The server:
- Parses it into an `Order` structure 
- Ensures `ActionBook["AAPL"]` exists 
- Pushes the order into `AAPL`’s queue 
- Notifies the corresponding writer thread
3. The writer thread wakes up, locks `state_mtx` exclusively, modifies the market state, and confirms to the client
