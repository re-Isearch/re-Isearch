#pragma once 

#include <vector>
#include <cstring>
#include <numeric>
#include <cmath>
#include <memory>
#include <unordered_map>


// Add to your class definition
enum class VectorStorageMode {
    DISABLED,     // Skip vectors entirely
    IN_MEMORY,    // Load all vectors into RAM
    MEMORY_MAPPED // Use mmap for on-demand access
};

class VectorIndex {
private:
    VectorStorageMode storage_mode_;
    
    // For in-memory storage
    std::unordered_map<labeltype, std::vector<float>> original_vectors_;
    
    // For memory-mapped storage
    void* mmap_ptr_ = nullptr;
    size_t mmap_size_ = 0;
    int mmap_fd_ = -1;
    size_t vectors_file_offset_ = 0;  // Offset where vector data starts in file
    std::unordered_map<labeltype, size_t> label_to_offset_;  // Label -> offset relative to vectors_file_offset_
    std::string filename_;  // Store filename for mmap
    
public:
    // Load vectors based on chosen mode
    bool load_vectors(const std::string& filename, std::ifstream& ifs, 
                      size_t num_vectors, 
                      VectorStorageMode mode = VectorStorageMode::IN_MEMORY) {
        storage_mode_ = mode;
        filename_ = filename;  // Store for mmap
        
        if (storage_mode_ == VectorStorageMode::DISABLED) {
            // Skip all vector data
            for (size_t i = 0; i < num_vectors; i++) {
                ifs.seekg(sizeof(labeltype) + dim_ * sizeof(float), std::ios::cur);
            }
            return true;
        }
        
        if (storage_mode_ == VectorStorageMode::IN_MEMORY) {
            return load_vectors_in_memory(ifs, num_vectors);
        }
        
        if (storage_mode_ == VectorStorageMode::MEMORY_MAPPED) {
            return load_vectors_mmap(ifs, num_vectors);
        }
        
        return false;
    }
    
private:
    bool load_vectors_in_memory(std::ifstream& ifs, size_t num_vectors) {
        original_vectors_.clear();
        
        for (size_t i = 0; i < num_vectors; i++) {
            labeltype label;
            ifs.read(reinterpret_cast<char*>(&label), sizeof(labeltype));
            
            std::vector<float> vec(dim_);
            ifs.read(reinterpret_cast<char*>(vec.data()), dim_ * sizeof(float));
            
            if (!ifs.good()) return false;
            
            original_vectors_[label] = std::move(vec);
        }
        
        return true;
    }
    
    bool load_vectors_mmap(std::ifstream& ifs, size_t num_vectors) {
        // CRITICAL: Record current position in the file stream
        // This is where the vector data section starts
        vectors_file_offset_ = ifs.tellg();
        
        // Calculate the exact size of the vector data section
        size_t entry_size = sizeof(labeltype) + dim_ * sizeof(float);
        size_t vectors_section_size = num_vectors * entry_size;
        
        // Build label->offset index by reading through the data
        label_to_offset_.clear();
        size_t relative_offset = 0;
        
        for (size_t i = 0; i < num_vectors; i++) {
            labeltype label;
            ifs.read(reinterpret_cast<char*>(&label), sizeof(labeltype));
            
            // Store the offset where THIS label's data starts
            // Since we're mapping only the vector section, this is offset from start of mmap
            label_to_offset_[label] = relative_offset;
            
            // Skip the vector data - we'll access it via mmap later
            ifs.seekg(dim_ * sizeof(float), std::ios::cur);
            
            if (!ifs.good()) return false;
            
            // Move offset forward
            relative_offset += entry_size;
        }
        
        // Now map just the vector section
        mmap_size_ = vectors_section_size;
        
        #ifdef _WIN32
            return setup_mmap_windows();
        #else
            return setup_mmap_unix();
        #endif
    }
    
    #ifndef _WIN32
    bool setup_mmap_unix() {
        // Open the file that we were reading from
        mmap_fd_ = open(filename_.c_str(), O_RDONLY);
        if (mmap_fd_ == -1) {
            perror("Failed to open file for mmap");
            return false;
        }
        
        // Map ONLY the vector section of the file
        // Note: offset must be page-aligned for mmap
        size_t page_size = sysconf(_SC_PAGE_SIZE);
        size_t aligned_offset = (vectors_file_offset_ / page_size) * page_size;
        size_t offset_adjustment = vectors_file_offset_ - aligned_offset;
        size_t map_size = mmap_size_ + offset_adjustment;
        
        void* map_base = mmap(nullptr, map_size, PROT_READ, MAP_PRIVATE, mmap_fd_, aligned_offset);
        if (map_base == MAP_FAILED) {
            perror("Failed to mmap file");
            close(mmap_fd_);
            mmap_fd_ = -1;
            return false;
        }
        
        // Adjust pointer to actual start of vector data
        mmap_ptr_ = static_cast<char*>(map_base) + offset_adjustment;
        mmap_size_ = map_size;  // Store actual mapped size for cleanup
        
        // Hint to OS about access pattern - random access for rescoring
        madvise(map_base, map_size, MADV_RANDOM);
        
        return true;
    }
    #endif
    
    #ifdef _WIN32
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
    #endif
    
public:
    // Unified interface to get a vector regardless of storage mode
    const float* get_vector(labeltype label) const {
        if (storage_mode_ == VectorStorageMode::DISABLED) {
            return nullptr;
        }
        
        if (storage_mode_ == VectorStorageMode::IN_MEMORY) {
            auto it = original_vectors_.find(label);
            if (it == original_vectors_.end()) return nullptr;
            return it->second.data();
        }
        
        if (storage_mode_ == VectorStorageMode::MEMORY_MAPPED) {
            return get_vector_from_mmap(label);
        }
        
        return nullptr;
    }
    
private:
    const float* get_vector_from_mmap(labeltype label) const {
        if (!mmap_ptr_) return nullptr;
        
        auto it = label_to_offset_.find(label);
        if (it == label_to_offset_.end()) return nullptr;
        
        // Simple offset calculation now:
        // it->second = offset within the mapped region
        // + sizeof(labeltype) = skip past the label to get to the vector data
        size_t offset = it->second + sizeof(labeltype);
        
        // Bounds check
        if (offset + dim_ * sizeof(float) > mmap_size_) {
            return nullptr;
        }
        
        // Return pointer directly into mapped memory
        return reinterpret_cast<const float*>(
            static_cast<char*>(mmap_ptr_) + offset
        );
    }
    
public:
    // Cleanup
    ~VectorIndex() {
        cleanup_mmap();
    }
    
private:
    void cleanup_mmap() {
        #ifndef _WIN32
        if (mmap_ptr_ != nullptr && mmap_ptr_ != MAP_FAILED) {
            munmap(mmap_ptr_, mmap_size_);
            mmap_ptr_ = nullptr;
        }
        if (mmap_fd_ != -1) {
            close(mmap_fd_);
            mmap_fd_ = -1;
        }
        #else
        if (mmap_ptr_ != nullptr) {
            UnmapViewOfFile(mmap_ptr_);
            mmap_ptr_ = nullptr;
        }
        #endif
    }
};

    // Check if rescoring is available
    bool is_rescoring_enabled() const {
        return storage_mode_ != VectorStorageMode::DISABLED;
    }
    
    // Get storage mode info (useful for logging/diagnostics)
    VectorStorageMode get_storage_mode() const {
        return storage_mode_;
    }
};

// Updated rescoring code that works with all storage modes:
void rescore_candidates(const float* query_ptr,
                       const std::vector<std::pair<float, labeltype>>& all_candidates,
                       std::vector<std::pair<float, labeltype>>& rescored) {
    
    if (!is_rescoring_enabled()) {
        // If rescoring is disabled, just copy candidates as-is
        rescored = all_candidates;
        return;
    }
    
    rescored.clear();
    rescored.reserve(all_candidates.size());
    
    for (const auto& [dist, label] : all_candidates) {
        // Use unified interface - works for both in-memory and mmap
        const float* vec_ptr = get_vector(label);
        
        if (vec_ptr != nullptr) {
            float true_dist;
            if (metric_ == Metric::Cosine || metric_ == Metric::IP) {
                float sim = hnswlib::cosine_similarity(query_ptr, vec_ptr, dim_);
                true_dist = -sim;
            } else {
                true_dist = l2_distance(query_ptr, vec_ptr, dim_);
            }
            rescored.emplace_back(true_dist, label);
        } else {
            // Vector not found - this shouldn't happen normally
            // You might want to log this or use approximate distance as fallback
            rescored.emplace_back(dist, label);
        }
    }
}

// Usage example:
void example_usage() {
    VectorIndex index;
    
    std::string filename = "vectors.bin";
    std::ifstream ifs(filename, std::ios::binary);
    
    // ... read header, num_vectors, etc ...
    size_t num_vectors = 1000000;
    
    // IMPORTANT: Pass the filename so mmap can open the same file
    
    // Option 1: Load everything into RAM (fast access, high memory)
    index.load_vectors(filename, ifs, num_vectors, VectorStorageMode::IN_MEMORY);
    
    // Option 2: Use mmap (moderate access speed, low memory, OS handles paging)
    index.load_vectors(filename, ifs, num_vectors, VectorStorageMode::MEMORY_MAPPED);
    
    // Option 3: Skip entirely (no rescoring)
    index.load_vectors(filename, ifs, num_vectors, VectorStorageMode::DISABLED);
    
    // Rescoring now works uniformly across all modes:
    std::vector<std::pair<float, labeltype>> all_candidates = /* ... */;
    std::vector<std::pair<float, labeltype>> rescored;
    index.rescore_candidates(query, all_candidates, rescored);
}
