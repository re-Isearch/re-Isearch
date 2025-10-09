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

static const char magic[8] = "SBIDXv1";

OffsetFile::OffsetFile(const std::string &path, size_t max_entries)
    : max_entries(max_entries)
{
    fd = ::open(path.c_str(), O_RDWR | O_CREAT, 0644);
    if (fd < 0) throw std::runtime_error("OffsetFile: cannot open " + path);

    // expected size
    filesize = header_size + max_entries * entry_size;

    struct stat st;
    if (fstat(fd, &st) < 0) throw std::runtime_error("OffsetFile: fstat failed");

    if ((size_t)st.st_size < filesize) {
        // new file, write header + resize
        ::pwrite(fd, magic, 8, 0);
        ::pwrite(fd, &entry_size, sizeof(entry_size), 8);
        if (ftruncate(fd, filesize) < 0) throw std::runtime_error("OffsetFile: ftruncate failed");
    } else {
        char buffer[8];
        ::pread(fd, buffer, 8, 0);
        if (memcmp(buffer, magic, 8) != 0)
	  LOG_ERROR_S() << "Offset File magic \"" << buffer << "\"!=\"" << magic << "\"";
    }

    map = ::mmap(nullptr, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) throw std::runtime_error("OffsetFile: mmap failed");
}

OffsetFile::~OffsetFile() {
    if (map && map != MAP_FAILED) {
        ::msync(map, filesize, MS_SYNC);
        ::munmap(map, filesize);
    }
    if (fd >= 0) ::close(fd);
}

OffsetEntry OffsetFile::get(size_t label) const {
    if (label >= max_entries) throw std::out_of_range("OffsetFile::get label OOB");
    char *base = (char*)map + header_size;
    return *((OffsetEntry*)(base + label * entry_size));
}

void OffsetFile::set(size_t label, const OffsetEntry &entry) {
    if (label >= max_entries) throw std::out_of_range("OffsetFile::set label OOB");
    char *base = (char*)map + header_size;
    memcpy(base + label * entry_size, &entry, entry_size);
    ::msync(base + label * entry_size, entry_size, MS_SYNC);
}

void OffsetFile::flush() {
    if (map && map != MAP_FAILED) {
        ::msync(map, filesize, MS_SYNC);
    }
}


void OffsetFile::for_each(const std::function<void(size_t, const OffsetEntry &)> &fn) const {
    const char *base = (const char*)map + header_size;
    for (size_t i = 0; i < max_entries; i++) {
        const OffsetEntry *e = reinterpret_cast<const OffsetEntry*>(base + i * entry_size);
        if (e->sid != 0 || e->file_start != 0 || e->file_end != 0) {
            fn(i, *e);
        }
    }
}

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

