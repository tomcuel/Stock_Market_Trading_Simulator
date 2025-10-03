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


#define PORT 8080
#define BUFFER_SIZE 1024

#define LOG_FILE "conversation_log.txt"


std::atomic<bool> is_running(true); // flag to stop the server from running 


// map of key-value pairs
std::map<int, int> value_map ={
{0, 12},{1, 45},{2, 78},{3, 23},{4, 56},
{5, 89},{6, 34},{7, 67},{8, 90},{9, 10}
};


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
// function to check if a specific key is pressed on Mac OS
bool key_pressed(const char& keyboard_touch){
    struct termios oldt, newt;
    int oldf;
    char ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK); 
    int result = read(STDIN_FILENO, &ch, 1); 
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    
    return (result > 0 && ch == keyboard_touch);
}
// function to listen for exit key to shutdown the server
void shutdown_server(int server_fd)
{
    while (is_running){
        if (key_pressed('q')){
            is_running = false;
            std::cout << "\n[Server] Stop asked...\n";
            log_message("SYSTEM", "Server stopped", LOG_FILE);
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    close(server_fd);
}
// function to handle communication with the client
void handle_client(int client_socket)
{
    char buffer[BUFFER_SIZE];
    std::string response;

    while (is_running){
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0){
            break; // the client has disconnected or an error occurred
        }

        buffer[bytes_received] = '\0';  // Null-terminate the buffer
        std::string message = buffer;
        std::cout << "Client: " << message << std::endl;

        // log received message
        log_message("Client", message, LOG_FILE);

        // process the message and prepare the response
        try{
            int key = std::stoi(message);
            if (value_map.find(key) != value_map.end()){
                response = "Response for the key " + std::to_string(key) + " : " + std::to_string(value_map[key]);
            } else {
                response = "Error : Enter a number between 0 and 9";
            }
        } catch (...){
            response = "Error : Enter a number between 0 and 9";
        }

        // send the response to the client
        send(client_socket, response.c_str(), response.size(), 0);
        // log the response
        log_message("Server", response, LOG_FILE);
    }

    // close the client socket after communication
    close(client_socket);
}


int main()
{
    // clear the log file and create the server ready file
    clear_file(LOG_FILE);
    log_message("SYSTEM", "Server launched", LOG_FILE);

    int server_fd, client_socket;
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address)); // Initialiser à zéro
    // create a socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("Error socket");
        exit(EXIT_FAILURE);
    }
    // configuration of the server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    // bind the socket to the address and port
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0){
        perror("Error bind");
        exit(EXIT_FAILURE);
    }

    // listen for connections
    if (listen(server_fd, 3) < 0){
        perror("Error listen");
        exit(EXIT_FAILURE);
    }
    std::cout << "Server launched... (Press 'q' to quit)\n";
    
    // start a thread to listen for 'q' key press
    std::thread exit_listener(shutdown_server, server_fd);

    std::cout << "Server waitinf for connexion on the port " << PORT << "...\n";
    socklen_t addr_len = sizeof(address);
    if ((client_socket = accept(server_fd, (struct sockaddr*)&address, &addr_len)) < 0){
        perror("Error accept");
        exit(EXIT_FAILURE);
    }
    std::cout << "Client connected!\n";

    while (is_running){

        // communication with the client
        socklen_t addr_len = sizeof(address);
        int client_socket = accept(server_fd, (struct sockaddr*)&address, &addr_len);
        if (client_socket < 0){
            if (is_running){
                std::cerr << "Error of connexion : " << strerror(errno) << std::endl;
                log_message("SYSTEM", "Error of connexion", LOG_FILE);
            }
            continue;  // if there is no client, continue to the next iteration
        }

        std::cout << "Client connected!\n";
        // handle communication with the client in a separate thread
        std::thread client_thread(handle_client, client_socket);
        client_thread.detach(); // let the thread run independently to keep the server running
    }

    // clean up and exit
    close(client_socket);
    close(server_fd);
    exit_listener.join();
    return 0;
}