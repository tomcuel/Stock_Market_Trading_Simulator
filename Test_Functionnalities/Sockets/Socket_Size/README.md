# ⚡ C++ Server and Client for Large Data Transmissiont

This project demonstrates a C++ TCP server **sending large datasets** to a C++ client using TCP sockets, with careful handling for very large messages and partial transmissions.
It validates full-frame vs naive receive to demonstrate the need for batching or framing for multi-100MB messages. Timeouts and memory safeguards prevent hangs or crashes.

---

## ⚙️ Overview
The system architecture:
* **Server** (`server.cpp`): **generates large data buffers** (tens to hundreds of MB) and sends them over TCP
* **Client** (`client.cpp`): receives the data, computes a **lightweight hash**, and compares the naive vs full-frame transmission
* Communication supports **full-frame transmission** (with an 8-byte size header) and **naive transmission** (terminated by a marker), allowing multiple receive methods.
* Safety mechanisms include:
    - **Timeouts** for receiving data
    - **Maximum buffer limits** to avoid memory crashes
    - **Leftover handling** to process partially read data without losing alignment

---

## 🧠 Key Features and Architecture

1. **Full vs Naive Transmission**
**Full transmission**:
* The server sends the **data length first** (8-byte header, network order), followed by the full dataset.
* The client reads the header first, then **reads exactly that many bytes**, ensuring no misalignment.
**Naive transmission**:
* The server sends raw data terminated by a **marker string** (`###END_OF_NAIVE###`).
* The client reads bytes until it finds the marker.
* For very large data, naive reading can timeout or crash if the marker is never found, which demonstrates the need for batching or chunked transfers.
2. **Batch-Safe Receiving**
* Very large messages are handled in **small `BUFFER_SIZE` chunks**, avoiding memory over-allocation and allowing the client to remain responsive.
* The client tracks **leftover bytes** from one receive operation and reuses them for subsequent reads, keeping the stream aligned.
* Both full-frame and naive receives respect a **10-second maximum wait**, preventing infinite loops.
3. **Hash Verification**
* Both the naive and full-frame transmissions are hashed using `std::hash`.
* Hash comparison ensures **data integrity**.

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

## 🧪 Observed Behavior and Limitations

During testing:
```bash
[CLIENT] Connected to server
```
1. First test size (~29 MB):
```bash
[CLIENT] Full received 29722220 bytes
[CLIENT] Hash (full):  a89d4fbbb52fe15c
[CLIENT] Naive received 29722220 bytes
[CLIENT] Hash (naive): a89d4fbbb52fe15c
[CLIENT] Data matches exactly for first test size
```
Both naive and full-frame methods work fine.

2. Second test size (~307 MB):
```bash
[CLIENT] Full received 307222220 bytes
[CLIENT] Hash (full):  aab90e0630693643
[CLIENT] Error: Receive timed out waiting for marker
```
The naive receive fails because the entire buffer is too large to read safely in a single pass (hence why the time stopping condition).
This highlights the necessity of sending data in batches or using the full-frame protocol for very large messages.

On the server side:
```bash
[SERVER] Listening on port 8080...
[SERVER] Client connected
[SERVER] Generated 29722220 bytes
[SERVER] Hash: a89d4fbbb52fe15c
[SERVER] Both transmissions complete
[SERVER] Generated 307222220 bytes
[SERVER] Hash: aab90e0630693643
```
We can see that the second one has really been send well.

---

## ⚡ Lessons and Best Practices
1. **Always use framing for large messages**:
Sending an explicit size header allows the client to allocate memory correctly and read the stream reliably.
2. **Batching / chunking**:
* For extremely large datasets, **split the data into smaller blocks** instead of sending it all at once.
* This prevents crashes on the client side and avoids TCP buffer saturation.
3. **Timeouts and safety checks**:
* Limit total receive time per message.
* Limit maximum buffer allocation to prevent `std::bad_alloc`.
* Track leftover bytes for proper stream alignment.
4. **Hashing for verification**:
* Computing hashes before and after transmission ensures **data integrity** for both naive and full-frame methods.

