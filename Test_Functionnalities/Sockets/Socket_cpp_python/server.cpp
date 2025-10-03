#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main()
{
    int server_fd, client_socket;
    struct sockaddr_in address;
    char buffer[BUFFER_SIZE];
    // socket creation
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("Error socket");
        exit(EXIT_FAILURE);
    }
    // server address configuration
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    // socket linking to the address and port
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0){
        perror("Error bind");
        exit(EXIT_FAILURE);
    }

    // server listening
    if (listen(server_fd, 3) < 0){
        perror("Error listen");
        exit(EXIT_FAILURE);
    }
    std::cout << "Server waiting for connexion on the port " << PORT << "...\n";

    socklen_t addr_len = sizeof(address);
    if ((client_socket = accept(server_fd, (struct sockaddr*)&address, &addr_len)) < 0){
        perror("Erreur accept");
        exit(EXIT_FAILURE);
    }
    std::cout << "Client connected!\n";

    while (true){
        // get the message from the client
        memset(buffer, 0, BUFFER_SIZE);
        int valread = read(client_socket, buffer, BUFFER_SIZE);
        if (valread <= 0){
            std::cout << "Client deconnected\n";
            break;
        }
        std::cout << "Client: " << buffer << std::endl;

        // send a response to the client
        std::cout << "Server > ";
        std::string response;
        std::getline(std::cin, response);
        send(client_socket, response.c_str(), response.length(), 0);
    }

    close(client_socket);
    close(server_fd);
    return 0;
}
