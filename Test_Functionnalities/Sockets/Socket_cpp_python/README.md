# 🌍 Python ↔️ C++ Socket Communication

This project demonstrates **cross-language communication** between a **C++ TCP server** and a **Python TCP client** using sockets.  
It shows how two programs written in different languages can exchange data seamlessly over a network connection.

---

## ⚙️ Overview

| Component | Language | Role |
|------------|-----------|------|
| `server.cpp` | C++ | Listens for incoming connections and exchanges text messages with the client |
| `client.py` | Python | Connects to the C++ server and interacts through user input |

This setup illustrates **interoperability** between Python and C++ via **standard TCP/IP sockets**.

---

## 🧩 Cross-Language Communication

C++ and Python can communicate natively through **sockets**, since both use the same underlying OS networking APIs (Berkeley sockets).  
This makes it possible to:
- Send and receive strings or binary data between both programs
- Develop high-performance systems (C++ side) with flexible scripting and UI layers (Python side)

In this project:
- The **C++ server** manages the socket connection and waits for messages
- The **Python client** sends user input to the server and displays responses

Both programs use the same protocol:  
**plain text messages** terminated by standard network buffering rules.

---

## 🧠 Why Mix Python and C++?
| C++ | Python |
|------------|-----------|
| High performance & low latency | Rapid development & ease of scripting |
| Precise memory and thread control | Rich standard library and ecosystem |
| Ideal for backend computation or simulation | Ideal for user interfaces or testing |

Combining both:
- Python can act as a frontend (interface, AI module, data visualization)
- C++ acts as a backend (processing engine, real-time server).
- Will be used in the stock marker with C++ core logic and Python analytics tools.
- AI frameworks where Python calls optimized C++ code.

---

## 🖥️ Server (C++)

### File: `server.cpp`

Key steps:
1. Create and configure a socket using `<arpa/inet.h>`
2. Bind it to `PORT = 8080` and start listening
3. Accept a single connection
4. Continuously:
   - Read client messages
   - Display them on the console
   - Send a manual response typed by the server user

Example output:
```yaml
Server waiting for connexion on the port 8080...
Client connected!
Client: hello
Server > Hi there!
Client: exit
Client deconnected
``` 
Compile with:
```bash
g++ -std=c++17 -Wall -Wfatal-errors server.cpp -o server.x
```
Run with:
```bash
./server.x
```

## 🐍 Client (Python)

### File: client.py
The Python client uses the built-in `socket` library to:
1. Connect to 127.0.0.1:8080
2. Read user input
3. Send messages encoded as UTF-8
4. Receive and print the server’s responses

Example session:
```yaml
Connected to the server!
Client > Hello
Server: Hi there!
Client > exit
```yaml
Run the client with:
```bash
python3 client.py
```