# ⚙️ Mutex-Concurrent Trading Engine — C++ Server + Python Stress Test

This project implements a **multi-client trading engine** in **C++**, designed to test **mutex efficiency and correctness** under high concurrency.  
It includes a **Python stress-testing script** that launches many clients sending random buy/sell orders simultaneously, all interacting with a shared order book.

---

## 🧩 Overview

This setup demonstrates how to safely coordinate **concurrent reads and writes** to a live trading engine using:
- `std::shared_mutex` for **per-symbol order books** (multiple readers, one writer)
- `std::mutex` for **individual client portfolios** (strict ownership)
- A **per-client thread model** for communication and processing
- A **Python load test** that stresses the server with random orders to check for race conditions, deadlocks, or inconsistent trades

---

## 🧠 Architecture

### 🔹 Core Components

| Component | Description |
|------------|--------------|
| **Server (C++)** | Handles multiple TCP clients, processes buy/sell orders, matches trades, and keeps order books consistent using mutexes |
| **Client (Python threads)** | Each simulated trader connects, sends random orders, and receives trade confirmations |
| **Mutex System** | Fine-grained locking ensures data integrity between concurrent operations |
| **Matching Engine** | Processes and matches limit orders in FIFO-like priority queues |

---

## 🧱 Mutex Model

| Mutex | Type | Scope | Protects |
|--------|------|--------|-----------|
| `market_mutexes[symbol]` | `std::shared_mutex` | Per-symbol | Each order book (buy/sell queues) |
| `portfolio_mutexes[client_id]` | `std::mutex` | Per-client | Portfolio cash & holdings |
| `client_sockets_mutex` | `std::mutex` | Global | Connected clients table |

**Locking Rules**
- `std::shared_lock` is used for **viewing** market data (`VIEW MARKET`), multiple readers, market snapshots can be accessed in parallel
- `std::unique_lock` is used for **modifying** order books (`BUY` / `SELL`), one order at a time modifies a symbol’s order book, there is no data races, all shared resources are protected by appropriate mutexes
- Portfolio updates (cash, holdings) are isolated by `std::lock_guard` on the client’s own mutex, it never blocks other clients

This ensures:
- **No deadlocks:** Lock order is consistent (market first, portfolio next)
- **High concurrency:** Different symbols and clients can operate in parallel

---

## 💰 Matching Engine Logic

1. **Order submission**  
   Clients send commands like:
   ```yaml
   BUY 10 AAPL LIMIT 145.5
   SELL 5 AAPL LIMIT 144.0
   ```
2. **Order matching**
- BUY orders match lowest-priced SELLs
- SELL orders match highest-priced BUYs
- Matching continues until no compatible orders remain
3. **Trade execution**
- Updates buyer/seller portfolios atomically under their mutexes
- Sends [TRADE] confirmation to both clients
4. **Unmatched leftovers**
- Are stored back in the order book for later matching

---

## 🧮 Internal Data Structures

### Internal Data Structures

Global registry of all order books, one per traded symbol (e.g: market["AAPL"] gives access to AAPL's order book)
```cpp
std::map<std::string, OrderBook> market;
```
Each symbol (e.g., "AAPL") has its own order book.
- Buy orders: priority queue sorted by descending price (best bids first)
- Sell orders: priority queue sorted by ascending price (best asks first)
```cpp
struct OrderBook {
    std::priority_queue<Order, std::vector<Order>, OrderCompareBuy> buyOrders;
    std::priority_queue<Order, std::vector<Order>, OrderCompareSell> sellOrders;
};
```
Each client owns a Portfolio structure storing cash balance and holdings (symbol → shares)
```cpp
std::unordered_map<int, Portfolio> portfolios;
```

### Synchronization Layer
1. `market_mutexes[symbol]` protects order books for a given symbol → `std::shared_mutex` allows concurrent readers (for VIEW operations) but exclusive write access for order matching or order insertion
```cpp
std::unordered_map<std::string, std::shared_mutex> market_mutexes; // symbol-level locks
```
2. `portfolio_mutexes[client_id]` ensures thread-safe portfolio updates → standard std::mutex since portfolios are modified frequently but rarely read in parallel by multiple threads.
```cpp
std::unordered_map<int, std::mutex> portfolio_mutexes;             // per-client locks
```
3. `client_sockets_mutex` guards the socket registry to prevent race conditions during client connection / disconnection
```cpp
std::unordered_map<int, int> client_sockets;                       // client_id → socket descriptor
std::mutex client_sockets_mutex;                                   // protects client_sockets map
```

### Concurrency Summary
- Matching Engine threads (one per client) call `process_order()` under a `unique_lock<shared_mutex>` for the symbol in question. This ensures exclusive access when modifying the order book
- Read-only operations such as `VIEW MARKET` acquire `shared_lock<shared_mutex>` allowing multiple clients to read concurrently
- Portfolio updates use `lock_guard<std::mutex>` per client to ensure atomic modification of balances and holdings
- Network socket operations use client_sockets_mutex to safely add, remove, or send messages across multiple client threads

---

## 🧩 Server Commands
| Command | Description |
|--------|--------|
| `BUY <qty> <symbol> LIMIT <price>` | Submit buy order |
| `SELL <qty> <symbol> LIMIT <price>` | Submit sell order |
| `VIEW MARKET <symbol>` | View buy/sell queues |
| `VIEW PORTFOLIO` | View current cash & holdings |
| `exit` | Disconnect client |

--- 

## 🔄 Example Session
Client 1:
```yaml
BUY 10 AAPL LIMIT 150
[ORDER] client 1 -> BUY 10 AAPL LIMIT 150
[TRADE] 1 bought 10 AAPL from 2 @ 149
```
Client 2:
```yaml
SELL 10 AAPL LIMIT 149
[ORDER] client 2 -> SELL 10 AAPL LIMIT 149
[TRADE] 1 bought 10 AAPL from 2 @ 149
```
Server logs:
```yaml
[TRADE] 1 bought 10 AAPL from 2 @ 149
Client 1 connected!
Client 2 connected!
```

---

## 🧪 Stress Testing the Mutex System
The Python script `mutex_concurrency_test.py` spawns multiple simulated clients that hammer the server with random orders.

### Run the stress test
```bash 
./launcher.sh
```
It does include the build of the `server.x` file with the `makefile`, launch `mutex_concurrency_test.py` and the cleaning afterward.

### Configurable Parameters
Inside `mutex_concurrency_test.py`:
```py
CLIENT_COUNT = 10           # number of concurrent clients
TEST_DURATION = 1           # seconds each client runs
HIGH_CONTENTION_SYMBOL = "AAPL"
```

### Log Outputs
After the test, all messages (client logs, orders, and trade events) are saved to `nrt_stress_test_log.txt`
```yaml
Launching trading server...
Starting 10 clients...

[Client 4]: [ORDER] client 3 -> SELL 9 AAPL LIMIT 154.3
[Client 1]: Portfolio (cash=10000):
[Client 5]: [ORDER] client 5 -> SELL 50 AAPL LIMIT 124.87
[Client 3]: [ORDER] client 4 -> BUY 49 AAPL LIMIT 115.47
[Client 2]: Portfolio (cash=10000):
[Client 6]: [ORDER] client 6 -> SELL 19 AAPL LIMIT 120.58
[Client 8]: [TRADE] 8 bought 10 AAPL from 6 @ 120.58
[Client 10]: Portfolio (cash=10000):

...

[TRADE] 6 bought 2 AAPL from 10 @ 141.13
[ORDER] client 10 -> SELL 43 AAPL LIMIT 113.07

All clients finished. Stopping server...

NRT mutex stress test complete!
Total orders processed: 59
1: Client 3 SELL 9 AAPL LIMIT @ 154.3
2: Client 5 SELL 50 AAPL LIMIT @ 124.87
3: Client 4 BUY 49 AAPL LIMIT @ 115.47

... 

58: Client 6 BUY 49 AAPL LIMIT @ 111.54
59: Client 10 SELL 43 AAPL LIMIT @ 113.07

Total trades executed: 39
1: Buyer 8 bought 10 AAPL from Seller 6 @ 120.58
2: Buyer 4 bought 40 AAPL from Seller 9 @ 115.47

...

38: Buyer 6 bought 2 AAPL from Seller 10 @ 141.13
39: Buyer 5 bought 5 AAPL from Seller 10 @ 147.9
```
