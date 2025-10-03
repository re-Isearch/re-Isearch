// Usage example
/*
using namespace std::chrono_literals;

hnswlib::L2Space space(dim);

HNSWIndexCache<float>::Config config;
config.capacity = 20;
config.async_load = true;
config.async_load_queue_size = 5;
config.max_idle_time = 300s;  // Evict after 5 min idle
config.auto_flush_on_evict = true;
config.collect_metrics = true;

HNSWIndexCache<float> cache(&space, config);

// Preload indexes you'll need soon
cache.preload_async("index1.bin");
cache.preload_async("index2.bin");

// Get index
auto* index = cache.get("index1.bin");
index->searchKnn(query, k, &result);

// Modify and mark dirty
index->addPoint(data, id);
cache.mark_dirty("index1.bin");

// Check metrics
auto metrics = cache.get_metrics();
std::cout << "Hit rate: " << metrics.hit_rate() << std::endl;
std::cout << "Avg load time: " << metrics.avg_load_time_ms() << "ms" << std::endl;

// Adjust config at runtime
auto new_config = cache.get_config();
new_config.capacity = 30;
cache.update_config(new_config);

// List what's cached
auto cached = cache.list_cached();
*/
