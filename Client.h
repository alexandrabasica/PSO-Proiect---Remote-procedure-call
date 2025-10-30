#ifndef CLIENT_H
#define CLIENT_H
#pragma once 
#include <cstdint>
#include <string>
#include "tinyxml2.h"
using namespace tinyxml2;
#include "Database.h"

struct sockaddr_in; 

class Client {
public:
    explicit Client(const std::string& host = "127.0.0.1", uint16_t port = 12345) noexcept;

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    Client(Client&&) noexcept;
    Client& operator=(Client&&) noexcept;
    ~Client();
    void connectTo();
    size_t sendAll(const void* data, size_t len);
    size_t sendString(const std::string& s) { return sendAll(s.data(), s.size()); }
    ssize_t recvSome(void* buffer, size_t max_len);
    void close();
    bool isConnected() const noexcept { return m_sockfd >= 0; }
    std::string host() const noexcept { return m_host; }
    uint16_t port() const noexcept { return m_port; }

private:
    void ensureNotConnected() const;
    void ensureConnected() const;

private:
    std::string m_host;
    uint16_t    m_port;
    int         m_sockfd; 
};

#endif 
