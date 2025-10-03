# 🧩 C++ Server and Python Tkinter Client

This project demonstrates an interaction between a **C++ TCP server** and a **Python GUI client** built using **Tkinter**.  
The communication is done through **sockets**, and the exchange is fully logged by the C++ server.

---

## ⚙️ Overview

The architecture mixes **C++ (for backend server logic)** and **Python (for graphical interface and communication)**.

- The **C++ server** (`server.cpp`) listens on port `8080`, accepts TCP connections, and responds to numeric queries
- The **Python client** (`client.py`) provides a **Tkinter GUI** for sending and receiving messages through sockets
- Each client request is handled in a separate thread, allowing for multiple simultaneous connections
- The server logs all exchanges into `conversation_log.txt`

---

## 🧠 Tkinter Client Architecture

The graphical interface is entirely managed through a dedicated **object-oriented Tkinter class** named `ChatGUI`.  
It provides a simple chat-like window to communicate with the server.

### 🔹 Sending Messages
Messages are sent through a background thread to avoid freezing the UI:
```python 
def on_send(self, event=None):
    message = self.entry.get()
    self.entry.delete(0, tk.END)
    threading.Thread(target=send_message, args=(message, self), daemon=True).start()
```
The function `send_message()` handles the socket communication:
- Creates a new TCP connection for each message
- Sends the message to the C++ server
- Receives and displays the server’s response in the GUI

### 🧱 Server Launch and Integration
Before the GUI starts, the Python script launches the C++ server automatically in the background:
```python 
def start_server():
    if not os.path.exists("./server.x"):
        raise FileNotFoundError("server.x not found. Compile your C++ server first.")

    # Launch C++ server silently in background
    server_process = subprocess.Popen(["./server.x"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    time.sleep(1)
```
This ensures the backend is ready before any user interaction occurs.

### 🧨 Shutdown and Cleanup
When the GUI is closed or the user types exit, the Python client:
- Terminates the background server process
- Closes all open sockets
- Gracefully quits the Tkinter main loop
```python 
def stop_all(gui):
    global server_process
    if server_process:
        os.kill(server_process.pid, signal.SIGTERM)
        print("Server stopped.")
    gui.root.quit()
```

## 🚀 Run Instructions
1. Compile the C++ server:
```bash
g++ -std=c++17 -Wall -Wfatal-errors server.cpp -o server.x
```
2. Launch the Python client:
```
python client.py
```
3. Use the GUI:
- Type a number between `0` and `9` → The server responds with a mapped value
- Type `exit` or close the window → The server shuts down automatically

Example Interaction:
```yaml
[GUI]
Client > 5
Server: Response for the key 5 : 89

Client > hello
Server: Error : Enter a number between 0 and 9
```
