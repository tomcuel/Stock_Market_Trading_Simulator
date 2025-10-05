# 🚀 Client–Server Launcher System (C++)

This module demonstrates a **multi-client communication system** using **TCP sockets** in C++.  
It includes:
- a **server** that handles multiple clients concurrently
- a **client** that sends requests and receives responses
- and a **launcher** that spawns multiple clients automatically for testing

---

## 🧠 Overview

### 🖥️ `server.cpp`
A multithreaded TCP server that:
- Listens on port `8080`
- Handles multiple clients simultaneously using detached threads
- Logs all interactions in `conversation_log.txt`
- Maps integer requests (0–9) to predefined values  
- Stops gracefully when **‘q’** is pressed in the terminal

**Features:**
- Thread-safe shutdown mechanism via `std::atomic<bool>`
- Dynamic client handling with `std::thread`
- File logging for both client and server messages
- Keypress detection without pressing Enter (on Unix/macOS)

---

### 💬 `client.cpp`
A simple TCP client that:
- Connects to the server on `127.0.0.1:8080`
- Sends multiple messages passed via command line arguments
- Receives and displays responses from the server
- Monitors the server’s availability — exits automatically if it disconnects
- Logs all messages locally

**Example:**
```bash
./client.x 1 4 5 6
```
This runs a client with ID `1` sending the numbers `4`, `5`, and `6`.

### ⚙️ launcher.cpp
A **parallel launcher** that:
- Spawns multiple clients in separate threads
- Each client sends its own predefined sequence of numbers to the server
Example code snippet:
```cpp
std::vector<std::string> messages = {"1 1 2 3", "2 4 5 6", "3 7 8 9"};
```
This launches three clients concurrently.


## 🛠️ Compilation

Compile the three programs using:
```bash
g++ -std=c++17 -Wall -Wfatal-errors server.cpp -o server.x
g++ -std=c++17 -Wall -Wfatal-errors client.cpp -o client.x
g++ -std=c++17 -Wall -Wfatal-errors launcher.cpp -o launcher.x
```

---

## ▶️ Usage

1. Start the server
```bash
./server.x
```
You’ll see:
```yaml
Serveur démarré... (Appuyez sur 'q' pour quitter)
Serveur en attente de connexion sur le port 8080...
```
2. Launch clients (in another terminal)
You can run them manually:
```bash
./client.x 1 1 2 3
./client.x 2 4 5 6
./client.x 3 7 8 9
```
Or use the launcher to start all clients automatically:
```bash
./launcher.x
```
3. Stop the server
Press `q` in the terminal where the server is running.

💬 Example Launcher Session (order is respected)

```yaml
Client démarré...
Serveur détecté. Entrez un nombre entre 0 et 9 (exit pour quitter) :
Client number 1 > 1
Serveur : Réponse pour la clé 1 : 45
Client number 1 > 2
Serveur : Réponse pour la clé 2 : 78
Client number 1 > 3
Serveur : Réponse pour la clé 3 : 23
Client démarré...
Serveur détecté. Entrez un nombre entre 0 et 9 (exit pour quitter) :
Client number 3 > 7
Serveur : Réponse pour la clé 7 : 67
Client number 3 > 8
Serveur : Réponse pour la clé 8 : 90
Client number 3 > 9
Serveur : Réponse pour la clé 9 : 10
Client démarré...
Serveur détecté. Entrez un nombre entre 0 et 9 (exit pour quitter) :
Client number 2 > 4
Serveur : Réponse pour la clé 4 : 56
Client number 2 > 5
Serveur : Réponse pour la clé 5 : 89
Client number 2 > 6
Serveur : Réponse pour la clé 6 : 34
```
