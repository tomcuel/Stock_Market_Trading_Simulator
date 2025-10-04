#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <queue>
#include <map>
#include <unordered_map>
#include <vector>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// ---------------- Order + Portfolio ----------------
struct Order {
    std::string type;        // BUY or SELL
    int quantity;
    std::string symbol;      // e.g. "AAPL"
    std::string order_type;  // LIMIT for now
    double price;
    int client_id;           // to know who sent it
};

struct Portfolio {
    double cash = 10000.0; // starting balance
    std::map<std::string, int> holdings; // symbol -> shares
};

// ---------------- Order Book ----------------
struct OrderCompareBuy {
    bool operator()(const Order& a, const Order& b)
    {
        return a.price < b.price; // highest price first
    }
};
struct OrderCompareSell {
    bool operator()(const Order& a, const Order& b)
    {
        return a.price > b.price; // lowest price first
    }
};

struct OrderBook {
    std::priority_queue<Order, std::vector<Order>, OrderCompareBuy> buyOrders;
    std::priority_queue<Order, std::vector<Order>, OrderCompareSell> sellOrders;
};

// ---------------- Global State ----------------
std::map<std::string, OrderBook> market;              // symbol -> orderbook
std::unordered_map<int, Portfolio> portfolios;        // client_id -> portfolio
std::unordered_map<std::string, std::shared_mutex> market_mutexes;
std::unordered_map<int, std::mutex> portfolio_mutexes;
std::unordered_map<int, int> client_sockets;          // client_id -> socket
std::mutex client_sockets_mutex;

// ---------------- Trade Execution ----------------
void execute_trade(int buyer, int seller, const std::string& symbol, int qty, double price)
{
    double total = qty * price;

    {
        std::lock_guard<std::mutex> lock(portfolio_mutexes[buyer]);
        portfolios[buyer].cash -= total;
        portfolios[buyer].holdings[symbol] += qty;
    }
    {
        std::lock_guard<std::mutex> lock(portfolio_mutexes[seller]);
        portfolios[seller].cash += total;
        portfolios[seller].holdings[symbol] -= qty;
    }

    std::ostringstream oss;
    oss << "[TRADE] " << buyer << " bought " << qty << " " << symbol << " from " << seller << " @ " << price << "\n";
    std::string trade_msg = oss.str();

    // Send trade to both buyer and seller if still connected
    std::lock_guard<std::mutex> lock(client_sockets_mutex);
    if (client_sockets.count(buyer))
        send(client_sockets[buyer], trade_msg.c_str(), trade_msg.size(), 0);
    if (client_sockets.count(seller))
        send(client_sockets[seller], trade_msg.c_str(), trade_msg.size(), 0);
}

// ---------------- Matching Engine ----------------
void process_order(Order o)
{
    std::unique_lock<std::shared_mutex> lock(market_mutexes[o.symbol]);
    auto& ob = market[o.symbol];

    if (o.type == "BUY"){
        while (!ob.sellOrders.empty() && ob.sellOrders.top().price <= o.price && o.quantity > 0){
            Order sell = ob.sellOrders.top();
            ob.sellOrders.pop();

            int tradedQty = std::min(o.quantity, sell.quantity);
            double tradePrice = sell.price; // fixing: seller’s ask wins

            execute_trade(o.client_id, sell.client_id, o.symbol, tradedQty, tradePrice);

            o.quantity -= tradedQty;
            sell.quantity -= tradedQty;

            if (sell.quantity > 0){
                ob.sellOrders.push(sell);
            }
        }
        if (o.quantity > 0){
            ob.buyOrders.push(o); // leftover goes to book
        }
    }
    else if (o.type == "SELL") {
        while (!ob.buyOrders.empty() && ob.buyOrders.top().price >= o.price && o.quantity > 0){
            Order buy = ob.buyOrders.top();
            ob.buyOrders.pop();

            int tradedQty = std::min(o.quantity, buy.quantity);
            double tradePrice = buy.price; // fixing: buyer’s bid wins

            execute_trade(buy.client_id, o.client_id, o.symbol, tradedQty, tradePrice);

            o.quantity -= tradedQty;
            buy.quantity -= tradedQty;

            if (buy.quantity > 0){
                ob.buyOrders.push(buy);
            }
        }
        if (o.quantity > 0){
            ob.sellOrders.push(o);
        }
    }
}

// ---------------- Views ----------------
std::string view_market(const std::string& symbol)
{
    std::shared_lock<std::shared_mutex> lock(market_mutexes[symbol]);
    auto& ob = market[symbol];
    std::ostringstream oss;

    oss << "Market for " << symbol << "\n";
    oss << "  Buys: ";
    auto copyB = ob.buyOrders;
    while (!copyB.empty()){
        auto o = copyB.top(); copyB.pop();
        oss << "[" << o.quantity << "@" << o.price << "] ";
    }
    oss << "\n  Sells: ";
    auto copyS = ob.sellOrders;
    while (!copyS.empty()){
        auto o = copyS.top(); copyS.pop();
        oss << "[" << o.quantity << "@" << o.price << "] ";
    }
    oss << "\n";
    return oss.str();
}

std::string view_portfolio(int client_id)
{
    std::lock_guard<std::mutex> lock(portfolio_mutexes[client_id]);
    auto& p = portfolios[client_id];
    std::ostringstream oss;
    oss << "Portfolio (cash=" << p.cash << "): ";
    for (auto& [sym, qty] : p.holdings){
        oss << sym << "=" << qty << " ";
    }
    return oss.str();
}

// ---------------- Networking ----------------
void handle_client(int client_socket, int client_id) 
{
    {
        std::lock_guard<std::mutex> lock(client_sockets_mutex);
        client_sockets[client_id] = client_socket;
    }

    portfolios[client_id] = Portfolio();
    portfolio_mutexes[client_id]; // ensure exists
    char buffer[BUFFER_SIZE];

    while (true){
        memset(buffer, 0, BUFFER_SIZE);
        int valread = read(client_socket, buffer, BUFFER_SIZE);
        if (valread <= 0){
            std::cout << "Client " << client_id << " disconnected\n";
            break;
        }

        std::string input(buffer);
        std::istringstream iss(input);
        std::string cmd;
        iss >> cmd;

        std::string response;

        if (cmd == "BUY" || cmd == "SELL"){
            Order o;
            o.type = cmd;
            iss >> o.quantity >> o.symbol >> o.order_type >> o.price;
            o.client_id = client_id;

            process_order(o);
            response = "[ORDER] client " + std::to_string(client_id) + " -> " + input;
        }
        else if (cmd == "VIEW"){
            std::string what;
            iss >> what;
            if (what == "MARKET"){
                std::string symbol; iss >> symbol;
                response = view_market(symbol);
            } 
            else if (what == "PORTFOLIO"){
                response = view_portfolio(client_id);
            } else {
                response = "Unknown VIEW option";
            }
        } else {
            response = "Unknown command";
        }

        send(client_socket, response.c_str(), response.size(), 0);
    }

    {
        std::lock_guard<std::mutex> lock(client_sockets_mutex);
        client_sockets.erase(client_id);
    }

    close(client_socket);
}

int main()
{
    int server_fd, client_socket;
    struct sockaddr_in address;
    socklen_t addr_len = sizeof(address);
    int client_id_counter = 1;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("Error socket");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0){
        perror("Error bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0){
        perror("Error listen");
        exit(EXIT_FAILURE);
    }
    std::cout << "Server listening on port " << PORT << "...\n";

    while (true){
        if ((client_socket = accept(server_fd, (struct sockaddr*)&address, &addr_len)) < 0) {
            perror("Error accept");
            continue;
        }
        int client_id = client_id_counter++;
        std::cout << "Client " << client_id << " connected!\n";
        std::thread(handle_client, client_socket, client_id).detach();
    }

    close(server_fd);
    return 0;
}
