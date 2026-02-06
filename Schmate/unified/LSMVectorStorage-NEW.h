// LSMVectorStorage.h
// Durable LSM-style vector storage with persistent delta logs and streaming compaction.
// Drop-in replacement for prior header; requires C++17 (std::filesystem).

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

    // Main mapping info
    void* main_mmap_ptr_ = nullptr;
    size_t main_mmap_size_ = 0;
    int main_mmap_fd_ = -1;
    size_t main_vectors_offset_ = 0;
    std::unordered_map<labeltype, size_t> main_offsets_; // label -> offset

    // Deltas and persistence
    std::vector<std::string> delta_files_; // persisted delta file paths (ascending)
    std::vector<std::unordered_map<labeltype, std::vector<float>>> delta_caches_; // oldest -> newest
    std::vector<std::unordered_set<labeltype>> delta_label_sets_;
    std::unordered_map<labeltype, std::vector<float>> current_delta_;
    size_t additions_since_flush_ = 0;

    // Version counters (persistent via discover on startup)
    uint64_t delta_version_counter_ = 0;
    uint64_t compact_version_counter_ = 0;

    // Compacted files
    std::vector<std::string> compact_files_; // full paths sorted ascending
    std::string current_vectors_file_;       // full path of active vectors-only file

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

    // --------------- helpers: filename formatting ----------------
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

    std::string compact_temp_filename_for(uint64_t ver) const {
        return compact_filename_for(ver) + ".tmp";
    }

    std::string delta_filename_for(uint64_t ver) const {
        namespace fs = std::filesystem;
        fs::path base(base_filename_);
        fs::path dir = base.parent_path();
        if (dir.empty()) dir = ".";
        std::string fname = base.filename().string() + ".delta." + format_version(ver);
        return (dir / fname).string();
    }

    std::string delta_temp_filename_for(uint64_t ver) const {
        return delta_filename_for(ver) + ".tmp";
    }

    // --------------- discovery: compact & delta files -------------
public:
    // Detect existing compact files and set compact_version_counter_
    void discover_compact_files() {
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
                    // suffix digits?
                    std::string suffix = fname.substr(prefix.size());
                    bool digits = !suffix.empty();
                    for (char c : suffix) { if (!std::isdigit(static_cast<unsigned char>(c))) { digits = false; break; } }
                    if (!digits) continue;
                    compact_files_.push_back((dir / fname).string());
                    try {
                        uint64_t v = std::stoull(suffix);
                        if (v > max_ver) max_ver = v;
                    } catch(...) {}
                }
            }
        } catch(...) {
            compact_files_.clear();
        }
        std::sort(compact_files_.begin(), compact_files_.end());
        compact_version_counter_ = max_ver;
        // ensure counter starts at next version when used
    }

    // Detect existing delta files and set delta_version_counter_
    void discover_delta_files() {
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
                    try {
                        uint64_t v = std::stoull(suffix);
                        if (v > max_ver) max_ver = v;
                    } catch(...) {}
                }
            }
        } catch(...) {
            delta_files_.clear();
        }
        std::sort(delta_files_.begin(), delta_files_.end());
        delta_version_counter_ = max_ver;
    }

    // Load a single delta file into the in-memory cache (append as newest)
    bool load_delta_file_into_cache(const std::string &path) {
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

    // Load all discovered delta files into memory (oldest first)
    bool load_delta_files_on_startup() {
        // delta_files_ must be sorted ascending already
        for (const auto &df : delta_files_) {
            if (!load_delta_file_into_cache(df)) {
                HNSWERR << "load_delta_files_on_startup: failed to load " << df << "\n";
                return false;
            }
        }
        return true;
    }

    // ---------------- mmap setup / cleanup ----------------
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
    bool setup_mmap_windows() {
        // Implement Windows mapping if required.
        return true;
    }
    #endif

    void cleanup_mmap() {
        #ifndef _WIN32
        if (main_mmap_ptr_ != nullptr) {
            size_t page_size = sysconf(_SC_PAGE_SIZE);
            size_t aligned_offset = (main_vectors_offset_ / page_size) * page_size;
            size_t offset_adjustment = main_vectors_offset_ - aligned_offset;
            void* map_base = static_cast<char*>(main_mmap_ptr_) - offset_adjustment;
            munmap(map_base, main_mmap_size_ + offset_adjustment);
            main_mmap_ptr_ = nullptr;
        }
        if (main_mmap_fd_ != -1) {
            close(main_mmap_fd_);
            main_mmap_fd_ = -1;
        }
        #else
        if (main_mmap_ptr_ != nullptr) {
            UnmapViewOfFile(main_mmap_ptr_);
            main_mmap_ptr_ = nullptr;
        }
        #endif
    }

    // ---------------- load vectors from vectors-only file ----------------
    bool load_vectors_from_vectors_file(const std::string& vectors_file) {
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

        // temporarily set base_filename_ to vectors_file for mmap open
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

    // ---------------- load vectors in-place from unified file ----------------
    bool load_with_mmap(std::ifstream& ifs, size_t num_vectors) {
        main_vectors_offset_ = ifs.tellg();

        size_t entry_size = sizeof(labeltype) + dim_ * sizeof(float);
        main_offsets_.clear();

        // faster skip: read labels and fill offsets
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

    // ---------------- in-memory loader ----------------
    bool load_in_memory(std::ifstream& ifs, size_t num_vectors) {
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
    // ---------------- public load entrypoint ----------------
    bool load_vectors(const std::string& filename, std::ifstream& ifs,
                      size_t num_vectors, VectorStorageMode mode, size_t flush_threshold = 0) {
        base_filename_   = filename;
        storage_mode_    = mode;
        if (flush_threshold) flush_threshold_ = flush_threshold;

        // discover persisted state (compacts + delta files) and replay deltas
        discover_compact_files();
        discover_delta_files();
        if (!load_delta_files_on_startup()) {
            HNSWERR << "load_vectors: failed to load delta files on startup\n";
            // If you prefer to continue even when deltas fail to load, change behavior here.
            return false;
        }

        // if compacts exist prefer compact
        if (!compact_files_.empty()) {
            main_offsets_.clear();
            return load_vectors_from_vectors_file(compact_files_.back());
        }

        // otherwise fallback to unified-file region
        return load_with_mmap(ifs, num_vectors);
    }

    bool load_vectors(const std::string& filename, std::ifstream& ifs,
                      VectorStorageMode mode = VectorStorageMode::MEMORY_MAPPED, size_t flush_threshold = 0) {
        base_filename_   = filename;
        storage_mode_    = mode;
        if (flush_threshold) flush_threshold_ = flush_threshold;

        size_t num_vectors;
        ifs.read(reinterpret_cast<char*>(&num_vectors), sizeof(size_t));
        return load_vectors(filename, ifs, num_vectors, mode, flush_threshold);
    }

    // ---------------- basic APIs: add / get ----------------
    void addPoint(labeltype label, const float* data) {
        if (storage_mode_ == VectorStorageMode::DISABLED) return;
        current_delta_[label] = std::vector<float>(data, data + dim_);
        additions_since_flush_++;
        if (additions_since_flush_ >= flush_threshold_) {
            flush_delta();
        }
    }

    const float* get_vector(labeltype label) const {
        // check current_delta_
        auto it = current_delta_.find(label);
        if (it != current_delta_.end()) return it->second.data();

        // check delta caches newest-first
        for (size_t i = delta_caches_.size(); i > 0; --i) {
            size_t idx = i - 1;
            if (delta_label_sets_[idx].find(label) == delta_label_sets_[idx].end()) continue;
            auto dit = delta_caches_[idx].find(label);
            if (dit != delta_caches_[idx].end()) return dit->second.data();
        }

        // main mmap
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
    // helper to read a main vector into a buffer (used by streaming compaction)
    bool read_main_vector_to_buffer(labeltype label, std::vector<float>& out_buf) const {
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
            // fallback: read from base_filename_ at offset
            std::ifstream ifs(base_filename_, std::ios::binary);
            if (!ifs.good()) return false;
            ifs.seekg(static_cast<std::streamoff>(main_vectors_offset_ + mit->second + sizeof(labeltype)));
            ifs.read(reinterpret_cast<char*>(out_buf.data()), dim_ * sizeof(float));
            return ifs.good();
        }
    }

public:
    // ---------------- flush delta -> persisted versioned delta file ----------------
    bool flush_delta() {
        if (current_delta_.empty()) return true;

        // bump counter and write tmp -> rename
        delta_version_counter_++;
        std::string tmpname = delta_temp_filename_for(delta_version_counter_);
        std::string finalname = delta_filename_for(delta_version_counter_);

        // Write tmp file
        {
            std::ofstream ofs(tmpname, std::ios::binary | std::ios::trunc);
            if (!ofs.good()) {
                HNSWERR << "flush_delta: failed to open tmp " << tmpname << "\n";
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
            // optional: fsync underlying file descriptor for extra durability.
            // Implementation of portable fsync on std::ofstream is system-dependent.
        }

        // Atomically rename tmp -> final
        std::error_code ec;
        std::filesystem::rename(tmpname, finalname, ec);
        if (ec) {
            HNSWERR << "flush_delta: rename failed " << tmpname << " -> " << finalname
                   << " error=" << ec.message() << "\n";
            std::filesystem::remove(tmpname);
            return false;
        }

        // Track disk file
        delta_files_.push_back(finalname);

        // Keep in-memory cache for runtime queries
        delta_caches_.push_back(std::move(current_delta_));
        std::unordered_set<labeltype> label_set;
        for (const auto &kv : delta_caches_.back()) label_set.insert(kv.first);
        delta_label_sets_.push_back(std::move(label_set));

        current_delta_.clear();
        additions_since_flush_ = 0;
        return true;
    }

    // ---------------- cleanup delta files & caches ----------------
    void cleanup_deltas() {
        // Delete persisted delta files
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

    // ---------------- streaming enumeration (delta-aware) ----------------
    template <typename Fn>
    void for_each_vector(Fn fn) {
        if (!current_delta_.empty()) flush_delta();

        std::unordered_set<labeltype> seen;
        seen.reserve(main_offsets_.size() + 128);

        // emit main vectors with delta overrides applied
        if (main_mmap_ptr_) {
            for (const auto &p : main_offsets_) {
                labeltype label = p.first;
                seen.insert(label);

                const float* base_vec = reinterpret_cast<const float*>(
                    static_cast<const char*>(main_mmap_ptr_) + p.second + sizeof(labeltype)
                );

                // search delta caches newest-first for override
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

        // emit delta-only labels (those not in main)
        for (const auto &dcache : delta_caches_) {
            for (const auto &kv : dcache) {
                labeltype label = kv.first;
                if (seen.find(label) == seen.end()) {
                    fn(label, kv.second.data());
                }
            }
        }
    }

    // ---------------- compaction: in-memory & streaming ----------------
public:
    bool compact() {
        if (delta_files_.empty() && current_delta_.empty() && delta_caches_.empty()) return true;
        if (config_.use_streaming_compaction) return compact_streaming();
        return compact_in_memory();
    }

private:
    bool compact_in_memory() {
        // Build all vectors into memory (not ideal for large sets)
        // We maintain same semantics: delta caches take precedence
        if (!current_delta_.empty()) flush_delta();

        std::unordered_map<labeltype, std::vector<float>> all_vectors;

        // load from main mmap first
        if (main_mmap_ptr_) {
            for (const auto &p : main_offsets_) {
                const float* vec_ptr = reinterpret_cast<const float*>(
                    static_cast<char*>(main_mmap_ptr_) + p.second + sizeof(labeltype)
                );
                all_vectors[p.first] = std::vector<float>(vec_ptr, vec_ptr + dim_);
            }
        }

        // apply deltas (in order oldest -> newest)
        for (const auto &dcache : delta_caches_) {
            for (const auto &kv : dcache) all_vectors[kv.first] = kv.second;
        }

        // also include any current_delta_ (should be empty after flush, but be safe)
        for (const auto &kv : current_delta_) all_vectors[kv.first] = kv.second;

        // write compact file
        discover_compact_files();
        compact_version_counter_++;
        std::string tmpfile = compact_temp_filename_for(compact_version_counter_);
        std::string newfile = compact_filename_for(compact_version_counter_);

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

        // unmap old, rename tmp->final
        cleanup_mmap();
        main_offsets_.clear();

        std::error_code ec;
        std::filesystem::rename(tmpfile, newfile, ec);
        if (ec) {
            std::filesystem::remove(tmpfile);
            return false;
        }

        // set current and remap
        current_vectors_file_ = newfile;
        if (!load_vectors_from_vectors_file(current_vectors_file_)) return false;

        // cleanup deltas and old compacts
        cleanup_deltas();
        cleanup_old_compact_files();

        return true;
    }

    bool compact_streaming() {
        // flush current delta into persisted caches
        if (!current_delta_.empty()) flush_delta();

        if (delta_caches_.empty() && main_offsets_.empty()) {
            // nothing to compact; but still create an empty vectors file if desired
            return compact_in_memory();
        }

        discover_compact_files();
        compact_version_counter_++;
        std::string tmpfile = compact_temp_filename_for(compact_version_counter_);
        std::string newfile = compact_filename_for(compact_version_counter_);

        std::ofstream ofs(tmpfile, std::ios::binary | std::ios::trunc);
        if (!ofs.good()) {
            std::filesystem::remove(tmpfile);
            return false;
        }

        // placeholder
        size_t placeholder = 0;
        ofs.write(reinterpret_cast<const char*>(&placeholder), sizeof(size_t));
        size_t written = 0;

        std::vector<float> tmpbuf; tmpbuf.resize(dim_);

        // stream main vectors (skip those overridden by deltas)
        // build a small map of labels that have deltas (newest wins) for quick check
        std::unordered_set<labeltype> labels_with_deltas;
        for (const auto &dcache : delta_caches_) {
            for (const auto &kv : dcache) labels_with_deltas.insert(kv.first);
        }

        for (const auto &p : main_offsets_) {
            labeltype label = p.first;
            if (labels_with_deltas.find(label) != labels_with_deltas.end()) continue;
            if (!read_main_vector_to_buffer(label, tmpbuf)) {
                ofs.close();
                std::filesystem::remove(tmpfile);
                return false;
            }
            ofs.write(reinterpret_cast<const char*>(&label), sizeof(labeltype));
            ofs.write(reinterpret_cast<const char*>(tmpbuf.data()), dim_ * sizeof(float));
            if (!ofs.good()) { ofs.close(); std::filesystem::remove(tmpfile); return false; }
            ++written;
        }

        // write latest delta vectors (each label once)
        // to ensure newest wins, iterate delta_caches_ from oldest to newest while tracking written labels
        std::unordered_set<labeltype> written_labels;
        for (size_t di = 0; di < delta_caches_.size(); ++di) {
            for (const auto &kv : delta_caches_[di]) {
                labeltype label = kv.first;
                // only write if this is the newest occurrence (we will skip older ones)
                // to ensure newest wins, we check for later occurrences:
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

        // finalize count
        ofs.seekp(0);
        ofs.write(reinterpret_cast<const char*>(&written), sizeof(size_t));
        ofs.close();

        // swap in new compact file
        cleanup_mmap();
        main_offsets_.clear();

        std::error_code ec;
        std::filesystem::rename(tmpfile, newfile, ec);
        if (ec) {
            std::filesystem::remove(tmpfile);
            return false;
        }

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

        // remap new file
        if (!load_vectors_from_vectors_file(current_vectors_file_)) return false;

        // cleanup deltas (persisted ones)
        cleanup_deltas();

        // remove older compact files
        cleanup_old_compact_files();

        return true;
    }

    // ---------------- cleanup old compacts: keep only current ----------------
    void cleanup_old_compact_files() {
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

    // ---------------- stats & cleanup ----------------
public:
    struct Stats {
        size_t main_vectors;
        size_t delta_files;
        size_t pending_additions;
        size_t total_vectors;
    };

    Stats get_stats() const {
        Stats s;
        s.main_vectors = main_offsets_.size();
        s.delta_files = delta_files_.size();
        s.pending_additions = current_delta_.size();
        s.total_vectors = s.main_vectors + s.pending_additions;
        return s;
    }

private:
    void cleanup() {
        flush_delta();
        cleanup_mmap();
        // remove persisted deltas? no, keep them unless explicitly compacted/cleaned
        delta_caches_.clear();
        delta_label_sets_.clear();
        compact_files_.clear();
    }
};

} // namespace hnswlib

