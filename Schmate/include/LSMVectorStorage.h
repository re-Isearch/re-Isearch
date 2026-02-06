// LSMVectorStorage.h -- Versioned compact-file support (vectors-only files)
// Includes streaming compaction mode to write vectors in-order without
// allocating all vectors at once.

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
#include <shared_mutex>

/*
The whole point of this class is to provide a flexible storage model for vectors
(embeddings) for re-scoring. Motivation is to be able to provide a means that does
not demand in-memory but can use memory mapping to reduce the RAM footprint.  This
is important since the HNSW algorithm needs to keep its index in memory for acceptable
performance. 
Since we want to support additions we create deltas. Since the deltas can get out of
the control we can compact them into a new vector store.  When later on saving the index
we gather all the vectors and write them into the combined index.


Design summary:

 - Every delta flush is written to disk as a versioned delta file: <base>.delta.<ver>
   with zero-padded version numbers (e.g. .delta.00001).

    Each delta file contains the same binary format we use for compact files (and the
    portion in the unfified index file): size_t num_vectors; [ labeltype, float*dim ] * num_vectors

    We maintain an in-memory counter delta_version_counter_ that is initialized on startup by
    scanning existing delta files (via discover_delta_files()) and set to the highest version found.

 - On each flush we ++delta_version_counter_ and write *.tmp then rename() to .delta.<ver> (atomic on POSIX).

 - On startup / load we call discover_delta_files() then load_delta_files_on_startup() which:
   finds delta files in ascending version order, loads them into delta_caches_ (oldest first), and builds
   delta_label_sets_, ensures later delta files override earlier ones when compaction runs (we keep them in order).

 - During compaction we merge main + deltas; on successful compaction we remove the delta files
   that were merged (and clear caches). If compaction fails, deltas remain intact.

 - get_vector() checks current_delta_ then delta_caches_ newest-first; that behavior stays correct
   after replay as we load delta_caches_ in the natural order (oldest → newest) so newest is at the back.

*/

namespace hnswlib {

enum class VectorStorageMode {
    NONE, // No rescoring!
    DISABLED,
    IN_MEMORY,
    MEMORY_MAPPED
};

struct VectorStorageConfig {
    VectorStorageMode mode = VectorStorageMode::DISABLED;
    size_t dimension = 0;
    size_t flush_threshold = 10000; // Flush to delta (to reduce memory footprint)
    // every flush_threshold*compact_threshold we compact
    size_t compact_threshold = 20; // How many deltas before we compact??
    bool auto_compact_on_save = true;

    // Enable streaming compaction (avoid building all vectors in RAM)
    bool use_streaming_compaction = true; // false;
};

class LSMVectorStorage {
private:
    VectorStorageConfig config_;
    VectorStorageMode &storage_mode_ = config_.mode;
    size_t &flush_threshold_         = config_.flush_threshold ;
    size_t &compact_threshold_       = config_.compact_threshold;

    std::string base_filename_;
    size_t dim_ = 0;

    // Main mapping info (points to either unified-file vector-region or a compact vectors-only file)
    void* main_mmap_ptr_ = nullptr;
    size_t main_mmap_size_ = 0;
    int main_mmap_fd_ = -1;
    size_t main_vectors_offset_ = 0; // offset inside base_filename_ where vectors start (or 0 for vectors-only file)
    std::unordered_map<labeltype, size_t> main_offsets_; // offset relative to main_vectors_offset_ (i * entry_size)

    // Delta files + caches
    std::vector<std::string> delta_files_;
    std::vector<std::unordered_map<labeltype, std::vector<float>>> delta_caches_;
    std::vector<std::unordered_set<labeltype>> delta_label_sets_;
    std::unordered_map<labeltype, std::vector<float>> current_delta_;

    std::atomic<size_t> additions_since_flush_ = 0; // Vectors added since last flush
    std::atomic<size_t> saves_since_compact_ = 0; // delta flushes since last compact()

    // Versioned compact files management
    std::vector<std::string> compact_files_;     // list of available compact files (sorted ascending)
    std::string current_vectors_file_;           // currently mmapped vectors file (if any)
    // unsigned compact_version_counter_ = 0;       // next version to use for new compact file

    // version counters
    std::atomic<unsigned> delta_version_counter_{0};
    std::atomic<unsigned> compact_version_counter_{0};


    // --- synchronization for SWMR ---
    // Readers take shared_lock, writers take unique_lock.
    mutable std::shared_mutex rw_mutex_;

public:
    LSMVectorStorage() {}
    LSMVectorStorage(size_t dim, size_t flush_threshold = 10000) {
        dim_ = dim;
        flush_threshold_ = flush_threshold;
    }
    LSMVectorStorage(size_t dim, const VectorStorageConfig& config) : config_(config) {
        dim_ = dim;
    }

    void set_dim(size_t dim) { dim_ = dim; }
    void set_storage_mode(VectorStorageMode mode) { storage_mode_ = mode; }
    void set_flush_threshold(size_t flush_threshold) { flush_threshold_ = flush_threshold; }
    void set_basename(const std::string& path) { base_filename_ = path; }
    void set_config(const VectorStorageConfig& cfg) { config_ = cfg; }

    size_t bytes_per_vector () const {
      if (storage_mode_ == VectorStorageMode::IN_MEMORY) return dim_ * sizeof(float);
      else if (storage_mode_ == VectorStorageMode::DISABLED) return 0;
      else return 0;
    }

    // ------------------------
    // Loading entrypoints
    // ------------------------
    bool load_vectors(const std::string& filename, std::ifstream& ifs,
                      size_t num_vectors, VectorStorageMode mode, size_t flush_threshold = 0) {
        std::unique_lock lk(rw_mutex_);

        base_filename_   = filename;
        storage_mode_    = mode;
        if (flush_threshold) flush_threshold_ = flush_threshold;

        lk.unlock();

        return load_vectors(ifs, num_vectors);
    }

    bool load_vectors(const std::string& filename, std::ifstream& ifs,
                      VectorStorageMode mode = VectorStorageMode::MEMORY_MAPPED, size_t flush_threshold = 0) {
        std::unique_lock lk(rw_mutex_);

        base_filename_   = filename;
        storage_mode_    = mode;
        if (flush_threshold) flush_threshold_ = flush_threshold;

        size_t num_vectors;
        ifs.read(reinterpret_cast<char*>(&num_vectors), sizeof(size_t));

        lk.unlock();

        return load_vectors(ifs, num_vectors);
    }

    bool load_vectors(std::ifstream& ifs, size_t num_vectors) {
        if (storage_mode_ == VectorStorageMode::NONE) return true;

        if (!ifs) return false; // No stream
        if (base_filename_.empty()) {
            HNSWERR << "Can't load vectors: No filename set!\n";
            return false;
        }
        if (!ifs.good()) return false;

        if (storage_mode_ == VectorStorageMode::DISABLED) {
	   // Skip the vector section
	   std::streamoff skip = static_cast<std::streamoff>(num_vectors) * (sizeof(labeltype) + dim_ * sizeof(float));
           ifs.seekg(skip, std::ios::cur);
           return true;
        }

        // Discover and load delta files
        if (!load_delta_files_on_startup()) {
            HNSWERR << "load_vectors: failed to load delta files on startup\n";
            return false;
        }   

        if (storage_mode_ == VectorStorageMode::IN_MEMORY) {
            return load_in_memory(ifs, num_vectors);
        }

        if (storage_mode_ == VectorStorageMode::MEMORY_MAPPED) {
            // Case 1: We might already have compacted vector files: prefer them.
            discover_compact_files(); // populate compact_files_ & compact_version_counter_
            if (!compact_files_.empty()) {
                // Load the latest compact file
                std::string latest = compact_files_.back();
                // clear stale unified offsets
                main_offsets_.clear(); // Added 2 Dec 2025
		return load_vectors_from_vectors_file(latest);
            }

            // Else, load vectors in-place from unified file region (ifs is positioned at vectors)
            return load_with_mmap(ifs, num_vectors);
        }

        return false;
    }

    bool load_vectors_at_offset(const std::string& filename,
                                 size_t file_offset,
                                 size_t num_vectors,
                                 VectorStorageMode mode,
                                 size_t flush_threshold) {
        storage_mode_ = mode;
        base_filename_ = filename;
        flush_threshold_ = flush_threshold;
        main_vectors_offset_ = file_offset;

       if (mode == VectorStorageMode::NONE) {
           // Nothing to do. Should probably not gotten this far.
           return true;
       }

        if (mode == VectorStorageMode::DISABLED) {
            return true;
        }

        if (mode == VectorStorageMode::IN_MEMORY) {
            std::ifstream ifs(filename, std::ios::binary);
            ifs.seekg(file_offset);
            return load_in_memory(ifs, num_vectors);
        }

        if (mode == VectorStorageMode::MEMORY_MAPPED) {
            discover_compact_files();
            if (!compact_files_.empty()) {
                return load_vectors_from_vectors_file(compact_files_.back());
            }

            size_t entry_size = sizeof(labeltype) + dim_ * sizeof(float);
            main_mmap_size_ = num_vectors * entry_size;

            std::ifstream ifs(filename, std::ios::binary);
            ifs.seekg(file_offset);

            main_offsets_.clear();
            for (size_t i = 0; i < num_vectors; i++) {
                labeltype label;
                ifs.read(reinterpret_cast<char*>(&label), sizeof(labeltype));
                main_offsets_[label] = i * entry_size;
                ifs.seekg(dim_ * sizeof(float), std::ios::cur);
                if (!ifs.good()) return false;
            }

            #ifdef _WIN32
            return setup_mmap_windows();
            #else
            return setup_mmap_unix();
            #endif
        }

        return false;
    }


    static VectorStorageMode string_to_mode(const std::string& mode) {
       char ch = mode.at(0);
       // Disabled
       if (ch == 'D' || ch == 'd') return VectorStorageMode::DISABLED;
       // Mapped, Memory-Mappe, MemoryMapped
       if (ch == 'M' || ch == 'm') return VectorStorageMode::MEMORY_MAPPED;
       // In-memory, InMemory
       if (ch == 'I' || ch == 'i') return VectorStorageMode::IN_MEMORY;
       return VectorStorageMode::NONE;
    }
    static std::string mode_to_string(const VectorStorageMode& mode) {
       switch(mode) {
        case VectorStorageMode::DISABLED: return "Disabled";
        case VectorStorageMode::IN_MEMORY: return "InMemory";
        case VectorStorageMode::MEMORY_MAPPED : return "MemoryMapped";
        case VectorStorageMode::NONE: ; default: return "NONE";

       }
    }
private:
    // ------------------------
    // Discover and naming helpers
    // ------------------------
    // These can be changed without breaking the code!
    static constexpr char vector_pred[] = ".vectors.";
    static constexpr char tmp_suffix[]  = ".tmp";
    static constexpr char delta_pred[]  = ".delta.";

   bool do_not_store() const {
       return (storage_mode_ == VectorStorageMode::DISABLED || storage_mode_ == VectorStorageMode::NONE);
    }

    static std::string format_version(uint64_t v) {
        std::ostringstream ss;
        ss << std::setw(5) << std::setfill('0') << v;
        return ss.str();
    }


    // the predicate ".delta." can be changed without breaking anthing
    // since we push its name onto a list (vector of strings).
    std::string make_delta_filename(size_t num) const {
#if 0
        namespace fs = std::filesystem;
        fs::path base(base_filename_);
        fs::path dir = base.parent_path();
        if (dir.empty()) dir = ".";
#endif
	// We have max NN deltas (99)
        std::ostringstream ss;
        ss << std::setw(2) << std::setfill('0') << num;
        return base_filename_ + delta_pred + ss.str();
    }
    std::string make_temp_delta_filename(size_t num) const {
        return make_delta_filename(num) + tmp_suffix;
    }



    std::string make_compact_filename(uint64_t ver) const {
        return base_filename_ + vector_pred + format_version(ver);
    }

    std::string make_temp_compact_filename(uint64_t ver) const {
        return make_compact_filename(ver) + tmp_suffix;
    }


//  General method to discover and build lists: first for compact and vector files
    unsigned discover_files(std::vector<std::string> &list, const char *predicate) const {

        unsigned max_version = 0;

        try {
            std::filesystem::path p(base_filename_);
            auto dir = p.parent_path();
            if (dir.empty()) dir = ".";

            std::string base_stem = p.string() + predicate;

            for (auto &entry : std::filesystem::directory_iterator(dir)) {
                if (!entry.is_regular_file()) continue;
                std::string path_str = entry.path().string();
                std::string fname = std::filesystem::path(path_str).filename().string();

                if (fname.rfind(base_stem, 0) == 0) {
                    const size_t slen = sizeof(tmp_suffix)-1;
                    // Want to ignore the temporary files (denoted by suffic)
                    if (fname.size() >= slen && fname.substr(fname.size()-slen) == tmp_suffix) continue;

                    // Want to make sure that the file has a numeric counter suffix
                    std::string suffix = fname.substr(base_stem.size());
                    bool digits = !suffix.empty();
                    for (char c : suffix) { if (!std::isdigit(static_cast<unsigned char>(c))) { digits = false; break; } }
                    if (!digits) continue;
                    unsigned version;
                    try { version  = std::stoull(suffix); } catch(...) { version = 0; }
                    if (version) {
//                      HNSWDEBUG << "Adding '" << fname << "' to list\n";
                        list.push_back(path_str);
                        if (version > max_version) max_version = version;
                    } 
//                      else HNSWDEBUG << "File '" << fname << "' does not fit pattern\n";
                }
            }
        } catch (...) {
            // ignore
        }

//        std::sort(list.begin(), list.end());
//      HNSWDEBUG << "MAX VERSION: " << max_version << "\n";
        return max_version;
    }

    bool  discover_compact_files() {
        std::unique_lock lk(rw_mutex_); // writer lock while we mutate compact_files_ and counter

        compact_files_.clear(); // Empty
	compact_version_counter_ = 0;
        unsigned  ver = discover_files(compact_files_, vector_pred);
        if (compact_files_.empty()) return false; 

        std::sort(compact_files_.begin(), compact_files_.end());
        compact_version_counter_.store(ver, std::memory_order_relaxed);

        return ver > 0;
    }

    bool discover_delta_files () {
        std::unique_lock lk(rw_mutex_);

        delta_files_.clear();
        unsigned  ver = discover_files(compact_files_, vector_pred);
        std::sort(delta_files_.begin(), delta_files_.end());
        delta_version_counter_.store(ver, std::memory_order_relaxed);
        return ver > 0;
    }



    // ------------------------
    // Mmap setup / cleanup
    // ------------------------
    #ifndef _WIN32
    bool setup_mmap_unix() {
        main_mmap_fd_ = open(base_filename_.c_str(), O_RDONLY);
        if (main_mmap_fd_ == -1) return false;

        size_t page_size = sysconf(_SC_PAGE_SIZE);
        size_t aligned_offset = (main_vectors_offset_ / page_size) * page_size;
        size_t offset_adjustment = main_vectors_offset_ - aligned_offset;
        size_t map_size = main_mmap_size_ + offset_adjustment;

        void* map_base = mmap(nullptr, map_size, PROT_READ, MAP_PRIVATE,
                             main_mmap_fd_, aligned_offset);
        if (map_base == MAP_FAILED) {
            close(main_mmap_fd_);
            main_mmap_fd_ = -1;
            return false;
        }

        main_mmap_ptr_ = static_cast<char*>(map_base) + offset_adjustment;
        madvise(map_base, map_size, MADV_RANDOM);
        return true;
    }
    #else // Windows  
    bool setup_mmap_windows() {
        // Convert filename to wide string for Windows API
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, filename_.c_str(), -1, NULL, 0);
        std::wstring wfilename(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, filename_.c_str(), -1, &wfilename[0], size_needed);
        
        // Open file
        HANDLE hFile = CreateFileW(
            wfilename.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
        
        if (hFile == INVALID_HANDLE_VALUE) {
            return false;
        }
        
        // Create file mapping for entire file (Windows doesn't support partial file mappings directly)
        HANDLE hMapFile = CreateFileMappingW(
            hFile,
            NULL,
            PAGE_READONLY,
            0,
            0,
            NULL
        );
        
        CloseHandle(hFile);  // Can close file handle after creating mapping
        
        if (hMapFile == NULL) {
            return false;
        }
        
        // Map view starting at our offset
        // Note: offset must be multiple of allocation granularity (typically 64KB)
        SYSTEM_INFO sys_info;
        GetSystemInfo(&sys_info);
        DWORD allocation_granularity = sys_info.dwAllocationGranularity;
        
        DWORD64 aligned_offset = (vectors_file_offset_ / allocation_granularity) * allocation_granularity;
        DWORD64 offset_adjustment = vectors_file_offset_ - aligned_offset;
        
        void* map_base = MapViewOfFile(
            hMapFile,
            FILE_MAP_READ,
            (DWORD)(aligned_offset >> 32),
            (DWORD)(aligned_offset & 0xFFFFFFFF),
            mmap_size_ + offset_adjustment
        );
        
        CloseHandle(hMapFile);  // Can close mapping handle after view is created
        
        if (map_base == NULL) {
            return false;
        }
        
        // Adjust pointer to actual start of vector data
        mmap_ptr_ = static_cast<char*>(map_base) + offset_adjustment;
        
        return true;
    }
    #endif // Windows 

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

    // ------------------------
    // Loading a vectors-only file (compacted)
    // ------------------------
    bool load_vectors_from_vectors_file(const std::string& vectors_file) {

        std::ifstream ifs(vectors_file, std::ios::binary);
        if (!ifs.good()) return false;

        size_t num_vectors;
        ifs.read(reinterpret_cast<char*>(&num_vectors), sizeof(size_t));
        if (!ifs.good()) return false;

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
                base_filename_ = old_base;
                return false;
            }
        }

        main_mmap_size_ = num_vectors * entry_size;

        cleanup_mmap();

#if 0
	HNSWERR << "vectors-file header: num_vectors=" << num_vectors << ", entry_size=" << entry_size << "\n";
	size_t printed = 0;
	for (const auto &kv : main_offsets_) {
    	   if (printed++ < 10) {
		 HNSWERR << "  label=" << kv.first << " offset=" << kv.second << "\n";
	   } else break;
	}
#endif

        #ifdef _WIN32
        bool ok = setup_mmap_windows();
        #else
        bool ok = setup_mmap_unix();
        #endif

        if (ok) {
            current_vectors_file_ = vectors_file;
            if (std::find(compact_files_.begin(), compact_files_.end(), vectors_file) == compact_files_.end()) {
                compact_files_.push_back(vectors_file);
                std::sort(compact_files_.begin(), compact_files_.end());
            }
        } else {
            base_filename_ = old_base;
        }

        return ok;
    }

    // ------------------------
    // Loading with mmap from unified file region (ifs is at vector region)
    // ------------------------
    bool load_with_mmap(std::ifstream& ifs, size_t num_vectors) {
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

    // ------------------------
    // In-memory loader
    // ------------------------
    bool load_in_memory(std::ifstream& ifs, size_t num_vectors) {
        current_delta_.clear();
	// Loads all the vectors from ifs into current_delta_
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

   ~LSMVectorStorage() {
        cleanup();
    }

    // ------------------------
    // Basic APIs: add/flush/get
    // ------------------------
    void addPoint(labeltype label, const float* data) {
        if (do_not_store()) return;

        current_delta_[label] = std::vector<float>(data, data + dim_);
        additions_since_flush_++;

        if (additions_since_flush_ >= flush_threshold_) {
            flush_delta();
	    bool should_compact = (delta_files_.size() >= compact_threshold_);
	    if (should_compact) compact();
        }
    }


    // Load a .delta file into cache
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

    // Find .delta files and load them
    bool load_delta_files_on_startup() { 
        // load each delta file; we call discover_delta_files() before this, typically.
        delta_version_counter_ =  discover_delta_files();
        for (const auto &df : delta_files_) {
            if (!load_delta_file_into_cache(df)) {
                HNSWERR << "load_delta_files_on_startup: failed to load " << df << "\n";
                return false;
            }       
        }
        return true;
    }

    bool flush_delta() {
        std::unique_lock lk(rw_mutex_);

        if (current_delta_.empty()) return true;
        const size_t delta_version_counter_ = delta_files_.size() + 1;

	const std::string delta_file = make_delta_filename (delta_version_counter_);
        std::string tmpname = make_temp_delta_filename(delta_version_counter_);

        std::ofstream ofs(tmpname, std::ios::binary);
        if (!ofs) return false;

        size_t num_vectors = current_delta_.size();
        ofs.write(reinterpret_cast<const char*>(&num_vectors), sizeof(size_t));

        for (const auto& [label, vec] : current_delta_) {
            ofs.write(reinterpret_cast<const char*>(&label), sizeof(labeltype));
            ofs.write(reinterpret_cast<const char*>(vec.data()), dim_ * sizeof(float));
        }
	ofs.flush();
	bool ok = ofs.good();
	ofs.close();

        if (!ok) {
	    HNSWERR << "flush_delta: flush I/O failed!\n";
	    std::filesystem::remove(tmpname);
	} else {
	    std::error_code ec;
	    std::filesystem::rename(tmpname, delta_file, ec);
            if (ec) {
                HNSWERR << "flush_delta: rename failed " << tmpname << " -> " << delta_file << " err=" << ec.message() << "\n";
	        ok = false;
	    }
	}
	if (!ok) {
	    std::filesystem::remove(tmpname);
            return false;
	}

        delta_files_.push_back(delta_file); // Add name to the delta_files_ list
        delta_caches_.push_back(std::move(current_delta_));

        std::unordered_set<labeltype> label_set;
        for (const auto& [label, vec] : delta_caches_.back()) label_set.insert(label);
        delta_label_sets_.push_back(std::move(label_set));

        current_delta_.clear();
        additions_since_flush_ = 0;
        saves_since_compact_++;
        return true;
    }

    const float* get_vector(labeltype label) const {
        std::shared_lock lk(rw_mutex_);

        // 1. Current Delta in memory
        auto it = current_delta_.find(label);
        if (it != current_delta_.end()) return it->second.data();

        // 2. Delta caches, newest-first
        for (size_t i = delta_caches_.size(); i > 0; --i) {
            size_t idx = i - 1;
            if (delta_label_sets_[idx].find(label) == delta_label_sets_[idx].end()) continue;
            auto dit = delta_caches_[idx].find(label);
            if (dit != delta_caches_[idx].end()) return dit->second.data();
        }

        // 3. Use MMAPED 
        if (storage_mode_ == VectorStorageMode::MEMORY_MAPPED && main_mmap_ptr_) {
            auto mit = main_offsets_.find(label);
            if (mit != main_offsets_.end()) {
                size_t offset = mit->second + sizeof(labeltype);
                return reinterpret_cast<const float*>(
                    static_cast<char*>(main_mmap_ptr_) + offset
                );
            }
        }

        // NOTFOUND
        return nullptr;
    }

private:
    // Helper to read vector for a label from the main source (mmap preferred, else unified-file read)
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
            // fallback: read from file at main_vectors_offset_
            std::ifstream ifs(base_filename_, std::ios::binary);
            if (!ifs.good()) return false;
            ifs.seekg(main_vectors_offset_ + mit->second + sizeof(labeltype));
            ifs.read(reinterpret_cast<char*>(out_buf.data()), dim_ * sizeof(float));
            return ifs.good();
        }
    }

public:
    // ------------------------
    // Save + consolidation
    // ------------------------
    bool save_vectors_to_stream(std::ofstream& ofs) {
	if (!ofs) return false;

        // exclusive writer lock
        std::unique_lock lk(rw_mutex_);

        size_t count = 0;
        const std::streampos original_pos = ofs.tellp();

        // 1. Write a dummy count
        ofs.write(reinterpret_cast<const char*>(&count), sizeof(size_t));
        
        lk.unlock();
        // 2. Stream vectors without loading into memory
        for_each_vector([&](labeltype lbl, const float* vec) {
            ofs.write(reinterpret_cast<const char*>(&lbl), sizeof(labeltype));
            ofs.write(reinterpret_cast<const char*>(&vec), dim_ * sizeof(float));
            count++;
        });
        const std::streampos end_pos = ofs.tellp();
        // 3. Seek back and write actual count
        ofs.seekp(original_pos);
        ofs.write(reinterpret_cast<const char*>(&count), sizeof(size_t));

        // 4. Now seek to end
        ofs.seekp(end_pos); 

        return ofs.good();
    }


    // This loads all the vectors into memory. This can swap!
    std::vector<std::pair<labeltype, std::vector<float>>> get_all_vectors() {
        if (!current_delta_.empty()) flush_delta();

        std::unordered_map<labeltype, std::vector<float>> all_vectors;

        if (!current_delta_.empty()) {
           flush_delta();
        }


        std::unique_lock unlock(rw_mutex_, std::adopt_lock);

        if (main_mmap_ptr_) {
            for (const auto& [label, offset] : main_offsets_) {
                const float* vec_ptr = reinterpret_cast<const float*>(
                    static_cast<char*>(main_mmap_ptr_) + offset + sizeof(labeltype)
                );
                all_vectors[label] = std::vector<float>(vec_ptr, vec_ptr + dim_);
            }
        }

        for (const auto& delta_cache : delta_caches_) {
            for (const auto& [label, vec] : delta_cache) {
                all_vectors[label] = vec;
            }
        }

        // also include any current_delta_ (should be empty now)
        for (const auto &kv : current_delta_) all_vectors[kv.first] = kv.second;

        std::vector<std::pair<labeltype, std::vector<float>>> result;
        result.reserve(all_vectors.size());
        for (auto& [label, vec] : all_vectors) result.emplace_back(label, std::move(vec));

        cleanup_deltas();
        return result;
    }


// Call fn(label, vec_ptr) for every vector stored.
// vec_ptr points into mmap'ed storage OR into delta_cache storage.
// Guaranteed not to allocate or load all vectors into RAM.
// Callbacks are invoked in arbitrary order unless you want sorted iteration.
template <typename Fn>
void for_each_vector(Fn fn) {
    // 1. Flush active delta so caches are consistent
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

    // 2. Track which labels we’ve emitted (so delta-only labels are covered)
    // A small unordered_map or set; does NOT store vectors.
    std::unordered_set<labeltype> seen;
    seen.reserve(main_offsets_.size() + 128);

    // 3. Emit mmap-based vectors first, applying delta overrides if present
    if (main_mmap_ptr_) {
        for (const auto& [label, offset] : main_offsets_) {
            seen.insert(label);

            const float* base_vec = reinterpret_cast<const float*>(
                static_cast<const char*>(main_mmap_ptr_) + offset + sizeof(labeltype)
            );

            // Check delta override
            const float* override_vec = nullptr;
            for (const auto& dcache : delta_caches_) {
                auto it = dcache.find(label);
                if (it != dcache.end()) {
                    override_vec = it->second.data();
                    break;
                }
            }

            fn(label, override_vec ? override_vec : base_vec);
        }
    }

    // 4. Emit delta-only vectors (no base entry)
    for (const auto& dcache : delta_caches_) {
        for (const auto& [label, vec] : dcache) {
            if (seen.find(label) == seen.end()) {
                fn(label, vec.data());
            }
        }
    }
}

    void clear() {
        cleanup_deltas();
        cleanup_mmap();
       // Need to remove current
       std::remove(current_vectors_file_.c_str());
       compact_files_.clear();
    }

private:
    void cleanup_deltas() {
        for (const auto& delta_file : delta_files_) std::remove(delta_file.c_str());
        delta_files_.clear();
        delta_caches_.clear();
        delta_label_sets_.clear();
    }

public:
    // ------------------------
    // Compaction: versioned .vectors.<ver> files
    // Supports streaming compaction mode controlled by config_.use_streaming_compaction
    // ------------------------
    bool compact() {

        if (delta_files_.empty() && current_delta_.empty()) {
            return true;
        }

        if (config_.use_streaming_compaction) {
            return compact_streaming();
        } else {
            return compact_in_memory();
        }
        saves_since_compact_ = 0;
    }

private:
    // Old style (in-memory consolidation) compaction
    bool compact_in_memory() {
        auto all_vectors = get_all_vectors();

        discover_compact_files();
        uint64_t version = compact_version_counter_++;

        std::string tmpfile = make_temp_compact_filename(version);
        std::string newfile = make_compact_filename(version);

        {
            std::ofstream ofs(tmpfile, std::ios::binary);
            if (!ofs) return false;

            size_t num_vectors = all_vectors.size();
            ofs.write(reinterpret_cast<const char*>(&num_vectors), sizeof(size_t));

            for (const auto& [label, vec] : all_vectors) {
                ofs.write(reinterpret_cast<const char*>(&label), sizeof(labeltype));
                ofs.write(reinterpret_cast<const char*>(vec.data()), dim_ * sizeof(float));
            }
            ofs.close();
        }

        cleanup_mmap();
        main_offsets_.clear();

        std::remove(newfile.c_str());
        if (std::rename(tmpfile.c_str(), newfile.c_str()) != 0) {
            std::remove(tmpfile.c_str());
            return false;
        }

        std::string prev = current_vectors_file_;
        current_vectors_file_ = newfile;
        if (std::find(compact_files_.begin(), compact_files_.end(), newfile) == compact_files_.end()) {
            compact_files_.push_back(newfile);
            std::sort(compact_files_.begin(), compact_files_.end());
        }
        if (!prev.empty() && prev != current_vectors_file_) {
            std::remove(prev.c_str());
            compact_files_.erase(std::remove(compact_files_.begin(), compact_files_.end(), prev),
                                 compact_files_.end());
        }

        main_offsets_.clear(); // ADDED 2 Dec 2025
        if (!load_vectors_from_vectors_file(current_vectors_file_)) {
            return false;
        }

        return true;
    }

    // Streaming compaction: writes compact file without holding all vectors
    bool compact_streaming() {
        std::unique_lock lk(rw_mutex_);

        // Open new compact file
        std::string new_file = next_compact_filename();
        std::ofstream ofs(new_file, std::ios::binary);
        if (!ofs.good()) { return false; }

        size_t count = 0;
        ofs.write((char*)&count, sizeof(size_t)); // placeholder

        lk.unlock();
        // 1. Stream vectors without loading into memory
        for_each_vector([&](labeltype lbl, const float* vec) {
            ofs.write((char*)&lbl, sizeof(labeltype));
            ofs.write((char*)vec, dim_ * sizeof(float));
            count++;
        });
        lk.lock();

        // 2. Seek back and write actual count
        ofs.seekp(0);
        ofs.write((char*)&count, sizeof(size_t));
        ofs.close();

        // 3. Remap the new compact file and cleanup
        return finalize_compaction(new_file);
    }

    // ------------------------
    // Stats + helpers
    // ------------------------
public:
    struct Stats {
        size_t main_vectors;
        size_t delta_files;
        size_t pending_additions;
        size_t total_vectors;
    };

    Stats get_stats() const {
        Stats stats;
        stats.main_vectors = main_offsets_.size();
        stats.delta_files = delta_files_.size();
        stats.pending_additions = current_delta_.size();
        stats.total_vectors = stats.main_vectors + stats.pending_additions;
        return stats;
    }

    bool is_rescoring_enabled() const {
        return storage_mode_ != VectorStorageMode::DISABLED;
    }

private:

    std::string next_compact_filename() {
        // ALWAYS increment here
	return make_compact_filename(++compact_version_counter_);
    }

    void cleanup() {
        flush_delta();
        cleanup_mmap();

        delta_caches_.clear();
        delta_label_sets_.clear();
	// Need to remove current
	std::remove(current_vectors_file_.c_str());
        compact_files_.clear();
    }

    void cleanup_old_compact_files() {
        for (auto &f : compact_files_) {
            if (f != current_vectors_file_) {
//		std::cerr << "REMOVE " << f << std::endl;
                std::remove(f.c_str());
            }
        }
        // Keep only the current one
        compact_files_.clear();
        compact_files_.push_back(current_vectors_file_);
    }

    bool finalize_compaction(const std::string& new_file) {

        // 1. Unmap previous mmap (if any)
        cleanup_mmap();  
        // This should set main_mmap_ptr_ = nullptr and clear file handles internally.

        // 2. Set new compact file as the active one
        current_vectors_file_ = new_file;

        // 3. Load (mmap) the new compact file
        if (!load_vectors_from_vectors_file(current_vectors_file_)) {
            HNSWERR << "Failed to load freshly compacted file: '" << current_vectors_file_ << "'\n";
            return false;
        }

        // 4. Cleanup delta caches (all deltas now merged)
        cleanup_deltas();

        // 5. Remove older compact vector files
        cleanup_old_compact_files();

        // 6. Success
        return true;
    }


}; // class LSMVectorStorage


} // namespace hnswlib

