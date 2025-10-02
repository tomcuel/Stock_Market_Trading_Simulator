#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main()
{
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];

    // Socket creation
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Error socket");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // IP address conversion
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0){
        perror("Invalid adress");
        exit(EXIT_FAILURE);
    }

    // Server connection
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
        perror("Error connexion");
        exit(EXIT_FAILURE);
    }
    std::cout << "Connected to the server !\n";

    while (true){
        std::cout << "Command (modify <value> / view / exit) > ";
        std::string message;
        std::getline(std::cin, message);

        // Send request to the server
        send(sock, message.c_str(), message.length(), 0);

        // Handle exit command
        if (message == "exit") break;

        // Receive response from the server
        memset(buffer, 0, BUFFER_SIZE);
        int valread = read(sock, buffer, BUFFER_SIZE);
        if (valread <= 0){
            std::cout << "Connexion closed by the server\n";
            break;
        }

        std::cout << "Server: " << buffer << std::endl;
    }

    close(sock);
    return 0;
}
