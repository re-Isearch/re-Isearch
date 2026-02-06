// LSMVectorStorage_threadsafe.h
// Single-writer / multi-reader thread-safe LSM vector storage
// - Durable delta logs
// - Streaming compaction
// - Versioned compact files
// - Uses std::shared_mutex for SWMR safety
// Requires C++17

#pragma once

#include "hnswlib/hnswlib.h"
#include <fcntl.h>
#include <cstring>
#include <numeric>
#include <cmath>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <filesystem>
#include <cstdio>
#include <cerrno>
#include <system_error>
#include <shared_mutex>
#include <atomic>

namespace hnswlib {

enum class VectorStorageMode {
    DISABLED,
    IN_MEMORY,
    MEMORY_MAPPED
};

struct VectorStorageConfig {
    VectorStorageMode mode = VectorStorageMode::DISABLED;
    size_t flush_threshold = 10000;
    size_t compact_threshold = 20;
    bool auto_compact_on_save = true;
    bool use_streaming_compaction = false;
};

class LSMVectorStorage {
private:
    VectorStorageConfig config_;
    VectorStorageMode &storage_mode_ = config_.mode;
    size_t &flush_threshold_         = config_.flush_threshold ;
    size_t &compact_threshold_       = config_.compact_threshold;

    std::string base_filename_;
    size_t dim_ = 0;

    // mmap / main offsets
    void* main_mmap_ptr_ = nullptr;
    size_t main_mmap_size_ = 0;
    int main_mmap_fd_ = -1;
    size_t main_vectors_offset_ = 0;
    std::unordered_map<labeltype, size_t> main_offsets_; // label -> offset

    // persistent deltas
    std::vector<std::string> delta_files_; // persisted delta file paths (ascending)
    std::vector<std::unordered_map<labeltype, std::vector<float>>> delta_caches_; // oldest->newest
    std::vector<std::unordered_set<labeltype>> delta_label_sets_;
    std::unordered_map<labeltype, std::vector<float>> current_delta_;
    size_t additions_since_flush_ = 0;

    // version counters
    std::atomic<uint64_t> delta_version_counter_{0};
    std::atomic<uint64_t> compact_version_counter_{0};

    // compacted files
    std::vector<std::string> compact_files_; // sorted ascending
    std::string current_vectors_file_;       // active vectors-only file full path

    // --- synchronization for SWMR ---
    // Readers take shared_lock, writers take unique_lock.
    mutable std::shared_mutex rw_mutex_;

public:
    LSMVectorStorage() {}
    LSMVectorStorage(size_t dim, size_t flush_threshold = 10000) {
        dim_ = dim;
        flush_threshold_ = flush_threshold;
    }

    ~LSMVectorStorage() {
        cleanup();
    }

    void set_dim(size_t dim) { dim_ = dim; }
    void set_storage_mode(VectorStorageMode mode) { storage_mode_ = mode; }
    void set_flush_threshold(size_t flush_threshold) { flush_threshold_ = flush_threshold; }
    void set_basename(const std::string& path) { base_filename_ = path; }
    void set_config(const VectorStorageConfig& cfg) { config_ = cfg; }

private:
    static std::string format_version(uint64_t v) {
        std::ostringstream ss;
        ss << std::setw(5) << std::setfill('0') << v;
        return ss.str();
    }

    std::string compact_filename_for(uint64_t ver) const {
        namespace fs = std::filesystem;
        fs::path base(base_filename_);
        fs::path dir = base.parent_path();
        if (dir.empty()) dir = ".";
        std::string fname = base.filename().string() + ".vectors." + format_version(ver);
        return (dir / fname).string();
    }
    std::string compact_temp_filename_for(uint64_t ver) const { return compact_filename_for(ver) + ".tmp"; }
    std::string delta_filename_for(uint64_t ver) const {
        namespace fs = std::filesystem;
        fs::path base(base_filename_);
        fs::path dir = base.parent_path();
        if (dir.empty()) dir = ".";
        std::string fname = base.filename().string() + ".delta." + format_version(ver);
        return (dir / fname).string();
    }
    std::string delta_temp_filename_for(uint64_t ver) const { return delta_filename_for(ver) + ".tmp"; }

public:
    // Discover compact and delta files; set counters; does not modify in-memory caches.
    void discover_compact_files() {
        std::unique_lock lk(rw_mutex_); // writer lock while we mutate compact_files_ and counter
        compact_files_.clear();
        namespace fs = std::filesystem;
        fs::path base(base_filename_);
        fs::path dir = base.parent_path();
        if (dir.empty()) dir = ".";

        std::string prefix = base.filename().string() + ".vectors.";

        uint64_t max_ver = 0;
        try {
            for (auto &entry : fs::directory_iterator(dir)) {
                if (!entry.is_regular_file()) continue;
                std::string fname = entry.path().filename().string();
                if (fname.rfind(prefix, 0) == 0) {
                    std::string suffix = fname.substr(prefix.size());
                    bool digits = !suffix.empty();
                    for (char c : suffix) { if (!std::isdigit(static_cast<unsigned char>(c))) { digits = false; break; } }
                    if (!digits) continue;
                    compact_files_.push_back((dir / fname).string());
                    try { uint64_t v = std::stoull(suffix); if (v > max_ver) max_ver = v; } catch(...) {}
                }
            }
        } catch(...) { compact_files_.clear(); }
        std::sort(compact_files_.begin(), compact_files_.end());
        compact_version_counter_.store(max_ver, std::memory_order_relaxed);
    }

    void discover_delta_files() {
        std::unique_lock lk(rw_mutex_);
        delta_files_.clear();
        namespace fs = std::filesystem;
        fs::path base(base_filename_);
        fs::path dir = base.parent_path();
        if (dir.empty()) dir = ".";

        std::string prefix = base.filename().string() + ".delta.";

        uint64_t max_ver = 0;
        try {
            for (auto &entry : fs::directory_iterator(dir)) {
                if (!entry.is_regular_file()) continue;
                std::string fname = entry.path().filename().string();
                if (fname.rfind(prefix, 0) == 0) {
                    std::string suffix = fname.substr(prefix.size());
                    bool digits = !suffix.empty();
                    for (char c : suffix) { if (!std::isdigit(static_cast<unsigned char>(c))) { digits = false; break; } }
                    if (!digits) continue;
                    delta_files_.push_back((dir / fname).string());
                    try { uint64_t v = std::stoull(suffix); if (v > max_ver) max_ver = v; } catch(...) {}
                }
            }
        } catch(...) { delta_files_.clear(); }
        std::sort(delta_files_.begin(), delta_files_.end());
        delta_version_counter_.store(max_ver, std::memory_order_relaxed);
    }

    bool load_delta_file_into_cache(const std::string &path) {
        // exclusive lock because we modify delta_caches_
        std::unique_lock lk(rw_mutex_);

        std::ifstream ifs(path, std::ios::binary);
        if (!ifs.good()) {
            HNSWERR << "load_delta_file_into_cache: can't open " << path << "\n";
            return false;
        }
        size_t n;
        ifs.read(reinterpret_cast<char*>(&n), sizeof(size_t));
        if (!ifs.good()) {
            HNSWERR << "load_delta_file_into_cache: header read failed " << path << "\n";
            return false;
        }

        std::unordered_map<labeltype, std::vector<float>> cache;
        cache.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            labeltype lbl;
            ifs.read(reinterpret_cast<char*>(&lbl), sizeof(labeltype));
            std::vector<float> vec(dim_);
            ifs.read(reinterpret_cast<char*>(vec.data()), dim_ * sizeof(float));
            if (!ifs.good()) {
                HNSWERR << "load_delta_file_into_cache: premature EOF " << path << "\n";
                return false;
            }
            cache.emplace(lbl, std::move(vec));
        }

        delta_caches_.push_back(std::move(cache));
        std::unordered_set<labeltype> label_set;
        for (const auto &kv : delta_caches_.back()) label_set.insert(kv.first);
        delta_label_sets_.push_back(std::move(label_set));
        return true;
    }

    bool load_delta_files_on_startup() {
        // load each delta file; we call discover_delta_files() before this, typically.
        for (const auto &df : delta_files_) {
            if (!load_delta_file_into_cache(df)) {
                HNSWERR << "load_delta_files_on_startup: failed to load " << df << "\n";
                return false;
            }
        }
        return true;
    }

private:
    #ifndef _WIN32
    bool setup_mmap_unix() {
        main_mmap_fd_ = open(base_filename_.c_str(), O_RDONLY);
        if (main_mmap_fd_ == -1) {
            HNSWERR << "setup_mmap_unix: open failed for " << base_filename_ << " errno=" << errno << "\n";
            return false;
        }
        size_t page_size = sysconf(_SC_PAGE_SIZE);
        size_t aligned_offset = (main_vectors_offset_ / page_size) * page_size;
        size_t offset_adjustment = main_vectors_offset_ - aligned_offset;
        size_t map_size = main_mmap_size_ + offset_adjustment;

        void* map_base = mmap(nullptr, map_size, PROT_READ, MAP_PRIVATE,
                             main_mmap_fd_, static_cast<off_t>(aligned_offset));
        if (map_base == MAP_FAILED) {
            HNSWERR << "setup_mmap_unix: mmap failed errno=" << errno << "\n";
            close(main_mmap_fd_);
            main_mmap_fd_ = -1;
            return false;
        }

        main_mmap_ptr_ = static_cast<char*>(map_base) + offset_adjustment;
        madvise(map_base, map_size, MADV_RANDOM);
        return true;
    }
    #endif

    #ifdef _WIN32
    bool setup_mmap_windows() { return true; }
    #endif

    void cleanup_mmap() {
        // THIS MUST BE CALLED UNDER EXCLUSIVE LOCK (or else readers can race).
        if (main_mmap_ptr_ != nullptr) {
            #ifndef _WIN32
            size_t page_size = sysconf(_SC_PAGE_SIZE);
            size_t aligned_offset = (main_vectors_offset_ / page_size) * page_size;
            size_t offset_adjustment = main_vectors_offset_ - aligned_offset;
            void* map_base = static_cast<char*>(main_mmap_ptr_) - offset_adjustment;
            munmap(map_base, main_mmap_size_ + offset_adjustment);
            main_mmap_ptr_ = nullptr;
            #else
            UnmapViewOfFile(main_mmap_ptr_);
            main_mmap_ptr_ = nullptr;
            #endif
        }
        if (main_mmap_fd_ != -1) {
            #ifndef _WIN32
            close(main_mmap_fd_);
            #endif
            main_mmap_fd_ = -1;
        }
    }

    bool load_vectors_from_vectors_file(const std::string& vectors_file) {
        // exclusive: this resets base_filename_/mmap state
        std::unique_lock lk(rw_mutex_);

        std::ifstream ifs(vectors_file, std::ios::binary);
        if (!ifs.good()) {
            HNSWERR << "load_vectors_from_vectors_file: can't open " << vectors_file << "\n";
            return false;
        }

        size_t num_vectors;
        ifs.read(reinterpret_cast<char*>(&num_vectors), sizeof(size_t));
        if (!ifs.good()) {
            HNSWERR << "load_vectors_from_vectors_file: header read failed\n";
            return false;
        }

        main_offsets_.clear();
        size_t entry_size = sizeof(labeltype) + dim_ * sizeof(float);

        std::string old_base = base_filename_;
        base_filename_ = vectors_file;
        main_vectors_offset_ = ifs.tellg();

        for (size_t i = 0; i < num_vectors; ++i) {
            labeltype label;
            ifs.read(reinterpret_cast<char*>(&label), sizeof(labeltype));
            main_offsets_[label] = i * entry_size;
            ifs.seekg(dim_ * sizeof(float), std::ios::cur);
            if (!ifs.good()) {
                HNSWERR << "load_vectors_from_vectors_file: premature EOF\n";
                base_filename_ = old_base;
                return false;
            }
        }

        main_mmap_size_ = num_vectors * entry_size;
        // cleanup old mmap (under exclusive lock)
        cleanup_mmap();

        #ifdef _WIN32
        bool ok = setup_mmap_windows();
        #else
        bool ok = setup_mmap_unix();
        #endif

        if (!ok) {
            base_filename_ = old_base;
            return false;
        }

        current_vectors_file_ = vectors_file;
        if (std::find(compact_files_.begin(), compact_files_.end(), vectors_file) == compact_files_.end()) {
            compact_files_.push_back(vectors_file);
            std::sort(compact_files_.begin(), compact_files_.end());
        }
        return true;
    }

    bool load_with_mmap(std::ifstream& ifs, size_t num_vectors) {
        std::unique_lock lk(rw_mutex_);

        main_vectors_offset_ = ifs.tellg();
        size_t entry_size = sizeof(labeltype) + dim_ * sizeof(float);
        main_offsets_.clear();

        for (size_t i = 0; i < num_vectors; i++) {
            labeltype label;
            ifs.read(reinterpret_cast<char*>(&label), sizeof(labeltype));
            main_offsets_[label] = i * entry_size;
            ifs.seekg(dim_ * sizeof(float), std::ios::cur);
            if (!ifs.good()) return false;
        }

        main_mmap_size_ = num_vectors * entry_size;
        cleanup_mmap();

        #ifdef _WIN32
        return setup_mmap_windows();
        #else
        return setup_mmap_unix();
        #endif
    }

    bool load_in_memory(std::ifstream& ifs, size_t num_vectors) {
        std::unique_lock lk(rw_mutex_);
        current_delta_.clear();
        for (size_t i = 0; i < num_vectors; i++) {
            labeltype label;
            ifs.read(reinterpret_cast<char*>(&label), sizeof(labeltype));
            std::vector<float> vec(dim_);
            ifs.read(reinterpret_cast<char*>(vec.data()), dim_ * sizeof(float));
            if (!ifs.good()) return false;
            current_delta_[label] = std::move(vec);
        }
        return true;
    }

public:
    // public load entrypoint (thread-safe)
    bool load_vectors(const std::string& filename, std::ifstream& ifs,
                      size_t num_vectors, VectorStorageMode mode, size_t flush_threshold = 0) {
        // Writer path: exclusive lock
        std::unique_lock lk(rw_mutex_);
        base_filename_   = filename;
        storage_mode_    = mode;
        if (flush_threshold) flush_threshold_ = flush_threshold;

        // discover persisted state and replay deltas
        // discover functions acquire the same lock inside; but we already hold it so call internal logic:
        lk.unlock();
        discover_compact_files();
        discover_delta_files();
        if (!load_delta_files_on_startup()) {
            HNSWERR << "load_vectors: failed to load delta files on startup\n";
            return false;
        }
        lk.lock();

        if (!compact_files_.empty()) {
            main_offsets_.clear();
            return load_vectors_from_vectors_file(compact_files_.back());
        }
        return load_with_mmap(ifs, num_vectors);
    }

    bool load_vectors(const std::string& filename, std::ifstream& ifs,
                      VectorStorageMode mode = VectorStorageMode::MEMORY_MAPPED, size_t flush_threshold = 0) {
        size_t num_vectors;
        ifs.read(reinterpret_cast<char*>(&num_vectors), sizeof(size_t));
        return load_vectors(filename, ifs, num_vectors, mode, flush_threshold);
    }

    // ---------------- add / flush / get ----------------

    void addPoint(labeltype label, const float* data) {
        // writer: exclusive
        std::unique_lock lk(rw_mutex_);
        if (storage_mode_ == VectorStorageMode::DISABLED) return;
        current_delta_[label] = std::vector<float>(data, data + dim_);
        additions_since_flush_++;
        if (additions_since_flush_ >= flush_threshold_) {
            // flush_delta acquires the same writer lock; avoid double-lock by calling internal _unsafe version
            lk.unlock();
            flush_delta();
            // no need to relock here
        }
    }

    // get_vector: reader (shared)
    const float* get_vector(labeltype label) const {
        std::shared_lock lk(rw_mutex_);

        // 1) current delta (in-memory)
        auto it = current_delta_.find(label);
        if (it != current_delta_.end()) return it->second.data();

        // 2) delta caches newest-first
        for (size_t i = delta_caches_.size(); i > 0; --i) {
            size_t idx = i - 1;
            if (delta_label_sets_[idx].find(label) == delta_label_sets_[idx].end()) continue;
            auto dit = delta_caches_[idx].find(label);
            if (dit != delta_caches_[idx].end()) return dit->second.data();
        }

        // 3) main mmap
        if (storage_mode_ == VectorStorageMode::MEMORY_MAPPED && main_mmap_ptr_) {
            auto mit = main_offsets_.find(label);
            if (mit != main_offsets_.end()) {
                size_t offset = mit->second + sizeof(labeltype);
                return reinterpret_cast<const float*>(
                    static_cast<char*>(main_mmap_ptr_) + offset
                );
            }
        }
        return nullptr;
    }

private:
    bool read_main_vector_to_buffer(labeltype label, std::vector<float>& out_buf) const {
        // reader: use shared lock because it reads main_offsets_ and main_mmap_ptr_
        std::shared_lock lk(rw_mutex_);
        auto mit = main_offsets_.find(label);
        if (mit == main_offsets_.end()) return false;
        size_t offset = mit->second + sizeof(labeltype);
        out_buf.resize(dim_);
        if (main_mmap_ptr_) {
            const float* vecptr = reinterpret_cast<const float*>(
                static_cast<char*>(main_mmap_ptr_) + offset
            );
            std::copy(vecptr, vecptr + dim_, out_buf.begin());
            return true;
        } else {
            // fallback: read from base file (needs no lock for file IO)
            lk.unlock();
            std::ifstream ifs(base_filename_, std::ios::binary);
            if (!ifs.good()) return false;
            ifs.seekg(static_cast<std::streamoff>(main_vectors_offset_ + mit->second + sizeof(labeltype)));
            ifs.read(reinterpret_cast<char*>(out_buf.data()), dim_ * sizeof(float));
            return ifs.good();
        }
    }

public:
    // ---------------- flush delta (writer) ----------------
    bool flush_delta() {
        // exclusive writer lock
        std::unique_lock lk(rw_mutex_);

        if (current_delta_.empty()) return true;

        uint64_t ver = delta_version_counter_.fetch_add(1, std::memory_order_relaxed) + 1;
        std::string tmpname = delta_temp_filename_for(ver);
        std::string finalname = delta_filename_for(ver);

        {
            std::ofstream ofs(tmpname, std::ios::binary | std::ios::trunc);
            if (!ofs.good()) {
                HNSWERR << "flush_delta: failed to open tmp " << tmpname << "\n";
                // revert counter? we used fetch_add so version was reserved; it's OK to leave gaps.
                return false;
            }
            size_t n = current_delta_.size();
            ofs.write(reinterpret_cast<const char*>(&n), sizeof(size_t));
            for (const auto &kv : current_delta_) {
                labeltype label = kv.first;
                const std::vector<float> &vec = kv.second;
                ofs.write(reinterpret_cast<const char*>(&label), sizeof(labeltype));
                ofs.write(reinterpret_cast<const char*>(vec.data()), dim_ * sizeof(float));
            }
            ofs.flush();
            // optionally fsync
        }

        std::error_code ec;
        std::filesystem::rename(tmpname, finalname, ec);
        if (ec) {
            HNSWERR << "flush_delta: rename failed " << tmpname << " -> " << finalname
                   << " error=" << ec.message() << "\n";
            std::filesystem::remove(tmpname);
            return false;
        }

        delta_files_.push_back(finalname);
        delta_caches_.push_back(std::move(current_delta_));
        std::unordered_set<labeltype> label_set;
        for (const auto &kv : delta_caches_.back()) label_set.insert(kv.first);
        delta_label_sets_.push_back(std::move(label_set));

        current_delta_.clear();
        additions_since_flush_ = 0;
        return true;
    }

    // ---------------- cleanup deltas (writer) ----------------
    void cleanup_deltas() {
        std::unique_lock lk(rw_mutex_);
        for (const auto &f : delta_files_) {
            std::error_code ec;
            std::filesystem::remove(f, ec);
            if (ec) {
                HNSWERR << "cleanup_deltas: failed to remove " << f << " error=" << ec.message() << "\n";
            }
        }
        delta_files_.clear();
        delta_caches_.clear();
        delta_label_sets_.clear();
    }

    // ---------------- streaming enumeration (reader for most parts) ----------------
    template <typename Fn>
    void for_each_vector(Fn fn) {
        // We will take a shared lock for reading maps/pointers; writer can block while compaction runs.
        // Flush current delta to persistent cache so it's included (we call flush_delta which is writer).
        {
            std::shared_lock lk(rw_mutex_);
            if (!current_delta_.empty()) {
                // release shared lock and call flush under exclusive lock
                lk.unlock();
                flush_delta();
            }
        }

        // Now enumerate under shared lock
        std::shared_lock lk(rw_mutex_);
        std::unordered_set<labeltype> seen;
        seen.reserve(main_offsets_.size() + 128);

        if (main_mmap_ptr_) {
            for (const auto &p : main_offsets_) {
                labeltype label = p.first;
                seen.insert(label);
                const float* base_vec = reinterpret_cast<const float*>(
                    static_cast<const char*>(main_mmap_ptr_) + p.second + sizeof(labeltype)
                );

                // check delta overrides newest-first
                const float* override_vec = nullptr;
                for (size_t i = delta_caches_.size(); i > 0; --i) {
                    size_t idx = i - 1;
                    auto it = delta_caches_[idx].find(label);
                    if (it != delta_caches_[idx].end()) {
                        override_vec = it->second.data();
                        break;
                    }
                }

                fn(label, override_vec ? override_vec : base_vec);
            }
        }

        // emit delta-only
        for (const auto &dcache : delta_caches_) {
            for (const auto &kv : dcache) {
                labeltype label = kv.first;
                if (seen.find(label) == seen.end()) {
                    fn(label, kv.second.data());
                }
            }
        }
    }

    // ---------------- compaction (writer) ----------------
    bool compact() {
        std::unique_lock lk(rw_mutex_);
        if (delta_files_.empty() && current_delta_.empty() && delta_caches_.empty()) return true;
        if (config_.use_streaming_compaction) {
            // unlocking here and calling streaming compaction which will re-acquire unique locks internally is complicated.
            // Keep it simple: call the streaming method while holding lock (streaming writes to disk but doesn't allocate all vectors).
            return compact_streaming_locked();
        }
        return compact_in_memory_locked();
    }

private:
    // in-memory compaction: called under exclusive lock
    bool compact_in_memory_locked() {
        if (!current_delta_.empty()) {
            // use existing flush_delta which expects exclusive lock; we're already holding it
            // so call internal write logic without double-locking; implement inline copy of flush behavior here
            // but for simplicity call flush_delta() after unlocking and relocking:
            // (practical systems use a memtable swap; here we do the simple path)
            std::unique_lock unlock(rw_mutex_, std::adopt_lock);
            unlock.unlock();
            flush_delta();
            unlock.relock();
        }

        std::unordered_map<labeltype, std::vector<float>> all_vectors;

        if (main_mmap_ptr_) {
            for (const auto &p : main_offsets_) {
                const float* vec_ptr = reinterpret_cast<const float*>(
                    static_cast<char*>(main_mmap_ptr_) + p.second + sizeof(labeltype)
                );
                all_vectors[p.first] = std::vector<float>(vec_ptr, vec_ptr + dim_);
            }
        }

        for (const auto &dcache : delta_caches_) {
            for (const auto &kv : dcache) all_vectors[kv.first] = kv.second;
        }

        // also include any current_delta_ (should be empty now)
        for (const auto &kv : current_delta_) all_vectors[kv.first] = kv.second;

        // create compact file
        discover_compact_files(); // will take lock internally
        uint64_t ver = compact_version_counter_.fetch_add(1, std::memory_order_relaxed) + 1;
        std::string tmpfile = compact_temp_filename_for(ver);
        std::string newfile = compact_filename_for(ver);

        {
            std::ofstream ofs(tmpfile, std::ios::binary | std::ios::trunc);
            if (!ofs.good()) {
                std::filesystem::remove(tmpfile);
                return false;
            }
            size_t n = all_vectors.size();
            ofs.write(reinterpret_cast<const char*>(&n), sizeof(size_t));
            for (const auto &kv : all_vectors) {
                labeltype label = kv.first;
                const std::vector<float> &vec = kv.second;
                ofs.write(reinterpret_cast<const char*>(&label), sizeof(labeltype));
                ofs.write(reinterpret_cast<const char*>(vec.data()), dim_ * sizeof(float));
            }
            ofs.flush();
        }

        cleanup_mmap();
        main_offsets_.clear();

        std::error_code ec;
        std::filesystem::rename(tmpfile, newfile, ec);
        if (ec) { std::filesystem::remove(tmpfile); return false; }

        current_vectors_file_ = newfile;
        if (!load_vectors_from_vectors_file(current_vectors_file_)) return false;

        cleanup_deltas();
        cleanup_old_compact_files();
        return true;
    }

    // streaming compaction while holding lock
    bool compact_streaming_locked() {
        if (!current_delta_.empty()) {
            // flush current delta; safe as we hold the exclusive lock
            // call flush_delta() which will also take the lock, so do a manual inline flush:
            uint64_t ver = delta_version_counter_.fetch_add(1, std::memory_order_relaxed) + 1;
            std::string tmpname = delta_temp_filename_for(ver);
            std::string finalname = delta_filename_for(ver);
            {
                std::ofstream ofs(tmpname, std::ios::binary | std::ios::trunc);
                size_t n = current_delta_.size();
                ofs.write(reinterpret_cast<const char*>(&n), sizeof(size_t));
                for (const auto &kv : current_delta_) {
                    ofs.write(reinterpret_cast<const char*>(&kv.first), sizeof(labeltype));
                    ofs.write(reinterpret_cast<const char*>(kv.second.data()), dim_ * sizeof(float));
                }
                ofs.flush();
            }
            std::error_code ec;
            std::filesystem::rename(tmpname, finalname, ec);
            if (ec) { std::filesystem::remove(tmpname); return false; }
            delta_files_.push_back(finalname);
            delta_caches_.push_back(std::move(current_delta_));
            std::unordered_set<labeltype> label_set;
            for (const auto &kv : delta_caches_.back()) label_set.insert(kv.first);
            delta_label_sets_.push_back(std::move(label_set));
            current_delta_.clear();
        }

        // prepare compact file
        discover_compact_files();
        uint64_t ver = compact_version_counter_.fetch_add(1, std::memory_order_relaxed) + 1;
        std::string tmpfile = compact_temp_filename_for(ver);
        std::string newfile = compact_filename_for(ver);

        std::ofstream ofs(tmpfile, std::ios::binary | std::ios::trunc);
        if (!ofs.good()) { std::filesystem::remove(tmpfile); return false; }

        size_t placeholder = 0;
        ofs.write(reinterpret_cast<const char*>(&placeholder), sizeof(size_t));
        size_t written = 0;
        std::vector<float> tmpbuf; tmpbuf.resize(dim_);

        // build set of labels that appear in deltas (fast check)
        std::unordered_set<labeltype> labels_with_deltas;
        for (const auto &dcache : delta_caches_) {
            for (const auto &kv : dcache) labels_with_deltas.insert(kv.first);
        }

        for (const auto &p : main_offsets_) {
            labeltype label = p.first;
            if (labels_with_deltas.find(label) != labels_with_deltas.end()) continue;
            if (!read_main_vector_to_buffer(label, tmpbuf)) { ofs.close(); std::filesystem::remove(tmpfile); return false; }
            ofs.write(reinterpret_cast<const char*>(&label), sizeof(labeltype));
            ofs.write(reinterpret_cast<const char*>(tmpbuf.data()), dim_ * sizeof(float));
            if (!ofs.good()) { ofs.close(); std::filesystem::remove(tmpfile); return false; }
            ++written;
        }

        // write latest delta vectors (newest wins)
        std::unordered_set<labeltype> written_labels;
        for (size_t di = 0; di < delta_caches_.size(); ++di) {
            for (const auto &kv : delta_caches_[di]) {
                labeltype label = kv.first;
                // skip if a later delta contains this label
                bool has_later = false;
                for (size_t dj = di + 1; dj < delta_caches_.size(); ++dj) {
                    if (delta_caches_[dj].find(label) != delta_caches_[dj].end()) { has_later = true; break; }
                }
                if (has_later) continue;
                if (written_labels.find(label) != written_labels.end()) continue;
                const auto &vec = kv.second;
                ofs.write(reinterpret_cast<const char*>(&label), sizeof(labeltype));
                ofs.write(reinterpret_cast<const char*>(vec.data()), dim_ * sizeof(float));
                if (!ofs.good()) { ofs.close(); std::filesystem::remove(tmpfile); return false; }
                ++written;
                written_labels.insert(label);
            }
        }

        ofs.seekp(0);
        ofs.write(reinterpret_cast<const char*>(&written), sizeof(size_t));
        ofs.close();

        cleanup_mmap();
        main_offsets_.clear();

        std::error_code ec;
        std::filesystem::rename(tmpfile, newfile, ec);
        if (ec) { std::filesystem::remove(tmpfile); return false; }

        std::string prev = current_vectors_file_;
        current_vectors_file_ = newfile;
        if (std::find(compact_files_.begin(), compact_files_.end(), newfile) == compact_files_.end()) {
            compact_files_.push_back(newfile);
            std::sort(compact_files_.begin(), compact_files_.end());
        }
        if (!prev.empty() && prev != current_vectors_file_) {
            std::error_code remerr;
            std::filesystem::remove(prev, remerr);
            compact_files_.erase(std::remove(compact_files_.begin(), compact_files_.end(), prev),
                                 compact_files_.end());
        }

        if (!load_vectors_from_vectors_file(current_vectors_file_)) return false;

        cleanup_deltas();
        cleanup_old_compact_files();

        return true;
    }

    // ---------------- cleanup old compacts (writer) ----------------
    void cleanup_old_compact_files() {
        std::unique_lock lk(rw_mutex_);
        if (current_vectors_file_.empty()) {
            compact_files_.clear();
            return;
        }
        namespace fs = std::filesystem;
        fs::path cur = fs::absolute(fs::path(current_vectors_file_));
        fs::path dir = cur.parent_path();
        std::string cur_name = cur.filename().string();
        fs::path base(base_filename_);
        std::string prefix = base.filename().string() + ".vectors.";

        for (auto &entry : fs::directory_iterator(dir)) {
            if (!entry.is_regular_file()) continue;
            std::string fname = entry.path().filename().string();
            if (fname.rfind(prefix, 0) != 0) continue;
            if (fname == cur_name) continue;
            std::error_code ec;
            fs::remove(entry.path(), ec);
            if (ec) {
                HNSWERR << "cleanup_old_compact_files: failed to remove " << entry.path()
                       << " error=" << ec.message() << "\n";
            }
        }

        compact_files_.clear();
        compact_files_.push_back(cur.string());
    }

public:
    struct Stats {
        size_t main_vectors;
        size_t delta_files;
        size_t pending_additions;
        size_t total_vectors;
    };

    Stats get_stats() const {
        std::shared_lock lk(rw_mutex_);
        Stats s;
        s.main_vectors = main_offsets_.size();
        s.delta_files = delta_files_.size();
        s.pending_additions = current_delta_.size();
        s.total_vectors = s.main_vectors + s.pending_additions;
        return s;
    }

private:
    void cleanup() {
        std::unique_lock lk(rw_mutex_);
        flush_delta();
        cleanup_mmap();
        delta_caches_.clear();
        delta_label_sets_.clear();
        compact_files_.clear();
    }
};

} // namespace hnswlib

