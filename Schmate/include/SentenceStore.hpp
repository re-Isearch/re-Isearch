#pragma once

#include <memory>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>
#include <stdexcept>

#include "FileStreamCache.hpp" 
#include "OffsetFile.hpp"

class SentenceStore {
public:
    virtual ~SentenceStore() = default;
    virtual bool    open(const std::string& filepath) = 0;
    virtual void    close() = 0;
    virtual int64_t append(std::string_view text) = 0;
    virtual int64_t append_from(SentenceStore& source) = 0;
    
    // Returns text for a specific entry. 
    // In Embedded mode, this might talk to re-Isearch instead.
    virtual std::string get(const OffsetEntry &e) = 0;

    virtual size_t size() const = 0;
    virtual void flush() = 0;
};

class FileSentenceStore : public SentenceStore {
private:
    FILE* fd = nullptr;
    std::string path;

public:

    FileSentenceStore() {}
    FileSentenceStore(const std::string& filepath) {
        if (!this->open(filepath)) throw std::runtime_error("Failed to open sentence file: " + path);
    }

    ~FileSentenceStore() {
       close();
    }

   bool open(const std::string& filepath) override {
     if (!filepath.empty()) path = filepath;
     if (fd) ::fclose(fd); 
     // We open via this call to make sure we get a high enough
     // handle that external libs don't close them on us.
     fd = schmate_util::fopen_high(path.c_str(), "a+b");
     return fd != nullptr;
   }

   void close() override {
    if (fd) {
      fflush(fd); fclose(fd);
      if (file_size(path) == 0)
	::unlink(path.c_str()); // Don't leave empty files around
    }
    fd = nullptr;
   }

    int64_t append(std::string_view text) override {
        if (fd == nullptr) return -1;
        fseek(fd, 0, SEEK_END);
        int64_t start_pos = ftell(fd);
        if (fwrite(text.data(), 1, text.size(), fd) != text.size()) {
            return -1; 
        }
        return start_pos;
    }

    // --- Low-Level Interface ---
    std::string get(off_t start, off_t end) {
        size_t len = (size_t)(end - start);
        if (len <= 0 || fd == nullptr) return "";
        
        std::string res;
        res.resize(len);
        
        // Use pread here if you want thread-safety without locking, 
        // but for now, we'll stick to fseek/fread for portability.
        fseek(fd, start, SEEK_SET);
        size_t read_bytes = fread(res.data(), 1, len, fd);
        res.resize(read_bytes);
        return res;
    }
    // --- High-Level Interface ---
    // This is what the BertIndex uses to stay abstract
    std::string get(const OffsetEntry& e) override {
        return get(e.file_start, e.file_end);
    }

    int64_t append_from(SentenceStore& source) override {
        // High-speed binary transfer
        FileSentenceStore* src_ptr = dynamic_cast<FileSentenceStore*>(&source);
        if (!src_ptr) throw std::runtime_error("Source is not a FileSentenceStore");

        fseek(this->fd, 0, SEEK_END);
        int64_t start_offset = ftell(this->fd);

        fseek(src_ptr->fd, 0, SEEK_SET);

        std::vector<char> buffer(1024 * 1024); // 1MB chunk size
        size_t bytes_read;
        while ((bytes_read = fread(buffer.data(), 1, buffer.size(), src_ptr->fd)) > 0) {
            if (fwrite(buffer.data(), 1, bytes_read, this->fd) != bytes_read) {
                throw std::runtime_error("Critical write failure during shard merge");
            }
        }
        
        fflush(this->fd);
        return start_offset;
    }

    size_t size() const override {
        fseek(fd, 0, SEEK_END);
        return (size_t)ftell(fd);
    }

    void flush() override {
        if (fd) fflush(fd);
    }
};


// The whole point of the SentenceStore is to abstract the file I/O to
// the sentences file away so that with the bridge integrating into re-Isearch
// we don't need or want the sentences to be written to disk.
//
// - With File it goes to disk.
// - With reIsearch the append does nothing.. The entries in Offset of the
//   start and end don't matter..  What matters is the sid and the span. FC  = {sid, sid+span}
//   and a call via IDB to get the contents via the Peer which delivers the whole tag contents.
//
class SentenceStoreFactory {
public:
    // Standard File creation
    static std::unique_ptr<SentenceStore> CreateFileStore(const std::string& path) {
        return std::make_unique<FileSentenceStore>(path);
    }

    // Bridge creation for Embedded mode
    static std::unique_ptr<SentenceStore> CreateBridgeStore(void* parent);

   // We'll pass Parent as a void * down the line to BertIndex from the EmbeddingIndexer bridge.
   // See opaque_ptr in BertIndex
   static std::unique_ptr<SentenceStore> Create(const std::string& uri, void* parent = nullptr) {
        if (parent != nullptr) return CreateBridgeStore(parent);
        return CreateFileStore(uri);
    }
};



