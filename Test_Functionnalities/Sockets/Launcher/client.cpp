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
// Function that monitors the server and exits if it stops
void monitor_server()
{
    while (is_running){
        if (!is_server_running()){
            std::cout << "\nServeur déconnecté. Fermeture du client...\n";
            is_running = false;
            exit(0); // Terminate the client process immediately
        }
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Check every second
    }
}


int main(int argc, char *argv[])
{
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <client number> <message>\n";
        return 1;
    }

    std::cout << "Client démarré...\n";
    std::thread server_monitor(monitor_server);

    // wait for the server to be available
    while (!is_server_running()){
        std::cout << "En attente du serveur...\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cout << "Serveur détecté. Entrez un nombre entre 0 et 9 (exit pour quitter) :\n";
    log_message("SYSTEM", "Client connecté !", LOG_FILE);

    // sending all the client messages
    for (int i=2; i< argc; ++i){
        if (!is_running){
            break;
        }

        std::cout << "Client number " << argv[1] << " > " << argv[i] << std::endl;
        std::string message = argv[1] + std::string(" ") + argv[i];

        // create socket to send message
        int client_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (client_socket == -1){
            std::cerr << "Erreur de création du socket\n";
            break;
        }
        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
        server_addr.sin_port = htons(SERVER_PORT);
        // connect to the server
        if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1){
            std::cerr << "Erreur de connexion au serveur\n";
            close(client_socket);
            break;
        }
        // send the message to the server
        send(client_socket, message.c_str(), message.size(), 0);
        // receive the response from the server
        char buffer[BUFFER_SIZE];
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0){
            std::cerr << "Erreur de réception du message. Le serveur a peut-être fermé.\n";
            close(client_socket);
            break;
        }
        buffer[bytes_received] = '\0';  // null-terminate the received string
        std::cout << "Serveur : " << buffer << std::endl;

        close(client_socket);

    }

    is_running = false;
    log_message("SYSTEM", "Client déconnecté !", LOG_FILE);
    server_monitor.join();  // wait for the monitoring thread to finish

    return 0;
}
