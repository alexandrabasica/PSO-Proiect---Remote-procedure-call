#include "Client.h"
#include <iostream>

int main() {
    try {
        Client client("127.0.0.1", 12345);
        client.connectTo();
        
        auto traceHandler = [](const std::string& line) {
            // std::cout << "TRACE: " << line << "\n"; // Suppress trace for clarity
        };
        auto outHandler = [](const std::string& line) {
            std::cout << "OUTPUT: " << line << "\n";
        };

        std::cout << "1. Declaring variable 'int a = 10;'\n";
        client.trace("int a = 10;", traceHandler, outHandler);

        std::cout << "\n2. Using variable 'printf(\"%d\", a);'\n";
        client.trace("printf(\"Value of a: %d\\n\", a);", traceHandler, outHandler);

        std::cout << "\n3. Trying invalid code 'printf(b);'\n";
        client.trace("printf(\"%d\", b);", traceHandler, outHandler);

        std::cout << "\n4. Verifying history is intact 'printf(a);'\n";
        client.trace("printf(\"Value of a is still: %d\\n\", a);", traceHandler, outHandler);
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
