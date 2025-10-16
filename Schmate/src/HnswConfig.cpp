#include "HnswConfig.hpp"
#include "Logger.hpp"
#include <thread>


    // Validation
    bool HnswConfig::validate() const {
        if (max_elements == 0) {
            LOG_ERROR_S() << "max_elements must be > 0";
            return false;
        }
        if (M == 0 || M > 128) {
            LOG_ERROR_S() << "M must be in range [1, 128]";
            return false;
        }
        if (ef_construction < M * 2) {
            LOG_WARN_S() << "ef_construction should be >= 2*M for good quality";
        }

/*

| Use Case                           | ef_search | Description                                               |
| ---------------------------------- | --------- | --------------------------------------------------------- |
| 🔹 **Speed-critical (real-time)**  | `16–64`   | Fast, but may miss some near neighbors.                   |
| 🔹 **Balanced (default)**          | `100–200` | Common sweet spot (used by FAISS, sentence-transformers). |
| 🔹 **High recall / offline batch** | `300–800` | High accuracy, slower query.                              |
| 🔹 **Max recall / evaluation**     | `1000+`   | Nearly exact results, but 10× slower.                     |

*/
        if (ef_search == 0) {
            LOG_ERROR_S() << "ef_search must be > 0";
            return false;
        }
/*
Rule of Thumb:
| Index Size | ef_search |
| ---------- | --------- |
| < 10 K     | 64–100    |
| 10 K–100 K | 100–200   |
| 100 K–1 M  | 200–400   |
| > 1 M      | 300–800+  |

*/
	if (ef_search > 900) {
	    if (ef_search < 1500)
	      LOG_WARN_S() << "ef_search " << ef_search << " is at least 10x slower than 300-800";
	    else 
	      LOG_ERROR_S() << "ef_search " << ef_search << " seems wrong!";
	    auto val = std::clamp(20 * std::sqrt(default_k), 50.0, 400.0);
	    LOG_INFO_S() << "A value of " << val << " might be a better choice";
	    if (ef_search > 1500) return false;
        }
        if (overlap_percent < 0.0f || overlap_percent >= 1.0f) {
            LOG_ERROR_S() << "overlap_percent must be in [0, 1)";
            return false;
        }
        if (max_tokens_per_chunk <= 0) {
            LOG_ERROR_S() << "max_tokens_per_chunk must be > 0";
            return false;
        }
        if (default_k == 0) {
            LOG_ERROR_S() << "default_k must be > 0";
            return false;
        }
        if (default_radius < 0.0f || default_radius > 1.0f) {
            LOG_WARN_S() << "default_radius typically in [0, 1]";
        }
        return true;
    }

    // Get epsilon for current metric
    float HnswConfig::get_epsilon() const {
        if (default_epsilon >= 0.0f) {
            return default_epsilon;
        }
        
        // Fall back to metric-specific epsilon
        switch (metric) {
            case Metric::L2:
                return default_epsilonL2 * default_epsilonL2; // Square it
            case Metric::InnerProduct:
            case Metric::Cosine:
                return default_epsilonIP;
            default:
                return 0.15f;
        }
    }

    // Get effective max_candidates (with cap applied)
    size_t HnswConfig::get_max_candidates(size_t request) const {
        if (request == 0) request = default_k * knn_lookahead_scale;
        if (max_candidates_cap > 0) {
            return std::min(request, max_candidates_cap);
        }
        return request;
    }

    // Get number of merge threads
    unsigned HnswConfig::get_merge_threads() const {
        if (merge_threads == 0) {
            return std::thread::hardware_concurrency();
        }
        return merge_threads;
    }

    // Binary serialization
    void HnswConfig::save(std::ostream& os) const {
        // Version marker
        uint32_t version = 1;
        os.write((char*)&version, sizeof(version));
        
        // Write all fields
        os.write((char*)this, sizeof(HnswConfig));
    }

    void HnswConfig::load(std::istream& is) {
        // Read version
        uint32_t version;
        is.read((char*)&version, sizeof(version));
        
        if (version != 1) {
            throw std::runtime_error("Unsupported config version");
        }
        
        // Read all fields
        is.read((char*)this, sizeof(HnswConfig));
        
        if (!validate()) {
            throw std::runtime_error("Loaded invalid configuration");
        }
    }

    // Save to file
    bool HnswConfig::save_to_file(const std::string& path) const {
        std::ofstream ofs(path, std::ios::binary);
        if (!ofs) {
            LOG_ERROR_S() << "Failed to open " << path << " for writing (save config)";
            return false;
        }
        save(ofs);
        return true;
    }

    // Load from file
    bool HnswConfig::load_from_file(const std::string& path) {
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs) {
            return false;  // File doesn't exist, not an error
        }
        try {
            load(ifs);
            return true;
        } catch (const std::exception& e) {
            LOG_ERROR_S() << "Error loading config from " << path << ": " << e.what();
            return false;
        }
    }

    // Merge/override with another config
    // Only copies fields that differ from defaults
    void HnswConfig::merge_from(const HnswConfig& other) {
        // Copy all fields from other
        // This is a simple approach - copies everything
        *this = other;
    }

    // Selective merge (only override non-default values)
    void HnswConfig::merge_overrides(const HnswConfig& override, const HnswConfig& defaults) {
        // Helper macro to check and override
        #define OVERRIDE_IF_DIFFERENT(field) \
            if (override.field != defaults.field) { \
                this->field = override.field; \
            }
        
        OVERRIDE_IF_DIFFERENT(default_search_mode);
        OVERRIDE_IF_DIFFERENT(max_elements);
        OVERRIDE_IF_DIFFERENT(M);
        OVERRIDE_IF_DIFFERENT(ef_construction);
        OVERRIDE_IF_DIFFERENT(ef_search);
        OVERRIDE_IF_DIFFERENT(metric);
        OVERRIDE_IF_DIFFERENT(bert_n_threads);
        OVERRIDE_IF_DIFFERENT(max_tokens_per_chunk);
        OVERRIDE_IF_DIFFERENT(overlap_percent);
        OVERRIDE_IF_DIFFERENT(debug);
        OVERRIDE_IF_DIFFERENT(default_k);
        OVERRIDE_IF_DIFFERENT(default_radius);
        OVERRIDE_IF_DIFFERENT(default_alpha);
        OVERRIDE_IF_DIFFERENT(default_minN);
        OVERRIDE_IF_DIFFERENT(default_lookahead);
        OVERRIDE_IF_DIFFERENT(default_gapDelta);
        OVERRIDE_IF_DIFFERENT(default_epsilon);
        OVERRIDE_IF_DIFFERENT(default_epsilonL2);
        OVERRIDE_IF_DIFFERENT(default_epsilonIP);
        OVERRIDE_IF_DIFFERENT(min_candidates);
        OVERRIDE_IF_DIFFERENT(max_candidates_cap);
        OVERRIDE_IF_DIFFERENT(knn_lookahead_scale);
        OVERRIDE_IF_DIFFERENT(flush_threshold);
        OVERRIDE_IF_DIFFERENT(flush_offsets_each);
        OVERRIDE_IF_DIFFERENT(parallel_merge);
        OVERRIDE_IF_DIFFERENT(merge_threads);
        OVERRIDE_IF_DIFFERENT(normalized_embeddings);
        
        #undef OVERRIDE_IF_DIFFERENT
    }

    // Print configuration
    void HnswConfig::print(std::ostream& os) const {
        os << "=== HNSW Configuration ===\n";
        os << "Search mode: " << search_mode_to_string(default_search_mode) << "\n";
        os << "\nIndex parameters:\n";
        os << "  max_elements: " << max_elements << "\n";
        os << "  M: " << M << "\n";
        os << "  ef_construction: " << ef_construction << "\n";
        os << "  ef_search: " << ef_search << "\n";
        os << "  metric: " << metric_to_string(metric) << "\n";
        os << "  normalized_embeddings: " << (normalized_embeddings ? "yes" : "no") << "\n";
        
        os << "\nEmbedding:\n";
        os << "  bert_n_threads: " << bert_n_threads << "\n";
        
        os << "\nChunking:\n";
        os << "  max_tokens_per_chunk: " << max_tokens_per_chunk << "\n";
        os << "  overlap_percent: " << overlap_percent << "\n";
        
        os << "\nSearch defaults:\n";
        os << "  k (knn): " << default_k << "\n";
        os << "  radius: " << default_radius << "\n";
        os << "  alpha (relative): " << default_alpha << "\n";
        os << "  minN (adaptive): " << default_minN << "\n";
        os << "  lookahead (adaptive): " << default_lookahead << "\n";
        os << "  gapDelta (adaptive): " << default_gapDelta << "\n";
        
        os << "\nEpsilon search:\n";
        os << "  epsilon: " << default_epsilon << "\n";
        os << "  epsilonL2: " << default_epsilonL2 << "\n";
        os << "  epsilonIP: " << default_epsilonIP << "\n";
        os << "  min_candidates: " << min_candidates << "\n";
        os << "  max_candidates_cap: " << max_candidates_cap << "\n";
        
        os << "\nPerformance:\n";
        os << "  knn_lookahead_scale: " << knn_lookahead_scale << "\n";
        os << "  flush_threshold: " << flush_threshold << "\n";
        os << "  flush_offsets_each: " << (flush_offsets_each ? "yes" : "no") << "\n";
        os << "  parallel_merge: " << (parallel_merge ? "yes" : "no") << "\n";
        os << "  merge_threads: " << get_merge_threads() << "\n";
        
        os << "\nDebug: " << (debug ? "enabled" : "disabled") << "\n";
   }



    // Dynamic setter by string key
    bool HnswConfig::set(const std::string& key, const std::string& value) {
        try {
            // size_t fields
            if (key == "max_elements") { max_elements = std::stoull(value); return true; }
            if (key == "M") { M = std::stoull(value); return true; }
            if (key == "ef_construction") { ef_construction = std::stoull(value); return true; }
            if (key == "ef_search") { ef_search = std::stoull(value); return true; }
            if (key == "bert_n_threads") { bert_n_threads = std::stoull(value); return true; }
            if (key == "default_k") { default_k = std::stoull(value); return true; }
            if (key == "default_minN") { default_minN = std::stoull(value); return true; }
            if (key == "default_lookahead") { default_lookahead = std::stoull(value); return true; }
            if (key == "min_candidates") { min_candidates = std::stoull(value); return true; }
            if (key == "max_candidates_cap") { max_candidates_cap = std::stoull(value); return true; }
            if (key == "knn_lookahead_scale") { knn_lookahead_scale = std::stoull(value); return true; }
            if (key == "merge_threads") { merge_threads = std::stoul(value); return true; }
            
            // int fields
            if (key == "max_tokens_per_chunk") { max_tokens_per_chunk = std::stoi(value); return true; }
            if (key == "flush_threshold") { flush_threshold = std::stoi(value); return true; }
            
            // float fields
            if (key == "overlap_percent") { overlap_percent = std::stof(value); return true; }
            if (key == "default_radius") { default_radius = std::stof(value); return true; }
            if (key == "default_alpha") { default_alpha = std::stof(value); return true; }
            if (key == "default_gapDelta") { default_gapDelta = std::stof(value); return true; }
            if (key == "default_epsilon") { default_epsilon = std::stof(value); return true; }
            if (key == "default_epsilonL2") { default_epsilonL2 = std::stof(value); return true; }
            if (key == "default_epsilonIP") { default_epsilonIP = std::stof(value); return true; }
            
            // bool fields
            if (key == "debug") { debug = parse_bool(value); return true; }
            if (key == "flush_offsets_each") { flush_offsets_each = parse_bool(value); return true; }
            if (key == "parallel_merge") { parallel_merge = parse_bool(value); return true; }
            if (key == "normalized_embeddings") { normalized_embeddings = parse_bool(value); return true; }
            
            // enum fields
            if (key == "metric") { metric = string_to_metric(value); return true; }
            if (key == "default_search_mode") { default_search_mode = string_to_search_mode(value); return true; }
            
            std::cerr << "Unknown config key: " << key << "\n";
            return false;
            
        } catch (const std::exception& e) {
            std::cerr << "Error setting " << key << "=" << value << ": " << e.what() << "\n";
            return false;
        }
    }


    // Dynamic getter by string key
    std::string HnswConfig::get(const std::string& key) const {
        // size_t fields
        if (key == "max_elements") return std::to_string(max_elements);
        if (key == "M") return std::to_string(M);
        if (key == "ef_construction") return std::to_string(ef_construction);
        if (key == "ef_search") return std::to_string(ef_search);
        if (key == "bert_n_threads") return std::to_string(bert_n_threads);
        if (key == "default_k") return std::to_string(default_k);
        if (key == "default_minN") return std::to_string(default_minN);
        if (key == "default_lookahead") return std::to_string(default_lookahead);
        if (key == "min_candidates") return std::to_string(min_candidates);
        if (key == "max_candidates_cap") return std::to_string(max_candidates_cap);
        if (key == "knn_lookahead_scale") return std::to_string(knn_lookahead_scale);
        if (key == "merge_threads") return std::to_string(merge_threads);
        
        // int fields
        if (key == "max_tokens_per_chunk") return std::to_string(max_tokens_per_chunk);
        if (key == "flush_threshold") return std::to_string(flush_threshold);
        
        // float fields
        if (key == "overlap_percent") return std::to_string(overlap_percent);
        if (key == "default_radius") return std::to_string(default_radius);
        if (key == "default_alpha") return std::to_string(default_alpha);
        if (key == "default_gapDelta") return std::to_string(default_gapDelta);
        if (key == "default_epsilon") return std::to_string(default_epsilon);
        if (key == "default_epsilonL2") return std::to_string(default_epsilonL2);
        if (key == "default_epsilonIP") return std::to_string(default_epsilonIP);
        
        // bool fields
        if (key == "debug") return debug ? "true" : "false";
        if (key == "flush_offsets_each") return flush_offsets_each ? "true" : "false";
        if (key == "parallel_merge") return parallel_merge ? "true" : "false";
        if (key == "normalized_embeddings") return normalized_embeddings ? "true" : "false";
        
        // enum fields
        if (key == "metric") return metric_to_string(metric);
        if (key == "default_search_mode") return search_mode_to_string(default_search_mode);
        
        throw std::runtime_error("Unknown config key: " + key);
    }

