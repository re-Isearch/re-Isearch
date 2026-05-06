#pragma once
#include <unordered_map>
#include <list>
#include <string>
#include <memory>
#include <mutex>
#include <functional>
#include <fstream>
#include <cstdint>


#define LRU_CACHE 1

template <typename Key, typename Value>
class LRUCache {
public:
    using value_ptr = std::shared_ptr<Value>;
    using deleter_t = std::function<void(const Key&, value_ptr)>;

    explicit LRUCache(size_t capacity_, deleter_t on_evict_ = nullptr)
        : capacity(capacity_), on_evict(on_evict_) {}

    value_ptr get(const Key &key) {
        std::lock_guard<std::mutex> lock(mu);
        auto it = cache.find(key);
        if (it == cache.end()) return nullptr;

        // Move accessed key to front (most recently used)
        usage.splice(usage.begin(), usage, it->second.second);
        return it->second.first;
    }

    void put(const Key &key, value_ptr val) {
        std::lock_guard<std::mutex> lock(mu);
        auto it = cache.find(key);
        if (it != cache.end()) {
            // Update value and move to front
            it->second.first = val;
            usage.splice(usage.begin(), usage, it->second.second);
            return;
        }

        // New insertion
        usage.push_front(key);
        cache[key] = { val, usage.begin() };

        if (cache.size() > capacity) {
            evict_one();
        }
    }

    bool contains(const Key &key) {
        std::lock_guard<std::mutex> lock(mu);
        return cache.find(key) != cache.end();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mu);
        if (on_evict) {
            for (auto &p : cache)
                on_evict(p.first, p.second.first);
        }
        cache.clear();
        usage.clear();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mu);
        return cache.size();
    }

    template <typename Func>
    void for_each(Func fn) {
        for (auto &pair : cache) {
            fn(pair.first, pair.second.first);
        }
    }


private:
    void evict_one() {
        if (usage.empty()) return;
        auto last = usage.back();
        auto it = cache.find(last);
        if (it != cache.end()) {
            if (on_evict) on_evict(it->first, it->second.first);
            cache.erase(it);
        }
        usage.pop_back();
    }

    size_t capacity;
    mutable std::mutex mu;
    std::list<Key> usage;
    std::unordered_map<Key, std::pair<value_ptr, typename std::list<Key>::iterator>> cache;
    deleter_t on_evict;
};



#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/mach_host.h>
#include <mach/vm_statistics.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

class HNSWCacheConfig {

/*
The memory usage for an HNSWlib index depends on several parameters:
Basic formula: Memory per vector is approximately (d * 4 + M * 2 * 4) bytes

Vector storage: 384 × 4 bytes = 1,536 bytes
Graph structure: 16 × 2 × 4 bytes = 128 bytes
Total per vector: ~1,664 bytes
For 100,000 vectors: ~158 MB (plus overhead)

Key factors affecting memory:

Vector dimension (largest contributor)
M parameter (higher M = more connections = more memory)
max_elements (reserves space upfront)
Overhead for index structures (~10-20% additional)

A conservative estimate would be 200-500 MB per 100k element index depending
on vector dimensions.
*/
public:
    // Estimate average HNSW index size in memory (in bytes)
    // This should be tuned based on your actual index characteristics
    static constexpr size_t DEFAULT_INDEX_SIZE_MB = 350;  // 350 MB per index
    
    // Safety margins
    static constexpr double MIN_FREE_RAM_RATIO = 0.2;     // Keep at least 20% RAM free
    static constexpr double MAX_RAM_USAGE_RATIO = 0.6;    // Use at most 60% of available RAM
    
    struct CacheParams {
        size_t max_cache_size;
        size_t available_ram_mb;
        size_t estimated_index_size_mb;
    };
    
    static size_t get_available_ram_bytes() {
#ifdef __linux__
        std::ifstream meminfo("/proc/meminfo");
        std::string line;
        size_t mem_available = 0;
        
        while (std::getline(meminfo, line)) {
            if (line.substr(0, 12) == "MemAvailable") {
                size_t pos = line.find(':');
                if (pos != std::string::npos) {
                    mem_available = std::stoull(line.substr(pos + 1));
                    break;
                }
            }
        }
        return mem_available * 1024;  // Convert from KB to bytes
#elif defined(_WIN32)
        MEMORYSTATUSEX status;
        status.dwLength = sizeof(status);
        GlobalMemoryStatusEx(&status);
        return status.ullAvailPhys;
#elif defined(__APPLE__)
        vm_size_t page_size;
        mach_port_t mach_port;
        mach_msg_type_number_t count;
        vm_statistics64_data_t vm_stats;
        
        mach_port = mach_host_self();
        count = sizeof(vm_stats) / sizeof(natural_t);
        host_page_size(mach_port, &page_size);
        
        if (host_statistics64(mach_port, HOST_VM_INFO,
                            (host_info64_t)&vm_stats, &count) == KERN_SUCCESS) {
            uint64_t free_memory = (uint64_t)vm_stats.free_count * (uint64_t)page_size;
            uint64_t inactive_memory = (uint64_t)vm_stats.inactive_count * (uint64_t)page_size;
            return free_memory + inactive_memory;
        }
        return 0;
#else
        return 0;  // Unsupported platform
#endif
    }
    
    static CacheParams calculate_cache_size(
        size_t estimated_index_size_bytes = DEFAULT_INDEX_SIZE_MB * 1024 * 1024,
        double ram_usage_ratio = MAX_RAM_USAGE_RATIO
    ) {
        size_t available_ram = get_available_ram_bytes();
        
        if (available_ram == 0) {
            // Fallback: default to small cache if we can't determine RAM
            return {2, 0, DEFAULT_INDEX_SIZE_MB};
        }
        
        // Calculate usable RAM for caching
        size_t usable_ram = static_cast<size_t>(available_ram * ram_usage_ratio);
        
        // Calculate number of indexes that fit
        size_t max_indexes = usable_ram / estimated_index_size_bytes;
        
        // Ensure at least 1, cap at reasonable maximum
        max_indexes = std::max<size_t>(1, std::min<size_t>(max_indexes, 1000));
        
        return {
            max_indexes,
            available_ram / (1024 * 1024),  // Convert to MB for reporting
            estimated_index_size_bytes / (1024 * 1024)
        };
    }
    
    // Helper to dynamically adjust based on actual loaded index size
    static size_t calculate_cache_size_from_sample(size_t sample_index_size_bytes) {
        auto params = calculate_cache_size(sample_index_size_bytes);
        return params.max_cache_size;
    }
};

#if 0
// Usage example:
inline size_t determine_hnsw_cache_size() {
    auto params = HNSWCacheConfig::calculate_cache_size();
    
    std::cout << "Available RAM: " << params.available_ram_mb << " MB\n"
              << "Estimated index size: " << params.estimated_index_size_mb << " MB\n"
              << "Recommended cache size: " << params.max_cache_size << " indexes\n";
    return params.max_cache_size;
}

// If you know the actual size of your indexes:
inline size_t determine_hnsw_cache_size_custom(size_t your_index_size_mb) {
    return HNSWCacheConfig::calculate_cache_size(
        your_index_size_mb * 1024 * 1024
    ).max_cache_size;
}
#endif



/*
Dimension of Common SBERT Models:

all-MiniLM-L6-v2            384    Most popular, fast, good quality
all-mpnet-base-v2           768    Higher quality, slower
all-MiniLM-L12-v2           384    Balance of speed/quality
paraphrase-MiniLM-L6-v2     384    Paraphrase detection
paraphrase-mpnet-base-v2    768    Paraphrase detection
multi-qa-MiniLM-L6-cos-v1   384    Question answering
multi-qa-mpnet-base-cos-v1  768    Question answering

For 384D SBERT vectors:

Float32: 1,536 bytes                   (384 × 4) + 160 = 1,696 bytes 

Binary:  48 bytes (32x compression)    (384 ÷ 8) + 160 = 208 bytes (-88% memory)
Binary + rescoring --> 1,584 bytes (48 + 160 + 1,536) = 1744 bytes (+2% memory)

Ternary: 96 bytes (16x compression)    (384 ÷ 4) + 160 = 256 bytes (-85% memory)
Ternary + rescoring --> 1,584 bytes (96 + 160 + 1,536) = 1792 bytes (+6% memory)

*/

inline size_t est_bytes_per_vector(const hnswlib::HnswConfig &cfg, size_t dim)
{
#if 1
   // Some constants. We may adjust later.
   static constexpr float  extra_overhead_factor = 0.05; // 5% extra
   static constexpr size_t link_bytes = 4;          // int32 neighbor id
   static constexpr double layer_factor = 1.2;      // multi-layer expansion
   static constexpr size_t per_node_overhead = 12;  // level + offsets

   size_t graph_bytes  = static_cast<size_t>( cfg.M * link_bytes * layer_factor);

   // NOTE: We ignore the extra overhead of RaBitQ etc.  But added 5%
   size_t bits_per_component = hnswlib::IntStorage::bits_per_element(cfg.storage_type());
   size_t vector_bits = dim * bits_per_component;
   size_t vector_bytes = (vector_bits + 7) / 8;

   size_t base_total = vector_bytes + graph_bytes + per_node_overhead;

   size_t total = static_cast<size_t>( total * (1.0 + extra_overhead_factor));

   // If rescoring is enabled then we need to add dim * sizeof(float);

   if (cfg.mode() != hnswlib::OptBinMode::PASS && cfg.enable_rescoring() && bits_per_component < 32)
      total += dim * sizeof(float);
   return total;
#else
    size_t vector_bytes = dim * sizeof(float);
    
    if (cfg.metric == MetricSpace::Binary || cfg.metric == MetricSpace::Ternary) {
        size_t quant_bytes;
        if (cfg.metric == MetricSpace::Binary) {
            quant_bytes = ((dim + 63) / 64) * 8;  // 48 bytes for 384D
        } else {
            quant_bytes = (dim + 3) / 4;           // 96 bytes for 384D
        }
        
        size_t graph_overhead = cfg.M * 10;  // ~160 bytes for M=16
        size_t total = quant_bytes + graph_overhead;
        
        if (cfg.enable_rescoring) {
            total += vector_bytes;  // Add original 1536 bytes for 384D
        }
        
        return total;
    } else {
        // Float metrics
        size_t graph_overhead = cfg.M * 10;  // ~160 bytes for M=16
        return vector_bytes + graph_overhead;
    }
#endif
}

inline size_t determine_optimal_hnsw_cache_size(const hnswlib::HnswConfig &cfg, size_t dim = 384)
{
  const size_t size = est_bytes_per_vector(cfg, dim);
  const size_t size_for_index = size*cfg.max_elements;

  if (cfg.debug) LOG_DEBUG_S() << "Avg. consumption per "
        << dim << "D " << hnswlib::metric_to_string(cfg.metric())
        << " vector: ~" << size/(1024*1024) << " MBytes";
    
    return HNSWCacheConfig::calculate_cache_size(size_for_index).max_cache_size;
}
