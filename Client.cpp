#include "Client.h"
#include <stdexcept>
#include <system_error>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>       
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

Client::Client(const std::string& host, uint16_t port) noexcept
    : m_host(host), m_port(port), m_sockfd(-1)
{}

Client::Client(Client&& other) noexcept
    : m_host(std::move(other.m_host)),
      m_port(other.m_port),
      m_sockfd(other.m_sockfd)
{
    other.m_sockfd = -1;
}

Client& Client::operator=(Client&& other) noexcept {
    if (this != &other) {
        close();
        m_host = std::move(other.m_host);
        m_port = other.m_port;
        m_sockfd = other.m_sockfd;
        other.m_sockfd = -1;
    }
    return *this;
}

Client::~Client() {
    close();
}

void Client::ensureNotConnected() const {
    if (m_sockfd >= 0) throw std::logic_error("already connected");
}

void Client::ensureConnected() const {
    if (m_sockfd < 0) throw std::logic_error("not connected");
}

void Client::connectTo() {
    ensureNotConnected();

    struct addrinfo hints{};
    hints.ai_family = AF_UNSPEC;      
    hints.ai_socktype = SOCK_STREAM;  

    char portstr[32];
    snprintf(portstr, sizeof(portstr), "%u", m_port);

    struct addrinfo* res = nullptr;
    int rc = getaddrinfo(m_host.c_str(), portstr, &hints, &res);
    if (rc != 0) {
        throw std::runtime_error(std::string("getaddrinfo: ") + gai_strerror(rc));
    }

    int sockfd = -1;
    struct addrinfo* rp;
    for (rp = res; rp != nullptr; rp = rp->ai_next) {
        sockfd = ::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd < 0) continue;

        if (::connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;
        }

        ::close(sockfd);
        sockfd = -1;
    }

    freeaddrinfo(res);

    if (sockfd < 0) {
        throw std::system_error(errno, std::generic_category(), "connect failed");
    }

    m_sockfd = sockfd;
}

size_t Client::sendAll(const void* data, size_t len) {
    ensureConnected();
    const char* p = static_cast<const char*>(data);
    size_t total = 0;

    while (total < len) {
        ssize_t n = send(m_sockfd, p + total, len - total, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            throw std::system_error(errno, std::generic_category(), "send failed");
        }
        total += static_cast<size_t>(n);
    }
    return total;
}

ssize_t Client::recvSome(void* buffer, size_t max_len) {
    ensureConnected();
    ssize_t n = recv(m_sockfd, buffer, max_len, 0);
    if (n < 0) {
        if (errno == EINTR) return recvSome(buffer, max_len);
        throw std::system_error(errno, std::generic_category(), "recv failed");
    }
    return n;
}

void Client::close() {
    if (m_sockfd >= 0) {
        ::shutdown(m_sockfd, SHUT_RDWR);
        ::close(m_sockfd);
        m_sockfd = -1;
    }
}
