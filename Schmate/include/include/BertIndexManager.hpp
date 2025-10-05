#pragma once

#include "SBertGGML.hpp"
#include "HnswConfig.hpp"
#include "ShardedIndex.hpp"

#include <string>
#include <memory>
#include <unordered_map>

class BertIndexManager {
    SBertGGML & embedder;
    HnswConfig & cfg;
    std::unordered_map<std::string, std::unique_ptr<ShardedIndex>> indexes;

public:
    BertIndexManager(SBertGGML & e, HnswConfig &c);

    // get or create a named index
    ShardedIndex & getOrCreate(const std::string & name);

    // shorthands
    void append(const std::string & name, const std::string & sentence);
    void append(const std::string & name, const std::string & sentence, int64_t sentence_id);

    void remove(const std::string & name, size_t label, size_t shard = 0);
    void undelete(const std::string & name, size_t label, size_t shard = 0);

    void delete_byAddress(const std::string & name, int64_t addr, size_t shard = 0);
    void undelete_byAddress(const std::string & name, int64_t addr, size_t shard = 0);

    std::vector<SearchResult> knn(const std::string & name, const std::string & query, size_t k = 0);
    std::vector<SearchResult> pknn(const std::string & name, const std::string & query, size_t k = 0);
    std::vector<SearchResult> radius(const std::string & name, const std::string & query, float minScore = -1.0f);
    std::vector<SearchResult> pradius(const std::string & name, const std::string & query, float minScore = -1.0f);
    std::vector<SearchResult> relative(const std::string & name, const std::string & query, float alpha = -1.0f);
    std::vector<SearchResult> adaptive(const std::string & name, const std::string & query,
                                       float alpha = -1.0f, size_t minN = 0, size_t lookahead = 0, float gapDelta = -1.0f);

    void merge(const std::string & name);
    void flush(const std::string & name);
    size_t shard_count(const std::string & name);

    std::string get_text(const std::string & name, const SearchResult & r, bool full_sentence=false);
    std::string reconstruct_sid(const std::string & name, int64_t sid);
    std::string reconstruct_label(const std::string & name, size_t label);
};

