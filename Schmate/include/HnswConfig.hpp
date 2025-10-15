#pragma once
#include <cstddef>
#include <string>

enum class Metric {
    L2,
    InnerProduct,
    Cosine
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
    size_t ef_search = 50;
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

