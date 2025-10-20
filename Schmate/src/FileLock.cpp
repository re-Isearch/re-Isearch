#include "FileLock.hpp"
#include "Logger.hpp"
#include "Util.hpp"

#include <iostream>
#include <chrono>
#include <thread>
#include <sys/stat.h>
#include <unistd.h>


// Method 1: Simple polling with stat
static bool wait_for_file_removal_simple(const std::string& path, 
                                   std::chrono::milliseconds poll_interval = std::chrono::milliseconds(100)) {
    while (file_size(path) > 0) {
        // File still exists
        std::this_thread::sleep_for(poll_interval);
    }
    return true;
}

// Method 2: With timeout
static bool wait_for_file_removal_timeout(const std::string& path,
                                   std::chrono::milliseconds timeout,
                                   std::chrono::milliseconds poll_interval = std::chrono::milliseconds(100)) {
    auto start = std::chrono::steady_clock::now();
    
    while (file_size(path) > 0) {
        // Check timeout
        auto elapsed = std::chrono::steady_clock::now() - start;
        if (elapsed >= timeout) {
            return false;  // Timeout
        }
        
        std::this_thread::sleep_for(poll_interval);
    }
    
    return true;  // File removed or zero'd
}

// Method 3: With callback
/*
    // Method 3: With progress callback
    std::cout << "\n=== Method 3: With callback ===\n";
    bool result = wait_for_file_removal_callback(path, [](int count) {
        if (count % 10 == 0) {
            std::cout << "Still waiting... (poll #" << count << ")\n";
        }
    });

*/
static bool wait_for_file_removal_callback(const std::string& path,
                                    std::function<void(int)> on_poll = nullptr,
                                    std::chrono::milliseconds poll_interval = std::chrono::milliseconds(100)) {
    int poll_count = 0;
    
    while (file_size(path) > 0) {
        poll_count++;
        if (on_poll) {
            on_poll(poll_count);
        }
        std::this_thread::sleep_for(poll_interval);
    }
    
    return true;
}

#ifdef __linux__
// Method 4: Linux inotify (efficient, no polling)
#include <sys/inotify.h>
#include <limits.h>
#include <poll.h>

static bool wait_for_file_removal_inotify(const std::string& path,
                                   std::chrono::milliseconds timeout = std::chrono::milliseconds(0)) {
    // Check if file exists first
    struct stat buffer;
    if (file_size(path) <= 0) {
        return true;  // Already removed
    }
    
    // Initialize inotify
    int fd = inotify_init1(IN_NONBLOCK);
    if (fd < 0) {
        std::cerr << "Failed to initialize inotify\n";
        return false;
    }
    
    // Watch the file for deletion
    int wd = inotify_add_watch(fd, path.c_str(), IN_DELETE_SELF | IN_ATTRIB);
    if (wd < 0) {
        std::cerr << "Failed to add watch\n";
        close(fd);
        return false;
    }
    
    // Wait for event
    struct pollfd pfd = { fd, POLLIN, 0 };
    int timeout_ms = timeout.count() > 0 ? timeout.count() : -1;
    
    while (true) {
        int ret = poll(&pfd, 1, timeout_ms);
        
        if (ret < 0) {
            std::cerr << "poll error\n";
            break;
        } else if (ret == 0) {
            // Timeout
            inotify_rm_watch(fd, wd);
            close(fd);
            return false;
        }
        
        // Read events
        char buf[4096] __attribute__ ((aligned(__alignof__(struct inotify_event))));
        ssize_t len = read(fd, buf, sizeof(buf));
        
        if (len < 0 && errno != EAGAIN) {
            std::cerr << "read error\n";
            break;
        }
        
        // Process events
        const struct inotify_event *event;
        for (char *ptr = buf; ptr < buf + len; 
             ptr += sizeof(struct inotify_event) + event->len) {
            event = (const struct inotify_event *) ptr;
            
            if (event->mask & IN_DELETE_SELF) {
                // File deleted!
                inotify_rm_watch(fd, wd);
                close(fd);
                return true;
            }
            
            if (event->mask & IN_ATTRIB) {
                // Check if file still exists (might have been replaced)
                if (file_size(path) > 0) {
                    inotify_rm_watch(fd, wd);
                    close(fd);
                    return true;
                }
            }
        }
    }
    
    inotify_rm_watch(fd, wd);
    close(fd);
    return false;
}
#endif

#ifdef __APPLE__
// Method 4: macOS kqueue (efficient, no polling)
#include <sys/event.h>
#include <fcntl.h>

static bool wait_for_file_removal_kqueue(const std::string& path, std::chrono::milliseconds timeout) {
    if (file_size(path) <= 0)
	return true;

    // Open the file
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        return true;  // File can't be open
    }
    
    // Create kqueue
    int kq = kqueue();
    if (kq < 0) {
        std::cerr << "Failed to create kqueue\n";
        close(fd);
        return false;
    }
    
    // Set up event for file deletion
    struct kevent change;
    EV_SET(&change, fd, EVFILT_VNODE, EV_ADD | EV_ENABLE | EV_ONESHOT,
           NOTE_DELETE | NOTE_REVOKE, 0, NULL);
    
    struct kevent event;
    struct timespec ts;
    struct timespec *ts_ptr = nullptr;
    
    if (timeout.count() > 0) {
        ts.tv_sec = timeout.count() / 1000;
        ts.tv_nsec = (timeout.count() % 1000) * 1000000;
        ts_ptr = &ts;
    }
    
    // Wait for event
    int nev = kevent(kq, &change, 1, &event, 1, ts_ptr);
    
    close(fd);
    close(kq);
    
    if (nev < 0) {
        std::cerr << "kevent error\n";
        return false;
    } else if (nev == 0) {
        // Timeout
        return false;
    }
    
    // Check if it was a delete event
    if (event.fflags & (NOTE_DELETE | NOTE_REVOKE)) {
        return true;
    }
    
    return false;
}
#endif

// Platform-independent wrapper
bool wait_for_file_removal(const std::string& path, std::chrono::milliseconds timeout,  bool use_native) {
#ifdef __linux__
    if (use_native) {
        return wait_for_file_removal_inotify(path, timeout);
    }
#elif defined(__APPLE__)
    if (use_native) {
        return wait_for_file_removal_kqueue(path, timeout);
    }
#endif
    
    // Fall back to polling
    if (timeout.count() > 0) {
        return wait_for_file_removal_timeout(path, timeout);
    } else {
        return wait_for_file_removal_simple(path);
    }
}
