import subprocess
import socket
import threading
import random
import time
import os
import signal
import re

# ---------------- CONFIGURATION ----------------
SERVER_EXEC = "./server.x"
HOST = "127.0.0.1"
PORT = 8080

CLIENT_COUNT = 10
TEST_DURATION = 1
HIGH_CONTENTION_SYMBOL = "AAPL"
ORDER_TYPES = ["LIMIT"]

# ---------------- REGEX PATTERNS ----------------
order_rx = re.compile(r"\[ORDER\] client (\d+) -> (BUY|SELL) (\d+) (\w+) (\w+) ([\d\.]+)")
trade_rx = re.compile(r"\[TRADE\] (\d+) bought (\d+) (\w+) from (\d+) @ ([\d\.]+)")

# ---------------- GLOBAL LOGS ----------------
portfolio_asks = []        # portfolio asks
fifo_orders = []           # server-side order log
trade_log = []             # server-side trade log

log_filename = "nrt_stress_test_log.txt"
# Capture everything printed in memory (and write progressively to file)
log_lines = []
def log(msg):
    print(msg)
    log_lines.append(msg)

# ---------------- RANDOM ORDER GENERATION ----------------
def random_order(client_id):
    side = random.choice(["BUY", "SELL"])
    qty = random.randint(1, 50)
    price = round(random.uniform(90, 160), 2)
    return f"{side} {qty} {HIGH_CONTENTION_SYMBOL} LIMIT {price}"

# ---------------- CLIENT THREAD FUNCTION ----------------
def client_thread_func(client_id):
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        for _ in range(20):
            try:
                s.connect((HOST, PORT))
                break
            except ConnectionRefusedError:
                time.sleep(0.1)
        else:
            log(f"[Client {client_id}] ERROR: Could not connect")
            return

        s.settimeout(1)
        start = time.time()
        while time.time() - start < TEST_DURATION:
            action_type = random.choices(["ORDER", "VIEW_PORTFOLIO"], weights=[0.8, 0.2])[0]
            msg = random_order(client_id) if action_type == "ORDER" else "VIEW PORTFOLIO"
            s.sendall(msg.encode())
            try:
                data = s.recv(2048)
                if data:
                    decoded = data.decode().strip()
                    log(f"[Client {client_id}]: {decoded}")

                    # --- Also log socket-level [ORDER] and [TRADE] messages ---
                    m = order_rx.search(decoded)
                    if m:
                        fifo_orders.append({
                            "client": int(m.group(1)),
                            "side": m.group(2),
                            "qty": int(m.group(3)),
                            "symbol": m.group(4),
                            "order_type": m.group(5),
                            "price": float(m.group(6))
                        })

                    m2 = trade_rx.search(decoded)
                    if m2:
                        trade_log.append({
                            "buyer": int(m2.group(1)),
                            "qty": int(m2.group(2)),
                            "symbol": m2.group(3),
                            "seller": int(m2.group(4)),
                            "price": float(m2.group(5))
                        })
            except socket.timeout:
                pass

            time.sleep(random.uniform(0.05, 0.2))

        s.sendall(b"exit")
        s.close()
    except Exception as e:
        log(f"[Client {client_id}] ERROR: {e}")


# ---------------- SERVER CONTROL ----------------
def start_server():
    if not os.path.exists(SERVER_EXEC):
        log(f"Error: {SERVER_EXEC} not found!")
        return None
    log("Launching trading server...")
    return subprocess.Popen(
        [SERVER_EXEC],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1
    )


# ---------------- MAIN TEST ----------------
def main():
    
    server_proc = start_server()
    if server_proc is None:
        return

    # Just sleep a short time to let server start (like your working script)
    time.sleep(1.5)
    # Start monitoring server output
    monitor_thread = threading.Thread(args=(server_proc,), daemon=True)
    monitor_thread.start()

    log(f"Starting {CLIENT_COUNT} clients...\n")
    threads = []
    for i in range(1, CLIENT_COUNT + 1):
        t = threading.Thread(target=client_thread_func, args=(i,))
        t.start()
        threads.append(t)

    for t in threads:
        t.join()

    log("\nAll clients finished. Stopping server...")
    try:
        server_proc.send_signal(signal.SIGINT)
        server_proc.wait(timeout=3)
    except subprocess.TimeoutExpired:
        server_proc.kill()

    log("\nNRT mutex stress test complete!")
    log(f"Total orders processed: {len(fifo_orders)}")
    for i, order in enumerate(fifo_orders, 1):
        log(f"{i}: Client {order['client']} {order['side']} {order['qty']} {order['symbol']} {order['order_type']} @ {order['price']}")

    log(f"\nTotal trades executed: {len(trade_log)}")
    for i, trade in enumerate(trade_log, 1):
        log(f"{i}: Buyer {trade['buyer']} bought {trade['qty']} {trade['symbol']} from Seller {trade['seller']} @ {trade['price']}")
    
    # Log the whole terminal output (sockets message + fifo and trade logs) to a file for manual inspection if needed
    with open(log_filename, "w") as f:
        f.write("\n".join(log_lines))

    log(f"\nFull terminal output saved to: {os.path.abspath(log_filename)}")

if __name__ == "__main__":
    main()
