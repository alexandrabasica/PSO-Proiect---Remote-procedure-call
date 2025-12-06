#ifndef TRACER_H
#define TRACER_H

#include <string>
#include <vector>
#include <functional>
#include <sys/types.h>

struct TraceEvent {
    std::string type; // SYSCALL, FORK, SIGNAL, etc.
    pid_t pid;
    std::string details;
};

class Tracer {
public:
    using EventHandler = std::function<void(const TraceEvent&)>;

    Tracer(const std::string& command, EventHandler handler);
    void run();

private:
    std::string m_command;
    EventHandler m_handler;

    void handleSyscall(pid_t pid);
    void handleFork(pid_t pid);
    std::string getSyscallName(long syscall_nr);
};

#endif // TRACER_H
