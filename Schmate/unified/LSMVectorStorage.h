// LSM-tree style vector storage with periodic compaction
#pragma once

#include "hnswlib/hnswlib.h"
#include <fcntl.h>
#include <cstring>
#include <numeric>
#include <cmath>
#include <memory>
#include <unordered_map>


namespace hnswlib {

enum class VectorStorageMode {
    DISABLED,     // Skip vectors entirely
    IN_MEMORY,    // Load all vectors into RAM
    MEMORY_MAPPED // Use mmap for on-demand access
};


class LSMVectorStorage {
private:
    VectorStorageMode storage_mode_;
    std::string base_filename_;
    size_t dim_;
    
    // Main file (memory-mapped, read-only)
    void* main_mmap_ptr_ = nullptr;
    size_t main_mmap_size_ = 0;
    int main_mmap_fd_ = -1;
    size_t main_vectors_offset_ = 0;
    std::unordered_map<labeltype, size_t> main_offsets_;
    
    // Delta files tracking
    std::vector<std::string> delta_files_;
    // NEW: Cache delta files in memory to avoid disk I/O and memory leaks
    std::vector<std::unordered_map<labeltype, std::vector<float>>> delta_caches_;
    
    // OPTIMIZATION: Track which labels exist in each delta for fast negative lookups
    // If label not in set, we can skip searching that delta entirely
    std::vector<std::unordered_set<labeltype>> delta_label_sets_;
    
    // Current in-memory delta (not yet flushed)
    std::unordered_map<labeltype, std::vector<float>> current_delta_;
    size_t additions_since_flush_ = 0;
    size_t flush_threshold_;  // Your configurable X
    
public:
    LSMVectorStorage(size_t dim, size_t flush_threshold = 10000) 
        : dim_(dim), flush_threshold_(flush_threshold) {}
    
    ~LSMVectorStorage() {
        cleanup();
    }
    
    // Initial load from combined index file (not standalone)
    bool load_vectors(const std::string& filename, std::ifstream& ifs,
                      size_t num_vectors, VectorStorageMode mode,
                      size_t flush_threshold) {
        storage_mode_ = mode;
        base_filename_ = filename;
        flush_threshold_ = flush_threshold;
        
        if (mode == VectorStorageMode::DISABLED) {
            for (size_t i = 0; i < num_vectors; i++) {
                ifs.seekg(sizeof(labeltype) + dim_ * sizeof(float), std::ios::cur);
            }
            return true;
        }
        
        if (mode == VectorStorageMode::IN_MEMORY) {
            return load_in_memory(ifs, num_vectors);
        }
        
        if (mode == VectorStorageMode::MEMORY_MAPPED) {
            return load_with_mmap(ifs, num_vectors);
        }
        
        return false;
    }
    
    // Alternative: Load when vectors are at a known offset in combined file
    bool load_vectors_at_offset(const std::string& filename, 
                                 size_t file_offset,
                                 size_t num_vectors, 
                                 VectorStorageMode mode,
                                 size_t flush_threshold) {
        storage_mode_ = mode;
        base_filename_ = filename;
        flush_threshold_ = flush_threshold;
        main_vectors_offset_ = file_offset;
        
        if (mode == VectorStorageMode::DISABLED) {
            return true;
        }
        
        if (mode == VectorStorageMode::IN_MEMORY) {
            std::ifstream ifs(filename, std::ios::binary);
            ifs.seekg(file_offset);
            return load_in_memory(ifs, num_vectors);
        }
        
        if (mode == VectorStorageMode::MEMORY_MAPPED) {
            // For mmap, we just need the offset - we'll map later
            size_t entry_size = sizeof(labeltype) + dim_ * sizeof(float);
            main_mmap_size_ = num_vectors * entry_size;
            
            // Build index by reading labels only
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
            
            // Setup mmap
            #ifdef _WIN32
            return setup_mmap_windows();
            #else
            return setup_mmap_unix();
            #endif
        }
        
        return false;
    }
    
private:
    bool load_with_mmap(std::ifstream& ifs, size_t num_vectors) {
        main_vectors_offset_ = ifs.tellg();
        
        // Build index
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
        
        // Setup mmap
        #ifdef _WIN32
        return setup_mmap_windows();
        #else
        return setup_mmap_unix();
        #endif
    }
    
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
            return false;
        }
        
        main_mmap_ptr_ = static_cast<char*>(map_base) + offset_adjustment;
        madvise(map_base, map_size, MADV_RANDOM);
        
        return true;
    }
    #endif
    
    #ifdef _WIN32
    bool setup_mmap_windows() {
        // Similar to previous implementation
        // ...
        return true;
    }
    #endif
    
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
    // Add a new vector
    void addPoint(labeltype label, const float* data) {
        if (storage_mode_ == VectorStorageMode::DISABLED) return;
        
        // Add to current delta (in-memory)
        current_delta_[label] = std::vector<float>(data, data + dim_);
        additions_since_flush_++;
        
        // Auto-flush if threshold reached
        if (additions_since_flush_ >= flush_threshold_) {
            flush_delta();
        }
    }
    
    // Flush current delta to a delta file
    bool flush_delta() {
        if (current_delta_.empty()) return true;
        
        // Create delta file name
        std::string delta_file = base_filename_ + ".delta." + 
                                std::to_string(delta_files_.size());
        
        std::ofstream ofs(delta_file, std::ios::binary);
        if (!ofs) return false;
        
        // Write delta header
        size_t num_vectors = current_delta_.size();
        ofs.write(reinterpret_cast<const char*>(&num_vectors), sizeof(size_t));
        
        // Write vectors
        for (const auto& [label, vec] : current_delta_) {
            ofs.write(reinterpret_cast<const char*>(&label), sizeof(labeltype));
            ofs.write(reinterpret_cast<const char*>(vec.data()), dim_ * sizeof(float));
        }
        
        if (!ofs.good()) return false;
        
        delta_files_.push_back(delta_file);
        
        // OPTIMIZATION: Keep delta in memory for fast reads
        delta_caches_.push_back(std::move(current_delta_));
        
        // OPTIMIZATION: Build label set for fast negative lookups
        std::unordered_set<labeltype> label_set;
        for (const auto& [label, vec] : delta_caches_.back()) {
            label_set.insert(label);
        }
        delta_label_sets_.push_back(std::move(label_set));
        
        current_delta_.clear();
        additions_since_flush_ = 0;
        
        return true;
    }
    
    // Get vector - optimized with label set filtering
    const float* get_vector(labeltype label) const {
        // 1. Check current in-memory delta (most recent)
        auto it = current_delta_.find(label);
        if (it != current_delta_.end()) {
            return it->second.data();
        }
        
        // 2. Check cached delta files (reverse order - newest first)
        // OPTIMIZATION: Use label sets to skip deltas that don't contain this label
        for (size_t i = delta_caches_.size(); i > 0; --i) {
            size_t idx = i - 1;
            
            // Fast negative check: if label not in set, skip this delta
            if (delta_label_sets_[idx].find(label) == delta_label_sets_[idx].end()) {
                continue;  // Label definitely not in this delta
            }
            
            // Label might be in this delta, do actual lookup
            auto dit = delta_caches_[idx].find(label);
            if (dit != delta_caches_[idx].end()) {
                return dit->second.data();  // Found - most recent version
            }
        }
        
        // 3. Check main mmap (oldest version, only if not in deltas)
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
public:
    // Save vectors as part of combined index file
    // This is the main method called from UnifiedIndex::saveIndex()
    bool save_vectors_to_stream(std::ofstream& ofs) {
        auto all_vectors = get_all_vectors_for_save();
        
        // Write number of vectors
        size_t num_vectors = all_vectors.size();
        ofs.write(reinterpret_cast<const char*>(&num_vectors), sizeof(size_t));
        
        // Write all vectors
        for (const auto& [label, vec] : all_vectors) {
            ofs.write(reinterpret_cast<const char*>(&label), sizeof(labeltype));
            ofs.write(reinterpret_cast<const char*>(vec.data()), dim_ * sizeof(float));
        }
        
        return ofs.good();
    }
    
    // Get all vectors consolidated (main + deltas) - helper for save operations
    std::vector<std::pair<labeltype, std::vector<float>>> get_all_vectors_for_save() {
        // Flush any pending delta first
        if (!current_delta_.empty()) {
            flush_delta();
        }
        
        // Collect all vectors: main + all deltas
        std::unordered_map<labeltype, std::vector<float>> all_vectors;
        
        // 1. Load from main file if it exists
        if (main_mmap_ptr_) {
            for (const auto& [label, offset] : main_offsets_) {
                const float* vec_ptr = reinterpret_cast<const float*>(
                    static_cast<char*>(main_mmap_ptr_) + offset + sizeof(labeltype)
                );
                all_vectors[label] = std::vector<float>(vec_ptr, vec_ptr + dim_);
            }
        }
        
        // 2. Merge all deltas (newer versions overwrite older)
        for (const auto& delta_cache : delta_caches_) {
            for (const auto& [label, vec] : delta_cache) {
                all_vectors[label] = vec;
            }
        }
        
        // 3. Convert to vector for ordered writing
        std::vector<std::pair<labeltype, std::vector<float>>> result;
        result.reserve(all_vectors.size());
        for (auto& [label, vec] : all_vectors) {
            result.emplace_back(label, std::move(vec));
        }
        
        // 4. Clean up deltas after consolidation
        cleanup_deltas();
        
        return result;
    }
    
private:
    void cleanup_deltas() {
        // Delete delta files
        for (const auto& delta_file : delta_files_) {
            std::remove(delta_file.c_str());
        }
        delta_files_.clear();
        delta_caches_.clear();
        delta_label_sets_.clear();
    }
    // Original compact() method - now creates a temporary complete file
    bool compact() {
        if (delta_files_.empty() && current_delta_.empty()) {
            return true;  // Nothing to compact
        }
        
        // Get all vectors consolidated
        auto all_vectors = get_all_vectors_for_save();
        
        // Create a temporary vectors-only file for the compacted data
        std::string temp_vectors_file = base_filename_ + ".vectors.tmp";
        std::ofstream ofs(temp_vectors_file, std::ios::binary);
        if (!ofs) return false;
        
        size_t num_vectors = all_vectors.size();
        ofs.write(reinterpret_cast<const char*>(&num_vectors), sizeof(size_t));
        
        for (const auto& [label, vec] : all_vectors) {
            ofs.write(reinterpret_cast<const char*>(&label), sizeof(labeltype));
            ofs.write(reinterpret_cast<const char*>(vec.data()), dim_ * sizeof(float));
        }
        ofs.close();
        
        // Note: You'll need to rebuild the full combined index file
        // This temporary file can be used during that process
        
        return true;
    }
    
    // Get statistics
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
        
        // Could also count vectors in delta files if needed
        return stats;
    }
    
    bool is_rescoring_enabled() const {
        return storage_mode_ != VectorStorageMode::DISABLED;
    }
    
private:
    void cleanup_mmap() {
        #ifndef _WIN32
        if (main_mmap_ptr_ != nullptr) {
            // Need to unmap the adjusted base pointer
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
    
    void cleanup() {
        // Flush any pending additions
        flush_delta();
        cleanup_mmap();
        
        // Clear delta caches and label sets to free memory
        delta_caches_.clear();
        delta_label_sets_.clear();
    }
};

#if 0
// Usage with HNSW index flushing:
class HNSWIndex {
private:
    LSMVectorStorage vector_storage_;
    size_t additions_since_index_flush_ = 0;
    size_t index_flush_threshold_;
    
public:
    HNSWIndex(size_t dim, size_t flush_threshold) 
        : vector_storage_(dim, flush_threshold),
          index_flush_threshold_(flush_threshold) {}
    
    void addPoint(labeltype label, const float* data) {
        // Add to HNSW structure
        // ... your existing HNSW logic ...
        
        // Add to vector storage
        vector_storage_.addPoint(label, data);
        additions_since_index_flush_++;
        
        // Periodic flush of HNSW index
        if (additions_since_index_flush_ >= index_flush_threshold_) {
            flush_index();
        }
    }
    
    void flush_index() {
        // 1. Flush vector storage deltas
        vector_storage_.flush_delta();
        
        // 2. Flush HNSW index structure
        // ... your existing HNSW serialization ...
        
        additions_since_index_flush_ = 0;
    }
    
    void compact_and_save() {
        // Full compaction - called on final save or periodically
        vector_storage_.compact();
        // ... save HNSW structure ...
    }
};

// Example usage:
void example() {
    HNSWIndex index(128, 10000);  // dim=128, flush every 10k additions
    
    // Add vectors - automatically flushes every 10k
    for (int i = 0; i < 100000; i++) {
        float data[128];
        // ... fill data ...
        index.addPoint(i, data);
    }
    
    // Final save with compaction
    index.compact_and_save();
}
#endif

}; // namespace hnswlib
