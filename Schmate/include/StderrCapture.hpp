#include "Logger.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <thread>
#include <atomic>

class StderrCapture {
public:
    static StderrCapture& instance() {
        static StderrCapture capture;
        return capture;
    }

    // Start capturing stderr
    bool start() {
        if (capturing) return false;

        // Create a pipe
        if (pipe(pipe_fds) == -1) {
            Logger::instance().error("Failed to create pipe for stderr capture");
            return false;
        }

        // Save original stderr
        original_stderr = dup(STDERR_FILENO);
        if (original_stderr == -1) {
            close(pipe_fds[0]);
            close(pipe_fds[1]);
            Logger::instance().error("Failed to duplicate stderr");
            return false;
        }

        // Redirect stderr to pipe write end
        if (dup2(pipe_fds[1], STDERR_FILENO) == -1) {
            close(pipe_fds[0]);
            close(pipe_fds[1]);
            close(original_stderr);
            Logger::instance().error("Failed to redirect stderr");
            return false;
        }

        // Make read end non-blocking
        int flags = fcntl(pipe_fds[0], F_GETFL, 0);
        fcntl(pipe_fds[0], F_SETFL, flags | O_NONBLOCK);

        capturing = true;

        // Start reader thread
        reader_thread = std::thread(&StderrCapture::read_loop, this);

        Logger::instance().info("Started stderr capture");
        return true;
    }

    // Stop capturing and restore stderr
    void stop() {
        if (!capturing) return;

        capturing = false;

        // Restore original stderr
        dup2(original_stderr, STDERR_FILENO);
        close(original_stderr);
        close(pipe_fds[1]);

        // Wait for reader thread
        if (reader_thread.joinable()) {
            reader_thread.join();
        }

        close(pipe_fds[0]);

        Logger::instance().info("Stopped stderr capture");
    }

    ~StderrCapture() {
        stop();
    }

private:
    StderrCapture() : capturing(false), original_stderr(-1) {
        pipe_fds[0] = -1;
        pipe_fds[1] = -1;
    }

    void read_loop() {
        char buffer[4096];
        std::string line_buffer;

        while (capturing) {
            ssize_t n = read(pipe_fds[0], buffer, sizeof(buffer) - 1);
            
            if (n > 0) {
                buffer[n] = '\0';
                line_buffer += buffer;

                // Process complete lines
                size_t pos;
                while ((pos = line_buffer.find('\n')) != std::string::npos) {
                    std::string line = line_buffer.substr(0, pos);
                    if (!line.empty()) {
                        // Log the line from stderr
                        // You can parse it to determine log level
                        if (line.find("error") != std::string::npos || 
                            line.find("Error") != std::string::npos ||
                            line.find("ERROR") != std::string::npos) {
                            LOG_ERROR_S() << "[STDERR] " << line;
                        } else if (line.find("warning") != std::string::npos ||
                                   line.find("Warning") != std::string::npos ||
                                   line.find("WARN") != std::string::npos) {
                            LOG_WARN_S() << "[STDERR] " << line;
                        } else {
                            LOG_INFO_S() << "[STDERR] " << line;
                        }
                    }
                    line_buffer.erase(0, pos + 1);
                }
            } else if (n == -1 && errno == EAGAIN) {
                // No data available, sleep briefly
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        // Flush remaining buffer
        if (!line_buffer.empty()) {
            LOG_INFO_S() << "[STDERR] " << line_buffer;
        }
    }

    std::atomic<bool> capturing;
    int pipe_fds[2];
    int original_stderr;
    std::thread reader_thread;
};

// Usage example in main.cpp:
/*
int main(int argc, char* argv[]) {
    // Setup logger first
    Logger::instance().set_level(LogLevel::INFO);
    Logger::instance().enable_console(true);
    Logger::instance().enable_file("sbert.log");

    // Start capturing stderr from libraries
    StderrCapture::instance().start();

    // Now all stderr output from libraries will be captured
    // and logged through Logger

    // ... your code that uses libraries ...

    // Cleanup happens automatically in destructor
    // or call: StderrCapture::instance().stop();

    return 0;
}
*/
