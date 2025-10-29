#include "Server.h"
#include "Client.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage:\n"
                  << "  " << argv[0] << " server <port>\n"
                  << "  " << argv[0] << " client <host> <port>\n";
        return 1;
    }

    std::string mode = argv[1];

    try {
        
        if (mode == "server") {
            if (argc != 3) {
                std::cerr << "Usage: " << argv[0] << " server <port>\n";
                return 1;
            }

            uint16_t port = static_cast<uint16_t>(std::stoi(argv[2]));
            Server server(port);

            std::cout << "Starting server on port " << port << "...\n";
            server.open();
            std::cout << "Server ready. Waiting for clients...\n";

            server.serveForever();
        }
        else if (mode == "client") {
            if (argc != 4) {
                std::cerr << "Usage: " << argv[0] << " client <host> <port>\n";
                return 1;
            }

            std::string host = argv[2];
            uint16_t port = static_cast<uint16_t>(std::stoi(argv[3]));

            Client client(host, port);
            client.connectTo();

            std::cout << "Connected to " << host << ":" << port << "\n";

            std::string input;
            char buffer[1024];

            while (true) {
                std::cout << "> ";
                if (!std::getline(std::cin, input)) break;

                if (input == "exit" || input == "quit") break;

                client.sendAll(input.data(), input.size());
                client.sendAll("\n", 1);

                ssize_t n = client.recvSome(buffer, sizeof(buffer) - 1);
                if (n <= 0) {
                    std::cout << "Server closed connection.\n";
                    break;
                }
                buffer[n] = '\0';
                std::cout << "Server replied: " << buffer;
            }

            client.close();
        }

        else {
            std::cerr << "Unknown mode: " << mode << "\n";
            return 1;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
