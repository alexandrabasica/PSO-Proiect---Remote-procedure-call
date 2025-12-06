#include "Server.h"
#include "Tracer.h"
#include <stdexcept>
#include <system_error>
#include <cerrno>
#include <cstring>
#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <csignal>
#include <sys/wait.h>

Server::Server(uint16_t port, Database* database) noexcept
    : m_port(port), m_listen_fd(-1), m_client_fd(-1), m_running(false), db(database)
{
    m_handler = [this](int client_fd, const sockaddr_in&) {
        constexpr size_t BUF_SZ = 1024;
        char buf[BUF_SZ];

        for (;;) {
            ssize_t r = recv(client_fd, buf, BUF_SZ - 1, 0);
            if (r > 0) {
                buf[r] = '\0';
                std::string msg(buf);
                
                if (msg.rfind("TRACE ", 0) == 0) {
                    std::string cmd = msg.substr(6);
                    if (db) db->addLog("Tracing command: " + cmd);
                    
                    auto sendCallback = [client_fd](const TraceEvent& evt) {
                        std::string line = evt.type + " [" + std::to_string(evt.pid) + "]: " + evt.details + "\n";
                        send(client_fd, line.c_str(), line.size(), 0);
                    };
                    
                    Tracer tracer(cmd, sendCallback);
                    tracer.run();
                    
                    std::string endMsg = "TRACE_END\n";
                    send(client_fd, endMsg.c_str(), endMsg.size(), 0);
                } else {
                    // Echo back
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
    if (m_listen_fd >= 0) return; // deja deschis

    setupSignalHandler();

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
        continue; // Try again instead of crashing
    }

    pid_t pid = fork();
    if (pid == 0) 
    { // proces copil
        ::close(m_listen_fd); // copilul nu are nevoie de socketul de ascultare
        
        // Reset SIGCHLD to default so waitpid works correctly in Tracer
        signal(SIGCHLD, SIG_DFL);
        
        try {
            m_handler(client, client_addr);
        } catch (...) { }
        ::shutdown(client, SHUT_RDWR);
        ::close(client);
        _exit(0); // copilul moare dupa terminarea clientului
    } else if (pid > 0) 
    { // parinte
        ::close(client); // parinte inchide socketul clientului, copilul se ocupa
    } else {
        // fork esuat
        ::shutdown(client, SHUT_RDWR);
        ::close(client);
        if(db) db->addLog("Fork failed for new client");
    }
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
