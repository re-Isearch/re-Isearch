#pragma once
#include <cstddef>
#include <string>
#include <fstream>
#include <iostream>


struct IndexFileExtensions {
  static constexpr const char* sentences= ".txt"; // Text of strings for debugging
  static constexpr const char* offsets  = ".obn"; // Offset file
  static constexpr const char* hnsw     = ".hix"; // HNSW index including Meta
  static constexpr const char* tuner    = ".eft"; // Ef search-time paramter learning
  static constexpr const char* eps      = ".eps"; // Epsilon learning
  static constexpr const char* hyparam  = ".hyp"; // Hyperparameters
  static constexpr const char* lock     = ".lock"; // Lock, 0 length = unlocked

} ;


enum class Metric {
    L2,
    InnerProduct,
    Cosine,
    Undefined
};

enum class SearchModes {
    Knn,
    Radius,
    Relative,
    Adaptive,
    Epsilon
};

struct HnswConfig {
    SearchModes default_search_mode = SearchModes::Knn;

    size_t max_elements = 100000;
    size_t M = 16;
    size_t ef_construction = 200;

/*   ef_search Rule of Thumb:
    | Index Size | ef_search |
    | ---------- | --------- |
    | < 10 K     | 64–100    |
    | 10 K–100 K | 100–200   |
    | 100 K–1 M  | 200–400   |
    | > 1 M      | 300–800+  | */
    size_t ef_search = 64;
    Metric metric = Metric::Cosine;

    size_t bert_n_threads = 4;

    // chunking
    int max_tokens_per_chunk = 128;
    float overlap_percent = 0.1f;

    // debug
    bool debug = true;  // default debug enabled

    // search defaults
    size_t default_k = 5;          // k for knn
    float default_radius = 0.7f;   // min score for radius
    float default_alpha = 0.8f;    // relative threshold multiplier
    size_t default_minN = 3;       // adaptive: minimum results
    size_t default_lookahead = 10; // adaptive: lookahead window
    float default_gapDelta = 0.1f; // adaptive: gap threshold

    // Epsilon search
    float default_epsilon   = 0.15f; // epsilon, if < 0 then use radius
    float default_epsilonL2 = 1.41;  // Distance threshold, this is then ^2
    float default_epsilonIP = 0.5;  // 

    size_t min_candidates = 10;    // Min candidates for epsilon
    size_t max_candidates_cap = 0; // 0 = auto

    // performance tuning
    size_t knn_lookahead_scale = 5;
    int    flush_threshold = 100; // Save index every, -1 only on explicit flush or exit.
    bool   flush_offsets_each = false;


    //
    bool parallel_merge = true;
    unsigned merge_threads = 0; // 0 = auto
    //
    bool normalized_embeddings = false;

    // Dynamic auto-tuning
    bool auto_tune_ef = true;
    bool auto_tune_eps = true;

    // Validation
    bool validate() const;

    // Get epsilon for current metric
    float get_epsilon() const;

    // Get effective max_candidates (with cap applied)
    size_t get_max_candidates(size_t request = 0) const;

    // Get number of merge threads
    unsigned get_merge_threads() const;

    // Print configuration
    void print(std::ostream& os = std::cout) const;

    // Binary serialization
    void save(std::ostream& os) const;

    void load(std::istream& is);

    // Save to file
    bool save_to_file(const std::string& path) const;

    // Load from file
    bool load_from_file(const std::string& path);

    // Merge/override with another config
    void merge_from(const HnswConfig& other);

    // Selective merge (only override non-default values)
    void merge_overrides(const HnswConfig& override, const HnswConfig& defaults);


    // Dynamic setter by string key
    bool set(const std::string& key, const std::string& value);

    // Typed setters for convenience
    bool set(const std::string& key, size_t value) {
        return set(key, std::to_string(value));
    }
    
    bool set(const std::string& key, int value) {
        return set(key, std::to_string(value));
    }
    
    bool set(const std::string& key, float value) {
        return set(key, std::to_string(value));
    }
    
    bool set(const std::string& key, bool value) {
        return set(key, value ? "true" : "false");
    }
    
    bool set(const std::string& key, Metric value) {
        return set(key, metric_to_string(value));
    }
    
    bool set(const std::string& key, SearchModes value) {
        return set(key, search_mode_to_string(value));
    }

    // Dynamic getter by string key
    std::string get(const std::string& key) const;

    // Get all keys
    static std::vector<std::string> get_all_keys() {
        return {
            "max_elements", "M", "ef_construction", "ef_search", "metric",
            "bert_n_threads", "max_tokens_per_chunk", "overlap_percent",
            "debug", "default_k", "default_radius", "default_alpha",
            "default_minN", "default_lookahead", "default_gapDelta",
            "default_epsilon", "default_epsilonL2", "default_epsilonIP",
            "min_candidates", "max_candidates_cap", "knn_lookahead_scale",
            "flush_threshold", "flush_offsets_each", "parallel_merge",
            "merge_threads", "normalized_embeddings", "default_search_mode",
            "auto_tune_ef", "auto_tune_eps"
        };
    }

private:
    static bool parse_bool(const std::string& s) {
        std::string lower = s;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower == "true" || lower == "1" || lower == "yes" || lower == "on") return true;
        if (lower == "false" || lower == "0" || lower == "no" || lower == "off") return false;
        throw std::runtime_error("Invalid boolean value: " + s);
    }

    // String conversions
    static std::string metric_to_string(Metric m) {
        switch (m) {
            case Metric::L2: return "L2";
            case Metric::InnerProduct: return "InnerProduct";
            case Metric::Cosine: return "Cosine";
            default: return "Unknown";
        }
    }

    static std::string search_mode_to_string(SearchModes m) {
        switch (m) {
            case SearchModes::Knn: return "Knn";
            case SearchModes::Radius: return "Radius";
            case SearchModes::Relative: return "Relative";
            case SearchModes::Adaptive: return "Adaptive";
            case SearchModes::Epsilon: return "Epsilon";
            default: return "Unknown";
        }
    }

    static Metric string_to_metric(const std::string& s) {
        if (s == "L2" || s == "l2") return Metric::L2;
        if (s == "InnerProduct" || s == "IP" || s == "ip") return Metric::InnerProduct;
        if (s == "Cosine" || s == "cosine") return Metric::Cosine;
        throw std::runtime_error("Unknown metric: " + s);
    }

    static SearchModes string_to_search_mode(const std::string& s) {
        if (s == "knn" || s == "Knn") return SearchModes::Knn;
        if (s == "radius" || s == "Radius") return SearchModes::Radius;
        if (s == "relative" || s == "Relative") return SearchModes::Relative;
        if (s == "adaptive" || s == "Adaptive") return SearchModes::Adaptive;
        if (s == "epsilon" || s == "Epsilon") return SearchModes::Epsilon;
        throw std::runtime_error("Unknown search mode: " + s);
    }

};


struct IndexMeta {
    uint32_t       version = 1;
    Metric         metric  = Metric::L2; // "L2", "cosine", "ip"
    bool           normalized = false;
    uint32_t       dim = 0;
    uint64_t       count = 0;

    void save(std::ofstream &ofs) const {
        ofs.write(reinterpret_cast<const char*>(this), sizeof(IndexMeta));
    }

    void load(std::ifstream &ifs) {
        ifs.read(reinterpret_cast<char*>(this), sizeof(IndexMeta));
    }
};

