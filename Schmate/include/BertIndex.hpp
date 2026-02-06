#pragma once

#define HNSWLIB_ERR_OVERRIDE LOG_ERROR_S()

#include "SBertGGML.hpp"
#include "HnswConfig.hpp"
#include "Util.hpp"
#include "unified_hnsw.hpp"
#include "FileLock.hpp"
#include "OffsetFile.hpp"
#include "AdaptiveSearchController.hpp"

#include <unordered_map>
#include <string>
#include <fstream>
#include <iostream>
#include <atomic>

struct SearchResult {
    float score;
    int64_t file_start;
    int64_t file_end;
    size_t label;
    int token_start;
    int token_end;
    int64_t sentence_id;
    std::string text; // optional
};

struct Chunk {
    std::string text;
    size_t start_token;
    size_t end_token;
};

/* To use instead Llama.cpp instead of bert.cpp

// Previously:
SBertGGML embedder("sbert.ggml");

// Now:
LlamaEmbedder embedder("llama-2-7b.Q4_K_M.gguf");

*/

class BertIndex {
friend class ShardedIndex;
    SBertGGML & embedder;
    hnswlib::HnswConfig & cfg;

    hnswlib::Metric metric = hnswlib::Metric::L2;

    std::unique_ptr<hnswlib::UnifiedIndex> index;

    std::string name;
    std::string sentences_path;
    std::string offsets_path;
    std::string index_path;

    size_t next_label = 0;
    std::queue<size_t> free_labels;    // pool of reusable labels

    std::unordered_map<size_t, std::pair<int,int>> chunk_token_map;
    std::unordered_map<size_t,int64_t> chunk_sentence_map;
    std::atomic<int64_t> auto_sentence_id{0};

    std::vector<Chunk> chunk_tokens(const std::string &sentence);

    size_t dirty_count = 0;

public:
    BertIndex(SBertGGML & emb, hnswlib::HnswConfig & c, const std::string & n, bool searchOnly = false);
    ~BertIndex();

    const std::string model_name() { return embedder.model_name; }

    size_t get_data_size() { return index ? index->get_data_size() : 0;}

#if 1

    // append returns the label assigned to the new chunk
    size_t append(const std::string & sentence);
    size_t append(const std::string & sentence, int64_t sentence_id);

    // removal
    void remove(size_t label);
    void undelete(size_t label, const OffsetEntry &entry);

    void delete_byAddress(const std::string &name, int64_t address, size_t shard = 0);
    void undelete_byAddress(const std::string &name, int64_t address, size_t shard = 0);

    // accessors used by merge/reconstruct
    size_t label_count() const;               // how many labels were allocated (next_label)

#else
    void append(const std::string & sentence);
    void append(const std::string & sentence, int64_t sentence_id);

    void remove(size_t label);
    void undelete(size_t label);
#endif

    size_t allocate_label();

    std::vector<SearchResult> search(const std::string & query); // Use config

    std::vector<SearchResult> knn(const std::string & query, size_t k=0);
    std::vector<SearchResult> radius(const std::string & query, float minScore=-1.0f);
    std::vector<SearchResult> relative(const std::string & query, float alpha=-1.0f, size_t maxK= 0);
    std::vector<SearchResult> adaptive(const std::string & query, float alpha=-1.0f,
                                       size_t minN=0, size_t lookahead=0, float gapDelta=-1.0f);
    std::vector<SearchResult> epsilon_search(const std::string &query, float epsilon=-1.0f);


    std::string reconstruct_sentence(int64_t sentence_id) const;

    void clear(); // Zap the contents and reset everything

    void flush();
    void save();
//  void load();

    size_t size() const { return index ? (size_t) index->size() : 0; }

    std::string get_text_by_label(size_t label) const;
    std::string reconstruct_label(size_t label) const { return get_text_by_label(label);}

    int64_t get_sentence_id(size_t label) const;

    // Get text for a search result (chunk or sentence-level)
    // If full_sentence==true, always reconstruct the sentence.
    // Otherwise return chunk text (unless sentence_id is set).
    std::string get_text(const SearchResult &r, bool full_sentence=false) const;

    // Auto-tuning
//    EfSearchTuner tuner;
//    EpsilonTuner eps_tuner;
    AdaptiveSearchController search_ctrl;


private:
   template<typename FilterFn>
   std::vector<SearchResult> filter_knn_results(const std::string &query,
        size_t max_k, FilterFn filter);
    inline bool is_valid_entry(const OffsetEntry &e) const {
      return !(e.sid == 0 || e.file_end <= e.file_start);
    }

   mutable std::unique_ptr<FileLock> file_lock;
   bool acquire_lock() const;
   void release_lock() const;
   int  wait_lock() const;
   bool remove_lockfile() const;

   std::vector<float> encode_text(const std::string& text);

   // convert raw hnsw distance to a score 
   float score_from_dist(float dist) const;

//    bool write_offsets(size_t, int64_t, size_t, size_t, int64_t, int64_t) ;
//    bool load_offsets();
    std::unique_ptr<OffsetFile> offsets;

    std::fstream sentences_file;
};


