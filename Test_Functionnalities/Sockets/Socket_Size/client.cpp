#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <thread>
#include <mutex>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <functional>
#include <algorithm> 
#include <cstring> 
#include <sys/socket.h>
#include <sys/time.h>

#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#define be64toh(x) OSSwapBigToHostInt64(x)
#else
#include <endian.h>
#endif

#define PORT 8080
#define BUFFER_SIZE 4096

std::mutex cout_mutex;
std::mutex recv_mutex;

std::string simple_hash(const std::string &data)
{
    std::hash<std::string> hasher;
    std::ostringstream oss;
    oss << std::hex << hasher(data);
    return oss.str();
}

// read naive data until marker
std::string recv_until_marker(int sock, const std::string &marker, std::string &leftover, int timeout_sec = 10)
{
    auto start =  std::chrono::steady_clock::now();

    std::lock_guard<std::mutex> lock(recv_mutex);
    std::string buffer = std::move(leftover);
    leftover.clear();
    char temp[BUFFER_SIZE];
    while (true){
        // check elapsed time for timeout
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
        if (elapsed > timeout_sec){
            throw std::runtime_error("Receive timed out waiting for marker");
        }
        ssize_t bytes = recv(sock, temp, sizeof(temp), 0);
        if (bytes <= 0){
            break;
        }
        buffer.append(temp, bytes);
        size_t pos = buffer.find(marker);
        if (pos != std::string::npos){
            std::string result = buffer.substr(0, pos);
            leftover = buffer.substr(pos + marker.size());
            return result;
        }
    }
    return buffer;
}

// read full framed message
std::string recv_full_string(int sock, std::string &leftover, int timeout_sec = 10)
{   
    auto start = std::chrono::steady_clock::now();

    std::lock_guard<std::mutex> lock(recv_mutex);
    uint64_t net_size;
    size_t need = sizeof(net_size);

    // If leftover has enough for size header, use it
    if (leftover.size() < need){
        char tmp[BUFFER_SIZE];
        while (leftover.size() < need){
            // check elapsed time for timeout
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
            if (elapsed > timeout_sec){
                throw std::runtime_error("Receive timed out waiting for size header");
            }
            ssize_t r = recv(sock, tmp, sizeof(tmp), 0);
            if (r <= 0){
                throw std::runtime_error("Failed to read size header");
            }
            leftover.append(tmp, r);
        }
    }

    std::memcpy(&net_size, leftover.data(), sizeof(net_size));
    leftover.erase(0, sizeof(net_size));

    uint64_t msg_size = be64toh(net_size);
    std::string data;
    data.reserve(msg_size);

    // consume leftover first
    if (!leftover.empty()){
        size_t use = msg_size < leftover.size() ? msg_size : leftover.size();
        data.append(leftover.substr(0, use));
        leftover.erase(0, use);
    }

    // then read remainder
    while (data.size() < msg_size){
        char tmp[BUFFER_SIZE];
        ssize_t r = recv(sock, tmp, sizeof(tmp), 0);
        if (r <= 0){
            throw std::runtime_error("Connection lost during receive");
        }
        size_t needed = msg_size - data.size();
        if ((size_t)r > needed){
            data.append(tmp, needed);
            leftover.assign(tmp + needed, r - needed);
        } 
        else {
            data.append(tmp, r);
        }
    }
    return data;
}

int main()
{
    int sock;
    struct sockaddr_in serv_addr;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Socket creation failed");
        return -1;
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        perror("Connection failed");
        return -1;
    }
    std::cout << "[CLIENT] Connected to server\n";

    try {
        std::string leftover;

        // Receive first test size (NUM_LINES_1) : both should pass 
        // Full framed receive first
        auto full_data = recv_full_string(sock, leftover);
        std::cout << "[CLIENT] Full received " << full_data.size() << " bytes\n";
        std::cout << "[CLIENT] Hash (full):  " << simple_hash(full_data) << "\n";

        // Naive receive after full
        const std::string marker_1 = "###END_OF_NAIVE###";
        auto naive_data = recv_until_marker(sock, marker_1, leftover);
        std::cout << "[CLIENT] Naive received " << naive_data.size() << " bytes\n";
        std::cout << "[CLIENT] Hash (naive): " << simple_hash(naive_data) << "\n";

        if (naive_data == full_data){
            std::cout << "[CLIENT] Data matches exactly for first test size!\n";
        }
        else {
            std::cout << "[CLIENT] Data differs between naive and full receive for first test size!\n";
        }

        // Receive second test size (NUM_LINES_2) : should make the naive version crash
        // Full framed receive first
        full_data = recv_full_string(sock, leftover);
        std::cout << "[CLIENT] Full received " << full_data.size() << " bytes\n";
        std::cout << "[CLIENT] Hash (full):  " << simple_hash(full_data) << "\n";

        // Naive receive after full
        const std::string marker_2 = "###END_OF_NAIVE###";
        naive_data = recv_until_marker(sock, marker_2, leftover);
        std::cout << "[CLIENT] Naive received " << naive_data.size() << " bytes\n";
        std::cout << "[CLIENT] Hash (naive): " << simple_hash(naive_data) << "\n";

        if (naive_data == full_data){
            std::cout << "[CLIENT] Data matches exactly for second test size!\n";
        }
        else {
            std::cout << "[CLIENT] Data differs between naive and full receive for second test size!\n";
        }
    } 
    catch (const std::exception &e){
        std::cerr << "[CLIENT] Error: " << e.what() << "\n";
    }

    close(sock);
    return 0;
}
