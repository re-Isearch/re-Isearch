#pragma once

#include "SBertGGML.hpp"
#include "HnswConfig.hpp"
#include "ShardedIndex.hpp"

#include <string>
#include <memory>
#include <unordered_map>

#define USE_LRUCACHE 1

#if USE_LRUCACHE
#include "LRUCache.hpp"
#endif

/* To use instead Llama.cpp instead of bert.cpp

// Previously:
SBertGGML embedder("sbert.ggml");

// Now:
LlamaEmbedder embedder("llama-2-7b.Q4_K_M.gguf");

or Using new bert.cpp API
SBertGGML embedder("sbert.gguf");



| Topic             | Recommendation 
| ----------------- | --------------------------------------------------------------------------------------- |
| **Model Choice**  | Use a **LLaMA model trained or fine-tuned for embeddings** (e.g., `mxbai-embed-large`,  |
|                   | `nomic-embed-text-v1.5.gguf`) rather than a chat model.                                 |
| **Performance**   | LLaMA embeddings are slower than SBERT in `bert.cpp`, but higher quality.               |
|                   | Use `ctx_params.n_threads` for tuning.                                                  |
| **Normalization** | If you use cosine similarity in HNSW, normalize the embedding after retrieval.          |
| **Memory**        | LLaMA models require more RAM — ensure `--n-gpu-layers` is 0 if running CPU-only.       |


*/


class BertIndexManager {
    SBertGGML           &embedder;
    hnswlib::HnswConfig &cfg;
    bool                searchOnly;
    std::string         base_dir;
    void               *opaque_ptr;
#if USE_LRUCACHE
    LRUCache<std::string, ShardedIndex> index_cache;
#else
    std::unordered_map<std::string, std::unique_ptr<ShardedIndex>> indexes;
#endif
public:
#if USE_LRUCACHE
    BertIndexManager(SBertGGML & e, hnswlib::HnswConfig &c, size_t max_cached=3,
	bool searchOnly = false, void *ptr = nullptr);
#else
    BertIndexManager(SBertGGML & e, hnswlib::HnswConfig &c,
	bool searchOnly = false, void *ptr = nullptr);
#endif

    void set_base_dir(const std::string& new_dir) { base_dir = new_dir; }
    void set_opaque_ptr(void *ptr) { opaque_ptr = ptr; }

    void clear(const std::string &name);

    ShardedIndex *get(const std::string &name, size_t identity = 0);
    // get or create a named index
    ShardedIndex & getOrCreate(const std::string & name, size_t identity = 0);

    // shorthands
    void append(const std::string &name, const std::string_view sentence);
    void append(const std::string &name, const std::string_view sentence, int64_t sentence_id, uint32_t span = 0);

    std::vector<size_t> find_labels_by_sid(const std::string& name, int64_t sid, size_t shard = 0);

    void remove(const std::string & name, size_t label);
    void remove(const std::string & name, size_t label, size_t shard);

    size_t removeDeletedElements(const std::string& name, std::function<bool(size_t)> isDeleted, size_t shard = 0);

    void undelete(const std::string &name, size_t label, size_t shard = 0);
    void undelete(const std::string & name, size_t label, const OffsetEntry &entry, size_t shard);

    int64_t get_sentence_id(const std::string &name, size_t label, size_t shard = 0);

    void delete_byAddress(const std::string & name, int64_t addr, size_t shard = 0);
    void undelete_byAddress(const std::string & name, int64_t addr, size_t shard = 0);

    std::vector<SearchResult> search(const std::string & name, const std::string & query); // Use config

    std::vector<SearchResult> knn(const std::string & name, const std::string & query, size_t k = 0);
    std::vector<SearchResult> radius(const std::string & name, const std::string & query, float minScore = -1.0f);
    std::vector<SearchResult> relative(const std::string & name, const std::string & query, float alpha = -1.0f);
    std::vector<SearchResult> adaptive(const std::string & name, const std::string & query,
                                       float alpha = -1.0f, size_t minN = 0, size_t lookahead = 0, float gapDelta = -1.0f);
    std::vector<SearchResult> epsilon_search(const std::string &name, const std::string &query, float epsilon = -1.0f);

#if 0 /* Parallel is now defined by a configuration */
    std::vector<SearchResult> pknn(const std::string & name, const std::string & query, size_t k = 0);
    std::vector<SearchResult> pradius(const std::string & name, const std::string & query, float minScore = -1.0f);
#endif

    void merge(const std::string & name);
    void flush(const std::string & name);
#if USE_LRUCACHE
    void flush_all() ; // Flushes the index cache
#endif
    size_t shard_count(const std::string & name);

    std::string get_text(const std::string & name, const SearchResult & r, bool full_sentence=false);
    std::string reconstruct_sid(const std::string & name, int64_t sid);
    std::string reconstruct_label(const std::string & name, size_t label);
};

