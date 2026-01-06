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
#include <signal.h>
#include <unistd.h>

static void timeout_handler(int sig)
{
    throw std::runtime_error("RPC timeout!");
}

Client::Client(const std::string &host, uint16_t port) noexcept
    : m_host(host), m_port(port), m_sockfd(-1)
{
}

Client::Client(Client &&other) noexcept
    : m_host(std::move(other.m_host)),
      m_port(other.m_port),
      m_sockfd(other.m_sockfd)
{
    other.m_sockfd = -1;
}

Client &Client::operator=(Client &&other) noexcept
{
    if (this != &other)
    {
        close();
        m_host = std::move(other.m_host);
        m_port = other.m_port;
        m_sockfd = other.m_sockfd;
        other.m_sockfd = -1;
    }
    return *this;
}

Client::~Client()
{
    close();
}

void Client::ensureNotConnected() const
{
    if (m_sockfd >= 0)
        throw std::logic_error("already connected");
}

void Client::ensureConnected() const
{
    if (m_sockfd < 0)
        throw std::logic_error("not connected");
}

void Client::connectTo()
{
    ensureNotConnected();

    struct addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    char portstr[32];
    snprintf(portstr, sizeof(portstr), "%u", m_port);

    struct addrinfo *res = nullptr;
    int rc = getaddrinfo(m_host.c_str(), portstr, &hints, &res);
    if (rc != 0)
    {
        throw std::runtime_error(std::string("getaddrinfo: ") + gai_strerror(rc));
    }

    int sockfd = -1;
    struct addrinfo *rp;
    for (rp = res; rp != nullptr; rp = rp->ai_next)
    {
        sockfd = ::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd < 0)
            continue;

        if (::connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0)
        {
            break;
        }

        ::close(sockfd);
        sockfd = -1;
    }

    freeaddrinfo(res);

    if (sockfd < 0)
    {
        throw std::system_error(errno, std::generic_category(), "connect failed");
    }

    m_sockfd = sockfd;
}

size_t Client::sendAll(const void *data, size_t len)
{
    ensureConnected();
    const char *p = static_cast<const char *>(data);
    size_t total = 0;

    while (total < len)
    {
        ssize_t n = send(m_sockfd, p + total, len - total, 0);
        if (n < 0)
        {
            if (errno == EINTR)
                continue;
            throw std::system_error(errno, std::generic_category(), "send failed");
        }
        total += static_cast<size_t>(n);
    }
    return total;
}

std::string Client::callWithTimeout(const std::string &msg, unsigned int seconds)
{
    if (!isConnected())
        connectTo();

    struct sigaction sa{};
    sa.sa_handler = timeout_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, nullptr);

    alarm(seconds);

    sendString(msg);

    char buf[1024]{};
    ssize_t n = recvSome(buf, sizeof(buf));

    alarm(0);

    return std::string(buf, n);
}

void Client::trace(const std::string& command, 
                   std::function<void(const std::string&)> traceCallback,
                   std::function<void(const std::string&)> outCallback) {
    ensureConnected();
    
    std::string msg = "TRACE " + command;
    ssize_t sent = send(m_sockfd, msg.c_str(), msg.size(), 0);
    if (sent < 0) {
        throw std::system_error(errno, std::generic_category(), "send() failed");
    }
    
    char buf[4096];
    for (;;) {
        ssize_t r = recv(m_sockfd, buf, sizeof(buf) - 1, 0);
        if (r > 0) {
            buf[r] = '\0';
            std::string chunk(buf);
            
            size_t endPos = chunk.find("TRACE_END\n");
            if (endPos != std::string::npos) {
                chunk = chunk.substr(0, endPos);
                
                if (!chunk.empty()) {
                    
                    size_t start = 0;
                    size_t pos = 0;
                    while ((pos = chunk.find('\n', start)) != std::string::npos) {
                        std::string line = chunk.substr(start, pos - start);
                        if (line.rfind("TRACE:", 0) == 0) {
                            traceCallback(line.substr(6));
                        } else if (line.rfind("OUT:", 0) == 0) {
                            outCallback(line.substr(4));
                        }
                        start = pos + 1;
                    }
                }
                break;
            }
            
            
            size_t start = 0;
            size_t pos = 0;
            while ((pos = chunk.find('\n', start)) != std::string::npos) {
                std::string line = chunk.substr(start, pos - start);
                if (line.rfind("TRACE:", 0) == 0) {
                    traceCallback(line.substr(6));
                } else if (line.rfind("OUT:", 0) == 0) {
                    outCallback(line.substr(4));
                }
                start = pos + 1;
            }
        } else if (r == 0) {
            throw std::runtime_error("Server disconnected during trace");
        } else {
            if (errno == EINTR) continue;
            throw std::system_error(errno, std::generic_category(), "recv() failed");
        }
    }
}

ssize_t Client::recvSome(void *buffer, size_t max_len)
{
    ensureConnected();
    ssize_t n = recv(m_sockfd, buffer, max_len, 0);
    if (n < 0)
    {
        if (errno == EINTR)
            return recvSome(buffer, max_len);
        throw std::system_error(errno, std::generic_category(), "recv failed");
    }
    return n;
}

void Client::close()
{
    if (m_sockfd >= 0)
    {
        ::shutdown(m_sockfd, SHUT_RDWR);
        ::close(m_sockfd);
        m_sockfd = -1;
    }
}
