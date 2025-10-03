import socket
import threading
import tkinter as tk
from tkinter import scrolledtext
import subprocess
import time
import os
import signal

SERVER_IP = "127.0.0.1"
SERVER_PORT = 8080
BUFFER_SIZE = 1024

is_running = True
server_process = None


# ---------- CLIENT ----------
def send_message(message, gui):
    global is_running
    if not message.strip():
        return
    if message == "exit":
        gui.append_text("Client disconnected!\n")
        stop_all(gui)
        return

    try:
        client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_socket.connect((SERVER_IP, SERVER_PORT))
        client_socket.sendall(message.encode())
        data = client_socket.recv(BUFFER_SIZE).decode()
        client_socket.close()

        gui.append_text(f"Client > {message}\n")
        gui.append_text(f"Server: {data}\n")
    except Exception as e:
        gui.append_text(f"[Error] {e}\n")
        stop_all(gui)


# ---------- GUI ----------
class ChatGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("C++ Server Client")
        self.root.geometry("600x400")

        self.text_area = scrolledtext.ScrolledText(root, wrap=tk.WORD, state="disabled")
        self.text_area.pack(padx=10, pady=10, fill=tk.BOTH, expand=True)

        self.entry = tk.Entry(root)
        self.entry.pack(padx=10, pady=5, fill=tk.X)
        self.entry.bind("<Return>", self.on_send)

        self.send_button = tk.Button(root, text="Send", command=self.on_send)
        self.send_button.pack(pady=5)

    def append_text(self, text):
        self.text_area.configure(state="normal")
        self.text_area.insert(tk.END, text + "\n")
        self.text_area.see(tk.END)
        self.text_area.configure(state="disabled")

    def on_send(self, event=None):
        message = self.entry.get()
        self.entry.delete(0, tk.END)
        threading.Thread(target=send_message, args=(message, self), daemon=True).start()


# ---------- SERVER LAUNCH ----------
def start_server():
    global server_process
    if not os.path.exists("./server.x"):
        raise FileNotFoundError("server.x not found. Compile your C++ server first.")

    # Launch C++ server in background
    server_process = subprocess.Popen(["./server.x"], stdout=subprocess.PIPE, stderr=subprocess.PIPE) # background process, we don't see outputs
    # server_process = subprocess.Popen(["./server.x"]) if we want to see server outputs in terminal
    print("Server started as background process.")

    # Give server some time to start listening
    time.sleep(1)


# ---------- STOP ----------
def stop_all(gui):
    global is_running, server_process
    is_running = False

    # Kill server process when closing client
    if server_process:
        os.kill(server_process.pid, signal.SIGTERM)
        print("Server stopped.")

    gui.root.quit()


# ---------- MAIN ----------
def main():
    # Start server
    start_server()

    # Launch GUI
    root = tk.Tk()
    gui = ChatGUI(root)
    root.protocol("WM_DELETE_WINDOW", lambda: stop_all(gui))
    root.mainloop()


if __name__ == "__main__":
    main()
