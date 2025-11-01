#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <vector>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <functional>

#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#define htobe64(x) OSSwapHostToBigInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#else
#include <endian.h>
#endif

#define PORT 8080
#define BUFFER_SIZE 4096
#define RESERVE_SIZE 200000000
#define NUM_LINES_1 1000000 
#define NUM_LINES_2 10000000

std::mutex cout_mutex;
std::mutex send_mutex;

// Lightweight Hash (portable)
std::string simple_hash(const std::string &data)
{
    std::hash<std::string> hasher;
    size_t h = hasher(data);
    std::ostringstream oss;
    oss << std::hex << h;
    return oss.str();
}

// Data generator
std::string generate_data(const int num_lines)
{
    std::string s;
    s.reserve(RESERVE_SIZE);
    for (int i = 0; i < num_lines; ++i){
        s += "Price_" + std::to_string(i) + " 2025-10-05T12:" + std::to_string(i % 60) + "\n";
    }
    return s;
}

// Send full
void send_full_string(int sock, const std::string &data)
{
    std::lock_guard<std::mutex> lock(send_mutex);
    uint64_t msg_size = data.size();
    uint64_t net_size = htobe64(msg_size);

    ssize_t sent = send(sock, &net_size, sizeof(net_size), 0);
    if (sent != sizeof(net_size))
        throw std::runtime_error("Failed to send message size header");

    size_t total = 0;
    while (total < data.size()){
        ssize_t bytes = send(sock, data.data() + total, data.size() - total, 0);
        if (bytes <= 0){
            throw std::runtime_error("Socket send error");
        }
        total += bytes;
    }
}

// Client handler
void handle_client(int client_socket)
{   
    std::vector<int> test_sizes = {NUM_LINES_1, NUM_LINES_2};
    for (const auto &num_lines : test_sizes){
        try {
            auto data = generate_data(num_lines);
            {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "[SERVER] Generated " << data.size() << " bytes\n";
                std::cout << "[SERVER] Hash: " << simple_hash(data) << "\n";
            }

            // Full framed send first
            send_full_string(client_socket, data);

            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            // Naive send with marker
            send(client_socket, data.c_str(), data.size(), 0);
            const std::string marker = "###END_OF_NAIVE###";
            send(client_socket, marker.c_str(), marker.size(), 0);

            {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "[SERVER] Both transmissions complete\n";
            }

        } 
        catch (const std::exception &e){
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cerr << "[SERVER] Error: " << e.what() << std::endl;
        }
    }
    close(client_socket);
}

// Main
int main()
{
    int server_fd, client_socket;
    struct sockaddr_in address;
    socklen_t addr_len = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0){
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    std::cout << "[SERVER] Listening on port " << PORT << "...\n";

    while (true){
        client_socket = accept(server_fd, (struct sockaddr *)&address, &addr_len);
        if (client_socket < 0){
            perror("accept failed");
            continue;
        }
        std::cout << "[SERVER] Client connected\n";
        std::thread(handle_client, client_socket).detach();
    }

    close(server_fd);
    return 0;
}
