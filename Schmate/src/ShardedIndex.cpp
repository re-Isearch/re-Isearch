
#include "ShardedIndex.hpp"
#include <thread>
#include <future>
#include <algorithm>
using namespace std;


// FORMAT   name_NN  where NN is the number of the shard

#define SHARD0 0

// TODO: Check need to check for current shard!!!!

std::string ShardedIndex::shard_basename(int shard) const
{
  if (shard <= 0) {
#if SHARD0
    return base_name + "_s0";
#else
    return base_name;
#endif
  }
  return base_name + "_s" + to_string(shard);
}


ShardedIndex::ShardedIndex(SBertGGML & emb, HnswConfig & c, const string & name)
: embedder(emb), cfg(c), base_name(name) {
    // start with one shard
    shards.push_back(make_unique<BertIndex>(embedder, cfg, shard_basename(0)));
}

BertIndex & ShardedIndex::current_shard() {
    lock_guard<mutex> lock(mtx);
    if (shards.empty()) throw runtime_error("No shards");
    auto &sh = shards.back();
    if (sh->size() >= cfg.max_elements) {
        string newname = shard_basename(shards.size());
        shards.push_back(make_unique<BertIndex>(embedder, cfg,newname));
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
#if 1

#include "ShardedIndex.hpp"
#ifdef USE_THREADPOOL
#include "ThreadPool.hpp"
static ThreadPool pool;  // global cached pool
#endif

// --- smarter parallel_search with optional (compile-time) thread-pool ---
template <typename Fn>
std::vector<SearchResult> parallel_search(std::vector<std::unique_ptr<BertIndex>> &shards, Fn fn) {
    if (shards.empty()) {
        return {};
    }
    if (shards.size() == 1) {
        return fn(*shards[0]);
    }

    std::vector<std::future<std::vector<SearchResult>>> futs;
    futs.reserve(shards.size());

#ifdef USE_THREADPOOL
    for (auto &sh : shards) {
        futs.push_back(pool.enqueue([&]() {
            return fn(*sh);
        }));
    }
#else
    for (auto &sh : shards) {
        futs.push_back(std::async(std::launch::async, [&]() {
            return fn(*sh);
        }));
    }
#endif

    std::vector<SearchResult> all;
    for (auto &f : futs) {
        auto part = f.get();
        all.insert(all.end(), part.begin(), part.end());
    }

    std::sort(all.begin(), all.end(), [](auto &a, auto &b){ return a.score > b.score; });
    return all;
}


#elif 1

// --- smarter parallel_search ---
template <typename Fn>
std::vector<SearchResult> parallel_search(std::vector<std::unique_ptr<BertIndex>> &shards, Fn fn) {
    if (shards.empty()) {
        return {};
    }

    if (shards.size() == 1) {
        // Fast path: no threading, no merge
        return fn(*shards[0]);
    }

    std::vector<std::future<std::vector<SearchResult>>> futs;
    futs.reserve(shards.size());

    for (auto &sh : shards) {
        futs.push_back(std::async(std::launch::async, [&]() {
            return fn(*sh);
        }));
    }

    std::vector<SearchResult> all;
    for (auto &f : futs) {
        auto part = f.get();
        all.insert(all.end(), part.begin(), part.end());
    }

    // Sort descending by score
    std::sort(all.begin(), all.end(),
              [](auto &a, auto &b) { return a.score > b.score; });

    return all;
}


#else
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
#endif

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

string ShardedIndex::reconstruct_label(size_t label) {
    for (auto &sh : shards) { 
        string s = sh->reconstruct_label(label);
        if (!s.empty()) return s;
    }   
    return "";
}


void ShardedIndex::flush() {
    for (auto &sh : shards) sh->flush();
}


#if 1

bool ShardedIndex::merge_last_two() {
    if (shards.size() < 2) {
        cerr << "Not enough shards to merge\n";
        return false;
    }

    size_t a_idx = shards.size() - 2;
    size_t b_idx = shards.size() - 1;
    auto &A = *shards[a_idx];
    auto &B = *shards[b_idx];

    size_t total_labels = A.label_count() + B.label_count();
    string merged_name = shard_basename (a_idx) + "_merged";

    HnswConfig newcfg = cfg;
    newcfg.max_elements = max(newcfg.max_elements, total_labels + 100);
    auto merged = std::make_unique<BertIndex>(embedder, newcfg, merged_name);

    // Copy A
    for (size_t label = 0; label < A.label_count(); ++label) {
        string text = A.get_text_by_label(label);
        int64_t sid = A.get_sentence_id(label);

        if (text.empty()) {
            // preserve sentence_id but mark deleted
            size_t newlabel = merged->append("", sid);
            merged->remove(newlabel);
        } else {
            merged->append(text, sid);
        }
    }

    // Copy B
    for (size_t label = 0; label < B.label_count(); ++label) {
        string text = B.get_text_by_label(label);
        int64_t sid = B.get_sentence_id(label);

        if (text.empty()) {
            size_t newlabel = merged->append("", sid);
            merged->remove(newlabel);
        } else {
            merged->append(text, sid);
        }
    }

    merged->flush();

    // Replace A with merged, drop B
    shards[a_idx] = std::move(merged);
    shards.pop_back();

    if (cfg.debug) cerr << "[INFO] Merged shards " << a_idx << " + " << b_idx << " -> " << merged_name
         << " (labels ~ " << total_labels << ")\n";
    return true;
}



#else

void ShardedIndex::merge_last_two() {
    if (shards.size() < 2) {
        cerr << "Not enough shards to merge\n";
        return;
    }

    size_t a_idx = shards.size() - 2;
    size_t b_idx = shards.size() - 1;
    auto &A = *shards[a_idx];
    auto &B = *shards[b_idx];

    size_t total = A.size() + B.size();
    string merged_name = base_name + "_merged" + to_string(a_idx);

    HnswConfig newcfg = cfg;
    newcfg.max_elements = total + 100; // buffer
    auto merged = std::make_unique<BertIndex>(embedder, newcfg, merged_name);

    // --- Step 1: copy from A ---
    for (size_t label = 0; label < A.size(); ++label) {
        string text = A.get_text_by_label(label);
        int64_t sid = A.get_sentence_id(label);

        if (text.empty()) {
            // placeholder for deleted chunk
            size_t new_label = merged->size();
            merged->append("", sid);  // keep same sentence_id
            merged->remove(new_label);
        } else {
            merged->append(text, sid);
        }
    }

    // --- Step 2: copy from B ---
    for (size_t label = 0; label < B.size(); ++label) {
        string text = B.get_text_by_label(label);
        int64_t sid = B.get_sentence_id(label);

        if (text.empty()) {
            size_t new_label = merged->size();
            merged->append("", sid);
            merged->remove(new_label);
        } else {
            merged->append(text, sid);
        }
    }

    merged->flush();

    // Replace shards
    shards[a_idx] = std::move(merged);
    shards.pop_back();

    if (cfg.debug) cerr << "[INFO] Compacted shards " << a_idx << " and " << b_idx
         << " into " << merged_name << " (" << total << " items)\n";
}
#endif

std::string ShardedIndex::get_text(const SearchResult &r, bool full_sentence) const {
    // naive: just try each shard until we find a matching label/sentence
    for (auto &sh : shards) {
        std::string txt = sh->get_text(r, full_sentence);
        if (!txt.empty()) return txt;
    }
    return "";
}




