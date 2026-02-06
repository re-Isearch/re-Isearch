
#pragma once
#include "BertIndex.hpp"
#include "EfSearchTuner.hpp"
#include <map>
#include <memory>
#include <mutex>

class ShardedIndex {
    SBertGGML & embedder;
    hnswlib::HnswConfig & cfg;
    std::string base_name;

    std::vector<std::unique_ptr<BertIndex>> shards;
    std::vector<std::unique_ptr<EfSearchTuner>> shard_tuners;

    mutable std::mutex mtx;

public:
    ShardedIndex(SBertGGML & emb, hnswlib::HnswConfig & c, const std::string & name, bool searchOnly = false);

    BertIndex & current_shard();
    BertIndex & get_shard(size_t i);
    size_t shard_count() const;

    void clear();

    void append(const std::string & sentence);
    void append(const std::string & sentence, int64_t sentence_id);

    void remove(size_t label, size_t shard=0);
    void undelete(size_t label, const OffsetEntry &entry, size_t shard=0);

    void delete_byAddress(int64_t address, size_t shard=0);
    void undelete_byAddress(int64_t address, size_t shard=0);

    std::vector<SearchResult> search(const std::string & query); // Use config

    std::vector<SearchResult> knn(const std::string & query, size_t k=0);
    std::vector<SearchResult> radius(const std::string & query, float minScore=-1.0f);
    std::vector<SearchResult> relative(const std::string & query, float alpha=-1.0f);
    std::vector<SearchResult> adaptive(const std::string & query,
                                       float alpha=-1.0f, size_t minN=0,
                                       size_t lookahead=0, float gapDelta=-1.0f);
    std::vector<SearchResult> epsilon_search(const std::string & query, float epsilon=-1.0f);

    std::string reconstruct_sid(int64_t sid);

    std::string reconstruct_label(size_t label);

    std::string get_text(const SearchResult &r, bool full_sentence=false) const;

    std::string shard_basename(int shard = -1) const;

    void flush();

    bool merge(); // Merge all the way
    bool merge_last_two();

    OffsetEntry get_offset_entry(size_t shard, size_t label);
private:

template <typename Fn>
    std::vector<SearchResult> parallel_search(std::vector<std::unique_ptr<BertIndex>> &shards, Fn fn);

    // merge shard n and shard n-1 into a new shard n-1 and remove n
    // When this works the number of shards gets decremented.
    bool merge_two_parallel(size_t n);
    bool merge_two_serial(size_t n);

    void add_shard(size_t i, bool searchOnly = false);
    size_t discover_shards(const std::string &base_name) const;
};


