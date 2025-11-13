#include "Server.h"
#include "Database.h"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>\n";
        return 1;
    }

    uint16_t port = static_cast<uint16_t>(std::stoi(argv[1]));
    Database db("database.xml");
    Server server(port, &db);

    std::cout << "Starting server on port " << port << "...\n";
    server.open();
    std::cout << "Server ready. Waiting for clients...\n";

    server.serveForever();
}
