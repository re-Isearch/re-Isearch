
#pragma once
#include "BertIndex.hpp"
#include <map>
#include <memory>
#include <mutex>

class ShardedIndex {
    SBertGGML & embedder;
    HnswConfig & cfg;
    std::string base_name;

    std::vector<std::unique_ptr<BertIndex>> shards;
    mutable std::mutex mtx;

public:
    ShardedIndex(SBertGGML & emb, HnswConfig & c, const std::string & name);

    BertIndex & current_shard();
    BertIndex & get_shard(size_t i);
    size_t shard_count() const;

    void append(const std::string & sentence);
    void append(const std::string & sentence, int64_t sentence_id);

    void remove(size_t label, size_t shard=0);
    void undelete(size_t label, size_t shard=0);

    void delete_byAddress(int64_t address, size_t shard=0);
    void undelete_byAddress(int64_t address, size_t shard=0);

    std::vector<SearchResult> knn(const std::string & query, size_t k=0);
    std::vector<SearchResult> radius(const std::string & query, float minScore=-1.0f);
    std::vector<SearchResult> relative(const std::string & query, float alpha=-1.0f);
    std::vector<SearchResult> adaptive(const std::string & query,
                                       float alpha=-1.0f, size_t minN=0,
                                       size_t lookahead=0, float gapDelta=-1.0f);

    std::string reconstruct_sid(int64_t sid);

    std::string reconstruct_label(size_t label);

    std::string get_text(const SearchResult &r, bool full_sentence=false) const;

    std::string shard_basename(int shard = -1) const;

    void flush();
    bool merge_last_two();
};


