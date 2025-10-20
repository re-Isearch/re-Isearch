#pragma once
#include <string>
#include <chrono>
#include <fstream>
#include <stdexcept>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>

class FileLock {
    std::string path;
    int fd = -1;
    bool locked = false;

public:
    explicit FileLock(const std::string &file) : path(file) {
        fd = ::open(file.c_str(), O_RDWR | O_CREAT, 0666);
        if (fd < 0) throw std::runtime_error("Cannot open lock file: " + file);
    }

    bool try_lock() {
        if (flock(fd, LOCK_EX | LOCK_NB) == 0) {
            locked = true;
            write_pid();
            return true;
        }
        return false;
    }

    void unlock() {
        if (locked) {
            flock(fd, LOCK_UN);
            locked = false;
            clear_pid();
        }
    }

    ~FileLock() {
        unlock();
        if (fd >= 0) ::close(fd);
    }

private:
    void write_pid() {
        ::ftruncate(fd, 0);
        std::string pid = std::to_string(::getpid()) + "\n";
        ::write(fd, pid.data(), pid.size());
        ::fsync(fd);
    }

    void clear_pid() {
        ::ftruncate(fd, 0);
    }
};


struct ScopedLock {
    std::function<void()> unlock_fn;
    ScopedLock(std::function<void()> fn) : unlock_fn(std::move(fn)) {}
    ~ScopedLock() { if (unlock_fn) unlock_fn(); }
};
/* 
// then:
acquire_lock();
ScopedLock guard([&]() { release_lock(); });
...
// do your append

*/


bool wait_for_file_removal(const std::string& path,
	std::chrono::milliseconds timeout = std::chrono::milliseconds(0),
	bool use_native = true);
