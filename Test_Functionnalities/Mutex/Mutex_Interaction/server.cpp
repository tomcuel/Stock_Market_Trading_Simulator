#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <chrono>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

std::vector<int> values = {10, 20, 30, 40, 50};
std::mutex mtx;
bool modifying = false;

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int valread = read(client_socket, buffer, BUFFER_SIZE);
        if (valread <= 0) {
            std::cout << "Client deconnected\n";
            break;
        }
        std::string input(buffer);
        std::cout << "Client: " << input << std::endl;
        
        if (input == "view") {
            std::string response = "Liste of values: ";
            for (int val : values) response += std::to_string(val) + " ";
            send(client_socket, response.c_str(), response.length(), 0);
        } 
        else if (input.rfind("modify ", 0) == 0) {
            int newValue = std::stoi(input.substr(7));
            if (!modifying) {
                modifying = true;
                mtx.lock();
                std::this_thread::sleep_for(std::chrono::seconds(10)); // Delay before allowing another modification
                values.push_back(newValue);
                std::string response = "Value added: " + std::to_string(newValue);
                send(client_socket, response.c_str(), response.length(), 0);
                mtx.unlock();
                modifying = false;
            } else {
                std::string response = "Modification happening, try again later!";
                send(client_socket, response.c_str(), response.length(), 0);
            }
        }
    }
    close(client_socket);
}

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
        perror("Erreur bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0) {
        perror("Erreur listen");
        exit(EXIT_FAILURE);
    }
    std::cout << "Server waiting for connexion on the port " << PORT << "...\n";

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
