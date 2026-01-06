#ifndef SERVER_H
#define SERVER_H
#pragma once
#include <cstdint>
#include <functional>
#include <atomic>
#include "tinyxml2.h"
using namespace tinyxml2;
#include "Database.h"

struct sockaddr_in;

class Server {
public:
    using ClientHandler = std::function<void(int, const sockaddr_in&)>;

    explicit Server(uint16_t port = 12345, Database* database = nullptr) noexcept;
    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    Server(Server&&) noexcept;
    Server& operator=(Server&&) noexcept;

    void open();

    void close();

    void serveOneClient();

    void serveForever();

    void setClientHandler(ClientHandler h) { m_handler = std::move(h); }

    bool isOpen() const noexcept { return m_listen_fd >= 0; }
    uint16_t port() const noexcept { return m_port; }

private:
    void setupSignalHandler();
    void ensureOpen() const;
    void closeClientIfOpen(); 

private:
    uint16_t m_port;
    int m_listen_fd;
    int m_client_fd;
    std::atomic<bool> m_running;
    ClientHandler m_handler;
    Database* db;
};

#endif
