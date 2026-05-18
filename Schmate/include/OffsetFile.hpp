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
    int64_t file_start;  // byte offset in sentences file
    int64_t file_end;    // byte offset in sentences file

    uint32_t  start_tok;   // token start in original sentence
    uint32_t  end_tok;     // token end in original sentence
    uint32_t  span;    // sid = ID start . sid + span = ID end

    // Increments file offsets by a given delta
    OffsetEntry& add_offset(off_t delta) {
        file_start += delta;
        file_end += delta;
	return *this;
    }
    
    // Checks if the entry is actually populated
    bool is_valid() const {
        return sid != 0 || file_start != 0 || file_end != 0;
    }
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

    void add_offset(off_t offset); // Increase by offset the file_start and file_end


    // The sentence id associated with a given label
    size_t get_sid(size_t label) const;
    // collect all entries for a given SID
    std::vector<std::pair<size_t, OffsetEntry>> find_by_sid(int64_t sid) const;
    std::vector<size_t> find_labels_by_sid(int64_t sid) const; // Just the labels

    OffsetEntry get(size_t label) const;
    OffsetEntry* get_mut(size_t label) const;

    size_t entry_address(size_t label) const {
      std::shared_lock<std::shared_mutex> rl(rwmutex);
      return header_size + label * entry_size;
    }

    void set(size_t label, const OffsetEntry &entry);
    void flush(size_t label = 0);

    bool validate_offsets(bool fix = false, bool verbose = true);

    static size_t current_capacity(size_t filesize) {
      if (filesize > 16)
        return (filesize - 16)/sizeof(OffsetEntry);
      return 0;
    }

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
    const size_t entry_size = sizeof(OffsetEntry);
    const size_t header_size = 16; // sizeof(magic) + sizeof(entry_size)
    size_t max_entries;
    size_t used_entries;     // number actually used
    mutable std::shared_mutex rwmutex; // readers/writers lock

};


