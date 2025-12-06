#include "Client.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    try {
        Client client("127.0.0.1", 12345);
        client.connectTo();
        
        std::cout << "Sending TRACE command...\n";
        client.trace("ls -la", [](const std::string& line) {
            std::cout << "TRACE: " << line << "\n";
        });
        
        std::cout << "Trace finished.\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
