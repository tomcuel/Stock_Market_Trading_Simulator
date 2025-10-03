#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <thread>
#include <chrono>
#include <vector>
#include <map>
#include <atomic>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <arpa/inet.h>


#define SERVER_IP "127.0.0.1"  // Adresse du serveur (localhost pour les tests locaux)
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

#define LOG_FILE "conversation_log.txt"


std::atomic<bool> is_running(true); 


// store a "message" coming from a "sender" into a file ("filename")
void log_message(const std::string& sender, const std::string& message, const std::string& filename)
{
    std::ofstream logFile(filename, std::ios::app);
    if (logFile){
        logFile << sender << ": " << message << std::endl;
    }
}
// clear the content of a file
void clear_file(const std::string& filename)
{
    std::ofstream file(filename, std::ios::trunc);
}
// function to check if the server is running by trying to connect
bool is_server_running()
{
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1){
        return false;  // Socket creation failed, server is not running
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0){
        close(client_socket);
        return false;
    }
    // try to connect to the server
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1){
        close(client_socket);
        return false;  // server is not running
    }

    close(client_socket);
    return true;
}
// function that monitors the server and exits if it stops
void monitor_server()
{
    while (is_running){
        if (!is_server_running()){
            std::cout << "\nServer deconnected. Client closing...\n";
            is_running = false;
            exit(0); // Terminate the client process immediately
        }
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Check every second
    }
}
// function to send a message to the server and receive a response based on threads
void send_message(const std::string& message)
{
    if (!is_running){
        return;
    }
    log_message("SYSTEM", "Client connected!", LOG_FILE);

    std::string client_number = message.substr(0, message.find(' '));  // Extract the client number
    int number_messages = std::count(message.begin(), message.end(), ' '); // Get the number of messages (number of spaces)
    std::vector<std::string> messages;
    // Extract the messages and store them in the vector
    size_t pos = message.find(' ') + 1;  // Position after the first space
    for (int i = 0; i < number_messages; ++i) {
        size_t next_space = message.find(' ', pos);
        if (next_space == std::string::npos) {
            messages.push_back(message.substr(pos));  // Last message
            break;
        }
        messages.push_back(message.substr(pos, next_space - pos));
        pos = next_space + 1;
    }
    
    for (int i=0; i<number_messages; ++i){
        if (!is_running){
            return;
        }
        std::cout << "Client number " << client_number << " > " << messages[i] << std::endl;
        std::string message = client_number + std::string(" ") + messages[i];

        // create socket to send message
        int client_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (client_socket == -1){
            std::cerr << "Error creating the socket\n";
            return;
        }
        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
        server_addr.sin_port = htons(SERVER_PORT);
        // connect to the server
        if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1){
            std::cerr << "Error to connect to the server\n";
            close(client_socket);
            return;
        }
        // send the message to the server
        send(client_socket, message.c_str(), message.size(), 0);
        // receive the response from the server
        char buffer[BUFFER_SIZE];
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0){
            std::cerr << "Error receiving the message. The server may have been closed\n";
            close(client_socket);
            return;
        }
        buffer[bytes_received] = '\0';  // null-terminate the received string
        std::cout << "Serveur : " << buffer << std::endl;

        close(client_socket);
        log_message("SYSTEM", "Client deconnected!", LOG_FILE);
    }
}


int main() 
{
    std::vector<std::string> messages = {"1 1 2 3", "2 4 5 6", "3 7 8 9"};
    std::vector<std::thread> threads;

    std::cout << "Client launched...\n";
    std::thread server_monitor(monitor_server);

    // wait for the server to be available
    while (!is_server_running()){
        std::cout << "Waiting for the server...\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cout << "Server detected. Enter a number between 0 and 9 (exit to quit):\n";

    // sending all the client messages to the server    
    for (const auto &msg : messages) {
        threads.emplace_back(send_message, msg);
    }
    
    for (auto &thread : threads) {
        thread.join();
    }
    
    is_running = false;
    server_monitor.join();
    return 0;
}

