#pragma once

#include <unordered_map>
#include <list>
#include <mutex>
#include <memory>
#include <string>
#include <chrono>
#include <atomic>
#include <future>
#include <functional>
#include "hnswlib/hnswlib.h"

template<typename dist_t>
class HNSWIndexCache {
public:
    struct Config {
        size_t capacity = 10;
        bool auto_flush_on_evict = true;
        bool async_load = false;
        size_t async_load_queue_size = 2;
        bool collect_metrics = true;
        std::chrono::seconds max_idle_time = std::chrono::seconds(0);  // 0 = disabled
    };
    
    struct Metrics {
        std::atomic<uint64_t> hits{0};
        std::atomic<uint64_t> misses{0};
        std::atomic<uint64_t> evictions{0};
        std::atomic<uint64_t> loads{0};
        std::atomic<uint64_t> total_load_time_ms{0};
        std::atomic<uint64_t> flushes{0};
        
        double hit_rate() const {
            uint64_t total = hits + misses;
            return total > 0 ? (double)hits / total : 0.0;
        }
        
        double avg_load_time_ms() const {
            uint64_t l = loads;
            return l > 0 ? (double)total_load_time_ms / l : 0.0;
        }
        
        void reset() {
            hits = 0;
            misses = 0;
            evictions = 0;
            loads = 0;
            total_load_time_ms = 0;
            flushes = 0;
        }
    };

private:
    struct IndexEntry {
        std::string path;
        std::unique_ptr<hnswlib::HierarchicalNSW<dist_t>> index;
        bool dirty;
        std::chrono::steady_clock::time_point last_access;
        
        IndexEntry(const std::string& p, 
                   std::unique_ptr<hnswlib::HierarchicalNSW<dist_t>> idx,
                   bool d = false)
            : path(p), index(std::move(idx)), dirty(d), 
              last_access(std::chrono::steady_clock::now()) {}
    };
    
    using ListIterator = typename std::list<IndexEntry>::iterator;
    using LoadCallback = std::function<void(const std::string&, bool)>;
    
    std::list<IndexEntry> lru_list;
    std::unordered_map<std::string, ListIterator> index_map;
    std::unordered_map<std::string, std::future<std::unique_ptr<hnswlib::HierarchicalNSW<dist_t>>>> pending_loads;
    
    Config config;
    Metrics metrics;
    mutable std::mutex mutex;
    hnswlib::SpaceInterface<dist_t>* space;
    LoadCallback on_load_complete;
    
    std::unique_ptr<hnswlib::HierarchicalNSW<dist_t>> load_index(const std::string& path) {
        auto start = std::chrono::steady_clock::now();
        
        auto index = std::make_unique<hnswlib::HierarchicalNSW<dist_t>>(space, path);
        
        if (config.collect_metrics) {
            auto end = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            metrics.total_load_time_ms += duration;
            metrics.loads++;
        }
        
        return index;
    }
    
    void evict() {
        if (lru_list.empty()) return;
        
        auto& victim = lru_list.back();
        
        if (config.auto_flush_on_evict && victim.dirty) {
            victim.index->saveIndex(victim.path);
            if (config.collect_metrics) {
                metrics.flushes++;
            }
        }
        
        index_map.erase(victim.path);
        lru_list.pop_back();
        
        if (config.collect_metrics) {
            metrics.evictions++;
        }
    }
    
    void evict_idle() {
        if (config.max_idle_time.count() == 0) return;
        
        auto now = std::chrono::steady_clock::now();
        auto it = lru_list.rbegin();
        
        while (it != lru_list.rend()) {
            auto age = std::chrono::duration_cast<std::chrono::seconds>(now - it->last_access);
            if (age < config.max_idle_time) break;
            
            if (config.auto_flush_on_evict && it->dirty) {
                it->index->saveIndex(it->path);
                if (config.collect_metrics) {
                    metrics.flushes++;
                }
            }
            
            index_map.erase(it->path);
            it = decltype(it)(lru_list.erase(std::next(it).base()));
            
            if (config.collect_metrics) {
                metrics.evictions++;
            }
        }
    }
    
    void touch(ListIterator it) {
        it->last_access = std::chrono::steady_clock::now();
        lru_list.splice(lru_list.begin(), lru_list, it);
    }

public:
    HNSWIndexCache(hnswlib::SpaceInterface<dist_t>* sp, const Config& cfg = Config())
        : config(cfg), space(sp) {}
    
    ~HNSWIndexCache() {
        std::lock_guard<std::mutex> lock(mutex);
        for (auto& entry : lru_list) {
            if (config.auto_flush_on_evict && entry.dirty) {
                entry.index->saveIndex(entry.path);
            }
        }
    }
    
    hnswlib::HierarchicalNSW<dist_t>* get(const std::string& path) {
        std::lock_guard<std::mutex> lock(mutex);
        
        evict_idle();
        
        auto it = index_map.find(path);
        if (it != index_map.end()) {
            if (config.collect_metrics) {
                metrics.hits++;
            }
            touch(it->second);
            return it->second->index.get();
        }
        
        if (config.collect_metrics) {
            metrics.misses++;
        }
        
        // Check if already loading async
        auto pending_it = pending_loads.find(path);
        if (pending_it != pending_loads.end()) {
            auto index = pending_it->second.get();
            pending_loads.erase(pending_it);
            
            if (lru_list.size() >= config.capacity) {
                evict();
            }
            
            lru_list.emplace_front(path, std::move(index), false);
            index_map[path] = lru_list.begin();
            return lru_list.front().index.get();
        }
        
        // Synchronous load
        auto index = load_index(path);
        
        if (lru_list.size() >= config.capacity) {
            evict();
        }
        
        lru_list.emplace_front(path, std::move(index), false);
        index_map[path] = lru_list.begin();
        
        return lru_list.front().index.get();
    }
    
    void preload_async(const std::string& path) {
        if (!config.async_load) return;
        
        std::lock_guard<std::mutex> lock(mutex);
        
        if (index_map.count(path) || pending_loads.count(path)) {
            return;  // Already loaded or loading
        }
        
        if (pending_loads.size() >= config.async_load_queue_size) {
            return;  // Queue full
        }
        
        pending_loads[path] = std::async(std::launch::async, [this, path]() {
            return load_index(path);
        });
    }
    
    void mark_dirty(const std::string& path) {
        std::lock_guard<std::mutex> lock(mutex);
        
        auto it = index_map.find(path);
        if (it != index_map.end()) {
            it->second->dirty = true;
        }
    }
    
    void flush(const std::string& path) {
        std::lock_guard<std::mutex> lock(mutex);
        
        auto it = index_map.find(path);
        if (it != index_map.end() && it->second->dirty) {
            it->second->index->saveIndex(path);
            it->second->dirty = false;
            if (config.collect_metrics) {
                metrics.flushes++;
            }
        }
    }
    
    void flush_all() {
        std::lock_guard<std::mutex> lock(mutex);
        
        for (auto& entry : lru_list) {
            if (entry.dirty) {
                entry.index->saveIndex(entry.path);
                entry.dirty = false;
                if (config.collect_metrics) {
                    metrics.flushes++;
                }
            }
        }
    }
    
    void evict_index(const std::string& path) {
        std::lock_guard<std::mutex> lock(mutex);
        
        auto it = index_map.find(path);
        if (it != index_map.end()) {
            if (config.auto_flush_on_evict && it->second->dirty) {
                it->second->index->saveIndex(path);
                if (config.collect_metrics) {
                    metrics.flushes++;
                }
            }
            lru_list.erase(it->second);
            index_map.erase(it);
        }
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex);
        return lru_list.size();
    }
    
    void set_capacity(size_t cap) {
        std::lock_guard<std::mutex> lock(mutex);
        config.capacity = cap;
        while (lru_list.size() > config.capacity) {
            evict();
        }
    }
    
    void update_config(const Config& cfg) {
        std::lock_guard<std::mutex> lock(mutex);
        config = cfg;
        while (lru_list.size() > config.capacity) {
            evict();
        }
    }
    
    Config get_config() const {
        std::lock_guard<std::mutex> lock(mutex);
        return config;
    }
    
    Metrics get_metrics() const {
        return metrics;
    }
    
    void reset_metrics() {
        metrics.reset();
    }
    
    void set_load_callback(LoadCallback callback) {
        std::lock_guard<std::mutex> lock(mutex);
        on_load_complete = callback;
    }
    
    bool contains(const std::string& path) const {
        std::lock_guard<std::mutex> lock(mutex);
        return index_map.count(path) > 0;
    }
    
    std::vector<std::string> list_cached() const {
        std::lock_guard<std::mutex> lock(mutex);
        std::vector<std::string> result;
        for (const auto& entry : lru_list) {
            result.push_back(entry.path);
        }
        return result;
    }
};

