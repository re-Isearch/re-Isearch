#pragma once

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <list>
#include <memory>
#include <string>
#include <stdexcept>
#include <mutex>

#include <ostream>
#include <streambuf>
#include <cstdio>


namespace schmate_util {
  FILE * fopen_high(const char *path, const char *mode) ;
}


#ifdef __GLIBCXX__
#include <ext/stdio_filebuf.h>
#endif

// ---- Portable fallback streambuf ----------------------------------------

struct FILEoutbuf : std::streambuf {
    explicit FILEoutbuf(FILE* f) : f(f) {}
protected:
    int overflow(int c) override {
        return (c != EOF && fputc(c, f) != EOF) ? c : EOF;
    }
    std::streamsize xsputn(const char* s, std::streamsize n) override {
        return static_cast<std::streamsize>(fwrite(s, 1, n, f));
    }
private:
    FILE* f;
};

// ---- Unified ostream -----------------------------------------------------

struct FILEostream : std::ostream {
    explicit FILEostream(FILE* f)
        : std::ostream(nullptr)
#ifdef __GLIBCXX__
        , sb(fileno(f), std::ios::out | std::ios::binary)
#else
        , sb(f)
#endif
    {
        rdbuf(&sb);
    }
private:
#ifdef __GLIBCXX__
    __gnu_cxx::stdio_filebuf<char> sb;
#else
    FILEoutbuf sb;
#endif
};

class FileStreamCache {
public:
    // Constructor with configurable cache size
    explicit FileStreamCache(size_t max_cache_size = 10) 
        : max_size_(max_cache_size) {
        if (max_size_ == 0) {
            throw std::invalid_argument("Cache size must be greater than 0");
        }
    }

    // Get a file stream, opening it if necessary
    std::fstream& get(const std::string& filename, 
                      std::ios_base::openmode mode = std::ios::in | std::ios::out) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = cache_map_.find(filename);
        
        if (it != cache_map_.end()) {
            // File is in cache - move to front (most recently used)
            lru_list_.splice(lru_list_.begin(), lru_list_, it->second.list_iter);
            return *it->second.stream;
        }
        
        // File not in cache - need to open it
        return openAndCache(filename, mode);
    }

    // Flush a specific file (write buffer to disk, keep in cache)
    void flush(const std::string& filename) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = cache_map_.find(filename);
        if (it != cache_map_.end()) {
            it->second.stream->flush();
        }
    }

    // Flush all cached files
    void flushAll() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        for (auto& entry : cache_map_) {
            entry.second.stream->flush();
        }
    }

    // Close a specific file and remove from cache
    void close(const std::string& filename) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = cache_map_.find(filename);
        if (it != cache_map_.end()) {
            it->second.stream->close();
            lru_list_.erase(it->second.list_iter);
            cache_map_.erase(it);
        }
    }

    // Close all files and clear cache
    void closeAll() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        for (auto& entry : cache_map_) {
            entry.second.stream->close();
        }
        lru_list_.clear();
        cache_map_.clear();
    }

    // Check if a file is in cache
    bool isCached(const std::string& filename) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return cache_map_.find(filename) != cache_map_.end();
    }

    // Get current cache size
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return cache_map_.size();
    }

    // Get maximum cache size
    size_t maxSize() const {
        return max_size_;
    }

    // Destructor ensures all files are closed
    ~FileStreamCache() {
        closeAll();
    }

private:
    struct CacheEntry {
        std::unique_ptr<std::fstream> stream;
        std::list<std::string>::iterator list_iter;
        std::ios_base::openmode mode;
    };

    std::fstream& openAndCache(const std::string& filename, 
                                std::ios_base::openmode mode) {
        // Evict least recently used if cache is full
        if (cache_map_.size() >= max_size_) {
            evictLRU();
        }

        // Create new stream
        auto stream = std::make_unique<std::fstream>();
        stream->open(filename, mode);
        
        if (!stream->is_open()) {
            throw std::runtime_error("Failed to open file: " + filename);
        }

        // Add to front of LRU list (most recently used)
        lru_list_.push_front(filename);
        
        // Create cache entry
        CacheEntry entry;
        entry.stream = std::move(stream);
        entry.list_iter = lru_list_.begin();
        entry.mode = mode;

        // Add to map and return reference
        auto result = cache_map_.emplace(filename, std::move(entry));
        return *result.first->second.stream;
    }

    void evictLRU() {
        if (lru_list_.empty()) return;

        // Get least recently used (back of list)
        const std::string& lru_file = lru_list_.back();
        
        // Close and remove from cache
        auto it = cache_map_.find(lru_file);
        if (it != cache_map_.end()) {
            it->second.stream->close();
            cache_map_.erase(it);
        }
        
        lru_list_.pop_back();
    }

    size_t max_size_;
    std::list<std::string> lru_list_;  // Front = most recent, Back = least recent
    std::unordered_map<std::string, CacheEntry> cache_map_;
    mutable std::mutex mutex_;  // Thread safety
};

/*
// Example usage
int main() {
    try {
        FileStreamCache cache(3);  // Cache up to 3 files
        
        // Open and write to files
        auto& file1 = cache.get("test1.txt", std::ios::out);
        file1 << "Hello from file 1" << std::endl;
        
        auto& file2 = cache.get("test2.txt", std::ios::out);
        file2 << "Hello from file 2" << std::endl;
        
        auto& file3 = cache.get("test3.txt", std::ios::out);
        file3 << "Hello from file 3" << std::endl;
        
        std::cout << "Cache size: " << cache.size() << std::endl;
        
        // Access file1 again (moves it to front of LRU)
        auto& file1_again = cache.get("test1.txt", std::ios::in | std::ios::out);
        file1_again << "Updated file 1" << std::endl;
        
        // This will evict file2 (least recently used)
        auto& file4 = cache.get("test4.txt", std::ios::out);
        file4 << "Hello from file 4" << std::endl;
        
        std::cout << "Cache size after eviction: " << cache.size() << std::endl;
        std::cout << "Is file2 cached? " << (cache.isCached("test2.txt") ? "Yes" : "No") << std::endl;
        std::cout << "Is file1 cached? " << (cache.isCached("test1.txt") ? "Yes" : "No") << std::endl;
        
        // Close specific file
        cache.close("test1.txt");
        std::cout << "Cache size after closing file1: " << cache.size() << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
*/
