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

#define SERVER_IP "127.0.0.1"  // Server Adress (localhost for testing)
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
// Function that monitors the server and exits if it stops
void monitor_server()
{
    while (is_running){
        if (!is_server_running()){
            std::cout << "\nServer deconnected. Closing the client...\n";
            is_running = false;
            exit(0); // Terminate the client process immediately
        }
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Check every second
    }
}


int main()
{
    std::cout << "Client launched...\n";
    std::thread server_monitor(monitor_server);

    // wait for the server to be available
    while (!is_server_running()){
        std::cout << "Waiting for the server...\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cout << "Serveur detected. Enter a number between 0  and 9 (exit to quit):\n";
    log_message("SYSTEM", "Client connected!", LOG_FILE);

    while (is_running){

        std::cout << "Client > ";
        std::string message;
        getline(std::cin, message);

        if (message == "exit"){
            is_running = false;
            std::cout << "Client deconnected!" << std::endl;
            log_message("SYSTEM", "Client deconnected!", LOG_FILE);
            break;
        }
        if (!is_running){
            break;
        }

        // create socket to send message
        int client_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (client_socket == -1){
            std::cerr << "Error to creat the socket\n";
            break;
        }
        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
        server_addr.sin_port = htons(SERVER_PORT);
        // connect to the server
        if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1){
            std::cerr << "Error of connexion to the server\n";
            close(client_socket);
            break;
        }
        // send the message to the server
        send(client_socket, message.c_str(), message.size(), 0);
        // receive the response from the server
        char buffer[BUFFER_SIZE];
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0){
            std::cerr << "Error for message reception. The server may have been closed\n";
            close(client_socket);
            break;
        }
        buffer[bytes_received] = '\0';  // null-terminate the received string
        std::cout << "Serveur : " << buffer << std::endl;

        close(client_socket);
    }

    is_running = false;
    server_monitor.join();  // wait for the monitoring thread to finish

    return 0;
}
