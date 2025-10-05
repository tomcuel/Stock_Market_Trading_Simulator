#include <iostream>
#include <vector>
#include <thread>
#include <cstdlib>

void run_client(const std::string &message) {
    std::string command = "./client.x " + message;
    system(command.c_str()); // execute the command to run the client
}

int main() {
    std::vector<std::string> messages = {"1 1 2 3", "2 4 5 6", "3 7 8 9"};  // Client numbers + index numbers
    std::vector<std::thread> clients;

    for (const auto &msg : messages) {
        clients.emplace_back(run_client, msg);  // lauch a client thread
    }

    for (auto &client : clients) {
        client.join();  // wait for the client threads to finish
    }

    return 0;
}
