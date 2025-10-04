#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <queue>
#include <map>
#include <sstream>
#include <chrono>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// ----------------- Order Definition -----------------
struct Order {
    std::string type;        // BUY or SELL
    int quantity;
    std::string action_id;   // e.g., AAPL
    std::string order_type;  // MARKET, LIMIT, STOP, LIMIT_STOP
    double price = 0;
    double trigger_lower = 0;
    double trigger_upper = 0;
    std::string validity_date;
    std::string validity_time;
    int client_socket;       // who sent it
};

// ----------------- Shared State -----------------
struct ActionBook {
    std::queue<Order> write_queue;
    std::mutex queue_mtx;
    std::condition_variable cv;
    bool running = true;

    ActionBook() = default;
    ActionBook(const ActionBook&) = delete;             // prevent copying
    ActionBook& operator=(const ActionBook&) = delete;  // prevent assignment
    ActionBook(ActionBook&&) = delete;                  // prevent moving
    ActionBook& operator=(ActionBook&&) = delete;       // prevent moving
};

std::map<std::string, ActionBook> action_books; // per-action queues
std::shared_mutex state_mtx; // readers/writers for market state

// Fake market state
std::map<std::string, std::vector<std::string>> market_state = {
    {"AAPL", {"Price=150"}},
    {"GOOG", {"Price=2800"}},
    {"TSLA", {"Price=750"}}
};

// ----------------- Writer Thread -----------------
void writer_thread_func(std::string action_id) {
    ActionBook &book = action_books[action_id];

    while (true) {
        std::unique_lock<std::mutex> lock(book.queue_mtx);
        book.cv.wait(lock, [&] { return !book.write_queue.empty(); });

        Order order = book.write_queue.front();
        book.write_queue.pop();
        lock.unlock();

        {
            // Exclusive write to market state
            std::unique_lock<std::shared_mutex> wlock(state_mtx);
            std::this_thread::sleep_for(std::chrono::seconds(2)); // simulate DB work
            market_state[order.action_id].push_back(order.type + " " + std::to_string(order.quantity));
            std::cout << "[Writer-" << action_id << "] Processed order: "
                      << order.type << " " << order.quantity << " " << order.action_id << std::endl;
        }

        // Confirm to client
        std::string doneMsg = "Order executed: " + order.type + " " + order.action_id + "\n";
        send(order.client_socket, doneMsg.c_str(), doneMsg.size(), 0);
    }
}

// ----------------- Client Handler -----------------
void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int valread = read(client_socket, buffer, BUFFER_SIZE);
        if (valread <= 0) {
            std::cout << "Client disconnected\n";
            break;
        }
        std::string input(buffer);
        input.erase(input.find_last_not_of("\n\r") + 1);

        std::cout << "Client: " << input << std::endl;

        if (input.rfind("view", 0) == 0) {
            std::istringstream iss(input);
            std::string cmd, action_id;
            iss >> cmd >> action_id;

            std::shared_lock<std::shared_mutex> rlock(state_mtx);
            std::string response = "State for " + action_id + ": ";
            for (auto &s : market_state[action_id]) response += s + "; ";
            response += "\n";
            send(client_socket, response.c_str(), response.size(), 0);
        } 
        else if (input.rfind("BUY", 0) == 0 || input.rfind("SELL", 0) == 0) {
            std::istringstream iss(input);
            Order order;
            iss >> order.type 
                >> order.quantity 
                >> order.action_id 
                >> order.order_type;

            if (order.order_type == "LIMIT") {
                iss >> order.price;
            } 
            else if (order.order_type == "STOP") {
                iss >> order.price >> order.trigger_upper;
            } 
            else if (order.order_type == "LIMIT_STOP") {
                iss >> order.price >> order.trigger_lower >> order.trigger_upper;
            }

            iss >> order.validity_date >> order.validity_time;
            order.client_socket = client_socket;

            // Ensure the ActionBook exists safely
            {
                std::unique_lock<std::shared_mutex> wlock(state_mtx);
                auto [it, inserted] = action_books.try_emplace(order.action_id);
                if (inserted) {
                    // Spawn a writer thread for this action
                    std::thread(writer_thread_func, order.action_id).detach();
                }
            }

            // Push the order into the right queue
            {
                ActionBook &book = action_books.at(order.action_id);
                std::lock_guard<std::mutex> qlock(book.queue_mtx);
                book.write_queue.push(order);
                book.cv.notify_one();
            }

            std::string response = "Your order was queued: " + input + "\n";
            send(client_socket, response.c_str(), response.size(), 0);
        }
    }
    close(client_socket);
}

// ----------------- Main -----------------
int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    socklen_t addr_len = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Error socket");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Error bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0) {
        perror("Error listen");
        exit(EXIT_FAILURE);
    }
    std::cout << "Server listening on port " << PORT << "...\n";

    while (true) {
        if ((client_socket = accept(server_fd, (struct sockaddr*)&address, &addr_len)) < 0) {
            perror("Error accept");
            continue;
        }
        std::cout << "Client connected!\n";
        std::thread(handle_client, client_socket).detach();
    }

    close(server_fd);
    return 0;
}
