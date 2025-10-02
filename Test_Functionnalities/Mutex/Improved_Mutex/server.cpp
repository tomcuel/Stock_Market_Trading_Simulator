#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <queue>
#include <chrono>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

std::vector<int> values = {10, 20, 30, 40, 50};

// --- Synchronization ---
std::shared_mutex rw_mtx;    // allows multiple readers, only one writer
std::mutex queue_mtx;        // protects the write_queue
std::condition_variable cv;  // signals the writer thread when new work arrives
std::queue<int> write_queue; // FIFO queue of pending modifications
bool writing = false;

void writer_thread_func() {
    while (true) {
        std::unique_lock<std::mutex> lock(queue_mtx);
        cv.wait(lock, [] { return !write_queue.empty(); }); // wait for work

        int newValue = write_queue.front();
        write_queue.pop();
        lock.unlock();

        {
            std::unique_lock<std::shared_mutex> wlock(rw_mtx); // exclusive write
            std::this_thread::sleep_for(std::chrono::seconds(2)); // simulate work
            values.push_back(newValue);
            std::cout << "[Writer] Value added: " << newValue << std::endl;
        }
    }
}

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
        std::cout << "Client: " << input << std::endl;

        if (input == "view") {
            std::shared_lock<std::shared_mutex> rlock(rw_mtx); // multiple readers
            std::string response = "Values: ";
            for (int v : values) response += std::to_string(v) + " ";
            send(client_socket, response.c_str(), response.size(), 0);
        } 
        else if (input.rfind("modify ", 0) == 0){
            int newValue = std::stoi(input.substr(7));
            {
                std::lock_guard<std::mutex> lock(queue_mtx);
                write_queue.push(newValue);
            }
            cv.notify_one();
            std::string response = "Your modify request was queued: " + std::to_string(newValue);
            send(client_socket, response.c_str(), response.size(), 0);
        }
    }
    close(client_socket);
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    socklen_t addr_len = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("Error socket");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0){
        perror("Error bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0){
        perror("Error listen");
        exit(EXIT_FAILURE);
    }
    std::cout << "Server listening on port " << PORT << "...\n";

    // Start background writer thread
    std::thread(writer_thread_func).detach();

    while (true){
        if ((client_socket = accept(server_fd, (struct sockaddr*)&address, &addr_len)) < 0){
            perror("Error accept");
            continue;
        }
        std::cout << "Client connected!\n";
        std::thread(handle_client, client_socket).detach();
    }

    close(server_fd);
    return 0;
}
