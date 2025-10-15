// include/OffsetFile.hpp
#pragma once
#include <cstdint>
#include <string>
#include <functional>
#include <shared_mutex>
#include <mutex>



/*

[Header: magic string "SBIDXv1", uint64_t entry_size]
[Entry0: label0 SID0 start_tok end_tok file_start file_end]
[Entry1: ...]
...
*/

// This structure helps up maintain the sentence IDs
// these are the GPs in re-Isearch
struct OffsetEntry {
    int64_t sid;         // persistent sentence ID
    size_t  start_tok;   // token start in original sentence
    size_t  end_tok;     // token end in original sentence
    int64_t file_start;  // byte offset in sentences file
    int64_t file_end;    // byte offset in sentences file
};


// include/OffsetFile.hpp
#pragma once
#include <string>
#include <cstdint>
#include <stdexcept>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

class OffsetFile {
public:
    OffsetFile(const std::string &path, size_t max_entries);
    ~OffsetFile();

    void resize(size_t new_capacity);

    // iterate through all valid entries
    void for_each(const std::function<void(size_t label, const OffsetEntry &)> &fn) const;

    // collect all entries for a given SID
    std::vector<std::pair<size_t, OffsetEntry>> find_by_sid(int64_t sid) const;

    OffsetEntry get(size_t label) const;
    OffsetEntry* get_mut(size_t label) const;

    size_t entry_address(size_t label) const {
      std::shared_lock<std::shared_mutex> rl(rwmutex);
      return header_size + label * entry_size;
    }

    void set(size_t label, const OffsetEntry &entry);
    void flush(size_t label = 0);

    bool validate_offsets(bool fix = false, bool verbose = true);

private:
    size_t detect_used_entries() const;
    size_t capacity(size_t length = 0) const {
      if (length == 0) length = filesize;
      return (length - header_size)/entry_size;
    }
    size_t maplen(size_t entries = 0) const {
      if (entries == 0) entries = max_entries;
      return header_size + entries * entry_size;
    }
    int fd = -1;
    void *map = nullptr;
    size_t filesize = 0;
    size_t header_size = 16; // magic + entry_size
    size_t entry_size = sizeof(OffsetEntry);
    size_t max_entries;
    size_t used_entries;     // number actually used
    mutable std::shared_mutex rwmutex; // readers/writers lock
};


