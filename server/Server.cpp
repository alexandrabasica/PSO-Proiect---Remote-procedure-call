#include "Server.h"
#include "Tracer.h"
#include <stdexcept>
#include <system_error>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <thread>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <csignal>
#include <sys/wait.h>
#include <fstream>
#include <chrono>
#include <cstdio>
#include <cstdlib>

Server::Server(uint16_t port, Database* database) noexcept
    : m_port(port), m_listen_fd(-1), m_client_fd(-1), m_running(false), db(database)
{
    m_handler = [this](int client_fd, const sockaddr_in&) {
        constexpr size_t BUF_SZ = 1024;
        char buf[BUF_SZ];
        std::string code_history = "";

        for (;;) {
            ssize_t r = recv(client_fd, buf, BUF_SZ - 1, 0);
            if (r > 0) {
                buf[r] = '\0';
                std::string msg(buf);
                
                if (msg.rfind("TRACE ", 0) == 0) {
                    std::string new_code = msg.substr(6);
                    if (db) db->addLog("Tracing code snippet");
                    
                    auto now = std::chrono::system_clock::now();
                    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
                    std::string baseName = "trace_tmp_" + std::to_string(timestamp) + "_" + std::to_string(client_fd);
                    std::string sourceFile = baseName + ".cpp";
                    std::string exeFile = "./" + baseName;

                    std::string full_code = code_history + "\n" + new_code;

                    std::ofstream out(sourceFile);
                    out << "#include <iostream>\n"
                        << "#include <cstdio>\n"
                        << "#include <cstdlib>\n"
                        << "#include <unistd.h>\n"
                        << "#include <string>\n"
                        << "#include <vector>\n"
                        << "int main() {\n"
                        << full_code << "\n"
                        << "return 0;\n"
                        << "}\n";
                    out.close();

                    std::string compileCmd = "g++ " + sourceFile + " -o " + baseName + " 2>&1";
                    FILE* pipe = popen(compileCmd.c_str(), "r");
                    if (!pipe) {
                        std::string err = "OUT:Failed to run compiler\n";
                        send(client_fd, err.c_str(), err.size(), 0);
                    } else {
                        char buffer[128];
                        std::string result = "";
                        while (!feof(pipe)) {
                            if (fgets(buffer, 128, pipe) != NULL)
                                result += buffer;
                        }
                        int rc = pclose(pipe);

                        if (rc == 0) {
                            code_history = full_code;

                            auto sendCallback = [client_fd](const TraceEvent& evt) {
                                std::string line = "TRACE:" + evt.type + " [" + std::to_string(evt.pid) + "]: " + evt.details + "\n";
                                send(client_fd, line.c_str(), line.size(), 0);
                            };
                            
                            
                            std::string outputFile = baseName + ".out";
                            std::string runCmd = exeFile + " > " + outputFile + " 2>&1";
                            
                            Tracer tracer(runCmd, sendCallback);
                            tracer.run();

                            // Read output
                            std::ifstream outFile(outputFile);
                            std::string outLine;
                            while (std::getline(outFile, outLine)) {
                                std::string msg = "OUT:" + outLine + "\n";
                                send(client_fd, msg.c_str(), msg.size(), 0);
                            }
                            outFile.close();
                            remove(outputFile.c_str());

                        } else {
                            std::string errMsg = "OUT:Compilation failed:\n" + result;
                            send(client_fd, errMsg.c_str(), errMsg.size(), 0);
                        }
                    }

                    remove(sourceFile.c_str());
                    remove(baseName.c_str());
                    
                    std::string endMsg = "TRACE_END\n";
                    send(client_fd, endMsg.c_str(), endMsg.size(), 0);
                } else {
                    ssize_t sent = 0;
                    while (sent < r) {
                        ssize_t n = send(client_fd, buf + sent, static_cast<size_t>(r - sent), 0);
                        if (n < 0) return;
                        sent += n;
                    }
                    if (db) db->addLog("Mesaj primit: " + std::string(buf, r));
                }
            } else if (r == 0) {
                if (db) db->addLog("Client deconectat");
                return;
            } else {
                if (errno == EINTR) continue;
                return;
            }
        }
    };

    if(db) db->addLog("Server pornit pe port " + std::to_string(port));
}


Server::~Server() {
    close();
}

Server::Server(Server&& other) noexcept
    : m_port(other.m_port),
      m_listen_fd(other.m_listen_fd),
      m_client_fd(other.m_client_fd),
      m_running(other.m_running.load()),
      m_handler(std::move(other.m_handler))
{
    other.m_listen_fd = -1;
    other.m_client_fd = -1;
    other.m_running.store(false);
}

Server& Server::operator=(Server&& other) noexcept {
    if (this != &other) {
        close();
        m_port = other.m_port;
        m_listen_fd = other.m_listen_fd;
        m_client_fd = other.m_client_fd;
        m_running.store(other.m_running.load());
        m_handler = std::move(other.m_handler);

        other.m_listen_fd = -1;
        other.m_client_fd = -1;
        other.m_running.store(false);
    }
    return *this;
}

void Server::ensureOpen() const {
    if (m_listen_fd < 0)
        throw std::runtime_error("listen socket not opened; call open() first");
}

void Server::open() {
    if (m_listen_fd >= 0) return;



    m_listen_fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_listen_fd < 0) {
        throw std::system_error(errno, std::generic_category(), "socket() failed");
    }

    int opt = 1;
    if (setsockopt(m_listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        int err = errno;
        ::close(m_listen_fd);
        m_listen_fd = -1;
        throw std::system_error(err, std::generic_category(), "setsockopt(SO_REUSEADDR) failed");
    }

    sockaddr_in sa{};
    sa.sin_family = AF_INET;
    sa.sin_port = htons(m_port);
    if (inet_pton(AF_INET, "0.0.0.0", &sa.sin_addr) != 1) {
        ::close(m_listen_fd);
        m_listen_fd = -1;
        throw std::runtime_error("inet_pton failed");
    }

    if (bind(m_listen_fd, reinterpret_cast<sockaddr*>(&sa), sizeof(sa)) < 0) {
        int err = errno;
        ::close(m_listen_fd);
        m_listen_fd = -1;
        throw std::system_error(err, std::generic_category(), "bind() failed");
    }

    if (listen(m_listen_fd, SOMAXCONN) < 0) {
        int err = errno;
        ::close(m_listen_fd);
        m_listen_fd = -1;
        throw std::system_error(err, std::generic_category(), "listen() failed");
    }

    m_running.store(true);
}

void Server::closeClientIfOpen() {
    if (m_client_fd >= 0) {
        ::shutdown(m_client_fd, SHUT_RDWR);
        ::close(m_client_fd);
        m_client_fd = -1;
    }
}

void Server::close() {
    m_running.store(false);
    closeClientIfOpen();
    if (m_listen_fd >= 0) {
        ::shutdown(m_listen_fd, SHUT_RDWR);
        ::close(m_listen_fd);
        m_listen_fd = -1;
    }
}

void Server::serveOneClient() {
    ensureOpen();

    sockaddr_in client_addr{};
    socklen_t addrlen = sizeof(client_addr);
    m_client_fd = accept(m_listen_fd, reinterpret_cast<sockaddr*>(&client_addr), &addrlen);
    if (m_client_fd < 0) {
        throw std::system_error(errno, std::generic_category(), "accept() failed");
    }

    char addrbuf[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &client_addr.sin_addr, addrbuf, sizeof(addrbuf)))
        std::cout << addrbuf << " connected!\n";
    else
        std::cout << "Client connected (address unknown)\n";

    try {
        m_handler(m_client_fd, client_addr);
    } catch (...) {
        
    }

    closeClientIfOpen();
}

void Server::serveForever() {
    ensureOpen();

while (m_running.load()) {
    sockaddr_in client_addr{};
    socklen_t addrlen = sizeof(client_addr);
    int client = -1;
    try {
        client = accept(m_listen_fd, reinterpret_cast<sockaddr*>(&client_addr), &addrlen);
        if (client < 0) {
            if (errno == EINTR) continue;
            throw std::system_error(errno, std::generic_category(), "accept() failed");
        }
    } catch (const std::exception& e) {
        std::cerr << "Accept error: " << e.what() << "\n";
        if (db) db->addLog("Accept error: " + std::string(e.what()));
        continue;
    }

        std::thread([this, client, client_addr]() {
            try {
                m_handler(client, client_addr);
            } catch (...) { }
            ::shutdown(client, SHUT_RDWR);
            ::close(client);
        }).detach();
    }

}

void Server::setupSignalHandler() {
    struct sigaction sa;
    sa.sa_handler = [](int) {
        while (waitpid(-1, nullptr, WNOHANG) > 0);
    };
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, nullptr);
}
