
#include "ShardedIndex.hpp"
#include <thread>
#include <future>
#include <algorithm>
using namespace std;

ShardedIndex::ShardedIndex(SBertGGML & emb, HnswConfig & c, const string & name)
: embedder(emb), cfg(c), base_name(name) {
    // start with one shard
    shards.push_back(make_unique<BertIndex>(embedder, cfg, base_name + "_shard0"));
}

BertIndex & ShardedIndex::current_shard() {
    lock_guard<mutex> lock(mtx);
    if (shards.empty()) throw runtime_error("No shards");
    auto &sh = shards.back();
    if (sh->size() >= cfg.max_elements) {
        string newname = base_name + "_shard" + to_string(shards.size());
        shards.push_back(make_unique<BertIndex>(embedder, cfg, newname));
    }
    return *shards.back();
}

BertIndex & ShardedIndex::get_shard(size_t i) {
    lock_guard<mutex> lock(mtx);
    if (i>=shards.size()) throw runtime_error("Invalid shard");
    return *shards[i];
}

size_t ShardedIndex::shard_count() const {
    return shards.size();
}

void ShardedIndex::append(const string & sentence) {
    current_shard().append(sentence);
}
void ShardedIndex::append(const string & sentence, int64_t sid) {
    current_shard().append(sentence, sid);
}

void ShardedIndex::remove(size_t label, size_t shard) {
    get_shard(shard).remove(label);
}
void ShardedIndex::undelete(size_t label, size_t shard) {
    get_shard(shard).undelete(label);
}

void ShardedIndex::delete_byAddress(int64_t addr, size_t shard) {
    auto &sh = get_shard(shard);
    // brute: scan offsets
    // simplified
    cerr << "[WARN] delete_byAddress not fully implemented\n";
}
void ShardedIndex::undelete_byAddress(int64_t addr, size_t shard) {
    cerr << "[WARN] undelete_byAddress not fully implemented\n";
}

// --- parallel search helper ---
template <typename Fn>
vector<SearchResult> parallel_search(vector<unique_ptr<BertIndex>> &shards, Fn fn) {
    vector<future<vector<SearchResult>>> futs;
    for (auto &sh : shards) {
        futs.push_back(async(launch::async, [&](){ return fn(*sh); }));
    }
    vector<SearchResult> all;
    for (auto &f : futs) {
        auto part = f.get();
        all.insert(all.end(), part.begin(), part.end());
    }
    sort(all.begin(), all.end(), [](auto &a,auto &b){return a.score>b.score;});
    return all;
}

// search variants
vector<SearchResult> ShardedIndex::knn(const string & query, size_t k) {
    return parallel_search(shards, [&](BertIndex &sh){return sh.knn(query,k);});
}
vector<SearchResult> ShardedIndex::radius(const string & query, float minScore) {
    return parallel_search(shards, [&](BertIndex &sh){return sh.radius(query,minScore);});
}
vector<SearchResult> ShardedIndex::relative(const string & query, float alpha) {
    return parallel_search(shards, [&](BertIndex &sh){return sh.relative(query,alpha);});
}
vector<SearchResult> ShardedIndex::adaptive(const string & query,
                                            float alpha, size_t minN,
                                            size_t lookahead, float gapDelta) {
    return parallel_search(shards, [&](BertIndex &sh){return sh.adaptive(query,alpha,minN,lookahead,gapDelta);});
}

string ShardedIndex::reconstruct_sid(int64_t sid) {
    for (auto &sh : shards) {
        string s = sh->reconstruct_sentence(sid);
        if (!s.empty()) return s;
    }
    return "";
}

void ShardedIndex::flush() {
    for (auto &sh : shards) sh->flush();
}

// stub merge
void ShardedIndex::merge_last_two() {
    if (shards.size()<2) { cerr<<"Not enough shards to merge\n"; return; }
    cerr<<"[WARN] merge_last_two not fully implemented\n";
}


