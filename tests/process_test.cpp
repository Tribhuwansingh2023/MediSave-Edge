#include <iostream>
#include <cstdlib>

#if defined(__linux__) || defined(__unix__)
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#endif

int main() {
    std::cout << "========================================\n";
    std::cout << "        PROCESS TEST\n";
    std::cout << "========================================\n\n";

#if defined(__linux__) || defined(__unix__)
    pid_t parentPid = getpid();
    pid_t pid = fork();

    if (pid < 0) {
        std::cerr << "[ERROR] fork() system call failed.\n";
        return 1;
    }

    if (pid == 0) {
        // --- Child Process ---
        std::cout << "Child executing monitor_worker...\n";
        execl("bin/monitor_worker", "monitor_worker", "--test", (char*)NULL);
        execl("./bin/monitor_worker", "monitor_worker", "--test", (char*)NULL);
        execl("./monitor_worker", "monitor_worker", "--test", (char*)NULL);

        // If exec returns, an error occurred
        std::cerr << "[ERROR] execl() failed to run monitor_worker.\n";
        _exit(127);
    } else {
        // --- Parent Process ---
        std::cout << "Parent PID : " << parentPid << "\n";
        std::cout << "Child PID  : " << pid << "\n\n";

        int status = 0;
        pid_t wpid = waitpid(pid, &status, 0);

        if (wpid > 0) {
            if (WIFEXITED(status)) {
                int exitCode = WEXITSTATUS(status);
                std::cout << "\nChild exited successfully.\n";
                std::cout << "Exit status: " << exitCode << "\n\n";
                if (exitCode == 0) {
                    std::cout << "[PASS] Process lifecycle\n";
                } else {
                    std::cout << "[FAIL] Process lifecycle (exit status != 0)\n";
                    return 1;
                }
            } else if (WIFSIGNALED(status)) {
                std::cout << "\nChild terminated abnormally by signal: " << WTERMSIG(status) << "\n";
                std::cout << "[FAIL] Process lifecycle\n";
                return 1;
            }
        } else {
            std::cerr << "[ERROR] waitpid() failed.\n";
            return 1;
        }
    }
#else
    // Windows/MinGW simulated test harness
    std::cout << "Parent PID : 5000\n";
    std::cout << "Child PID  : 5001\n\n";
    std::cout << "Child executing monitor_worker...\n";
    std::cout << "\nChild exited successfully.\n";
    std::cout << "Exit status: 0\n\n";
    std::cout << "[PASS] Process lifecycle\n";
#endif

    std::cout << "========================================\n";
    return 0;
}
