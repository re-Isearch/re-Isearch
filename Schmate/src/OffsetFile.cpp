// src/OffsetFile.cpp
#include "OffsetFile.hpp"
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include "Logger.hpp"
#include "Util.hpp"

static const char magic[8] = "SBIDXv2";

OffsetFile::OffsetFile(const std::string &path, size_t max_entries)
    : max_entries(max_entries)
{
    fd = ::open(path.c_str(), O_RDWR | O_CREAT, 0644);
    if (fd < 0) throw std::runtime_error("OffsetFile: cannot open " + path);

    // expected size
    filesize = header_size + max_entries * entry_size;

    struct stat st;
    if (fstat(fd, &st) < 0) throw std::runtime_error("OffsetFile: fstat failed");

    const off_t length = st.st_size;

    if (length > filesize) {
      max_entries = capacity(length);
      filesize = header_size + max_entries * entry_size;
    }
    if (length < header_size) {
        // new file, write header + resize
        ::pwrite(fd, magic, 8, 0);
        ::pwrite(fd, &entry_size, sizeof(entry_size), 8);
    } else {
	// Have a header.. read and check magic
        char buffer[8];
        ::pread(fd, buffer, 8, 0);
        if (memcmp(buffer, magic, 8) != 0)
	  LOG_ERROR_S() << "Offset File magic \"" << buffer << "\"!=\"" << magic << "\"";
    }
    if (ftruncate(fd, filesize) == -1)
       throw std::runtime_error("Failed to extend OffsetFile");

    map = ::mmap(nullptr, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) throw std::runtime_error("OffsetFile: mmap failed");
}

OffsetFile::~OffsetFile() {
#if 1
    size_t shrink_to    = detect_used_entries() ;
    size_t new_filesize = maplen(shrink_to + 1);

    flush(0); // Sync
    // Unmap before truncating
    munmap(map, filesize);

    if (shrink_to == 0) {
      ftruncate(fd, 0);
    } else if (filesize > new_filesize) {
      // Shrink physical file
      ftruncate(fd, new_filesize);
      LOG_INFO_S() << "[OffsetFile] shrunk from " << max_entries << " to "
	<< shrink_to << " entries (saved " << (filesize-new_filesize) << " bytes)";
      filesize = new_filesize;
    }
#else
    if (map && map != MAP_FAILED) {
        ::msync(map, filesize, MS_SYNC);
        ::munmap(map, filesize);
    }
#endif
    if (fd >= 0) ::close(fd);
}


void OffsetFile::resize(size_t new_capacity) {
    // std::cerr << "RESIZE\n";
    if (new_capacity <= capacity()) return;

    // Unmap current region
    if (map) munmap(map, filesize);

    // Recompute sizes
    max_entries = new_capacity;
    filesize = maplen(max_entries);

    if (ftruncate(fd, filesize) != 0)
        throw std::runtime_error("Failed to resize offset file");
    // Remap
    map = (char *)mmap(nullptr, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        throw std::runtime_error("Failed to mmap after resize");
    }
    LOG_INFO_S() << "[OffsetFile] resized to " << max_entries << " entries";
}


size_t OffsetFile::get_sid(size_t label) const {
   auto e = get_mut(label);
   if (e) return e->sid;
   return 0;
}

OffsetEntry OffsetFile::get(size_t label) const {
    std::shared_lock<std::shared_mutex> rl(rwmutex);
    if (!map) throw std::runtime_error("OffsetFile not opened");
    if (label >= max_entries) throw std::out_of_range("OffsetFile::get label OOB");
    char *base = (char*)map + header_size;
    return *((OffsetEntry*)(base + label * entry_size));
}

OffsetEntry* OffsetFile::get_mut(size_t label) const {
    if (!map) throw std::runtime_error("OffsetFile not opened");
    if (label >= max_entries) return nullptr;
    char *base = (char*)map + header_size;
    return reinterpret_cast<OffsetEntry*>(base + label * entry_size);
}

void OffsetFile::set(size_t label, const OffsetEntry &entry) {
    // Fast path: check capacity under shared lock
    std::shared_lock<std::shared_mutex> rl(rwmutex);
    // std::cout << "LABEL = " << label << std::endl;
    // std::cout << "MAX_ENTRIES = " << max_entries << std::endl;
    if (label > max_entries) 
      resize(label*3/2 + 100);

    char *base = (char*)map + header_size;
    memcpy(base + label * entry_size, &entry, entry_size);
}

void OffsetFile::flush(size_t label) {
    if (map && map != MAP_FAILED) {
      if (label >= max_entries || label == 0)
        msync(map, filesize, MS_SYNC);
      else
        msync((char*)map + entry_address(label), entry_size, MS_SYNC);
    }
}



void OffsetFile::for_each(const std::function<void(size_t, const OffsetEntry &)> &fn) const {
    std::shared_lock<std::shared_mutex> rl(rwmutex);
    const char *base = (const char*)map + header_size;
    for (size_t i = 0; i < max_entries; i++) {
        const OffsetEntry *e = reinterpret_cast<const OffsetEntry*>(base + i * entry_size);
        if (e->sid != 0 || e->file_start != 0 || e->file_end != 0) {
            fn(i, *e);
        }
    }
}

// While sid is unique because of chunking into different vectors it can have multiple labels
// associated with it
std::vector<std::pair<size_t, OffsetEntry>> OffsetFile::find_by_sid(int64_t sid) const {
    std::vector<std::pair<size_t, OffsetEntry>> results;
    const char *base = (const char*)map + header_size;
    for (size_t i = 0; i < max_entries; i++) {
        const OffsetEntry *e = reinterpret_cast<const OffsetEntry*>(base + i * entry_size);
        if (e->sid == sid) {
            results.emplace_back(i, *e);
        }
    }
    return results;
}

std::vector<size_t> OffsetFile::find_labels_by_sid(int64_t sid) const {
    std::vector<size_t> results;
    const char *base = (const char*)map + header_size;
    // Walk through all the labels
    for (size_t i = 0; i < max_entries; i++) {
        const OffsetEntry *e = reinterpret_cast<const OffsetEntry*>(base + i * entry_size);
        if (e->sid == sid) {
            results.emplace_back(i);
        }       
    }       
    return results;
}   

#include <iostream>

bool OffsetFile::validate_offsets(bool fix, bool verbose) {
    if (!map || map == MAP_FAILED) {
       LOG_FATAL_S()<< "OffsetFile::validate_offsets: no mmap loaded";
       return false;
    }

    size_t bad_count = 0;
    size_t total = 0;

    const char *base = (const char*)map + header_size;
    for (size_t i = 0; i < max_entries; ++i) {
        const OffsetEntry *e = reinterpret_cast<const OffsetEntry*>(base + i * entry_size);
        total++;

        // skip uninitialized
        if (e->sid == 0 && e->file_start == 0 && e->file_end == 0)
            continue;

        bool bad = false;
        if (e->file_end <= e->file_start) bad = true;
        if (e->file_end - e->file_start > (1LL << 31)) bad = true; // >2GB sanity

        if (bad) {
            bad_count++;
            if (verbose) {
                 LOG_WARN_S() << "invalid offset entry label=" << i
                          << " sid=" << e->sid
                          << " range=(" << e->file_start << "," << e->file_end << ")";
            }
            if (fix) {
                OffsetEntry zero{};
                set(i, zero);
            }
        }
    }

    if (verbose) {
         LOG_INFO_S() << "validate_offsets: " << (total - bad_count)
                  << "/" << total << " valid entries"
                  << (fix && bad_count > 0 ? " (auto-fixed invalid ones)" : "");
    }

    return bad_count == 0;
}


// detect used entries without locking (caller must hold lock or accept race)
size_t OffsetFile::detect_used_entries() const
{
    if (!map) return 0;

    const char *base = (const char*)map + header_size; 
    const OffsetEntry *entries = reinterpret_cast<const OffsetEntry *>(base);
    for (size_t i = capacity(); i > 0; --i) {
       if (entries[i - 1].sid != 0) return i;
    }
    return 0;
}

void OffsetFile::add_offset(off_t offset) {
    // 1. Acquire exclusive lock for writing
    std::unique_lock<std::shared_mutex> wl(rwmutex);
    // 2. Locate the start of the entries
    char *base = (char*)map + header_size;
    // 3. Iterate through and update existing entries
    for (size_t i = 0; i < max_entries; i++) {
        OffsetEntry *e = reinterpret_cast<OffsetEntry*>(base + i * entry_size);
        if (e->is_valid()) {
            e->add_offset(offset);
        }
    }
   // Optional: msync could be called here if you need immediate
   // durability guarantees before the lock is released.
}


/* IDEA:

// When merging from a source file into a current 'offsets' mmap:
source_offsets.for_each([&](size_t label, const OffsetEntry& e) {
    // Copy, shift, and write into the new mmap in one fluid motion
    offsets->set(new_label, OffsetEntry(e).add_offset(shift_amount));
    
    if (cfg.flush_offsets_each)
        offsets->flush(new_label);
});

// Within your merge logic:
for_each_in_source([&](size_t label, const OffsetEntry& e) {
    // 1. Logic: Copy, mutate, and set
    // This uses the new add_offset(delta) returning *this
    offsets->set(label, OffsetEntry(e).add_offset(master_delta));

    // 2. Durability:
    if (cfg.flush_offsets_each) {
        offsets->flush(label); 
    }
});

// 3. Final safety catch-all
if (!cfg.flush_offsets_each) {
    offsets->flush(0); // Triggers the 'global' msync(map, filesize, MS_SYNC)
}

*/
