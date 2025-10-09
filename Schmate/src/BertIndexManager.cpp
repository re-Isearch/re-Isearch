
#include "BertIndexManager.hpp"
#include <stdexcept>

BertIndexManager::BertIndexManager(SBertGGML & e, HnswConfig &c)
: embedder(e), cfg(c)
{}

ShardedIndex& BertIndexManager::getOrCreate(const std::string & name) {
    auto it = indexes.find(name);
    if (it != indexes.end()) return *it->second;
    // create new
    indexes[name] = std::make_unique<ShardedIndex>(embedder, cfg, name);
    return *indexes[name];
}

void BertIndexManager::append(const std::string & name, const std::string & sentence) {
    getOrCreate(name).append(sentence);
}

void BertIndexManager::append(const std::string & name, const std::string & sentence, int64_t sentence_id) {
    getOrCreate(name).append(sentence, sentence_id);
}

void BertIndexManager::remove(const std::string & name, size_t label, size_t shard) {
    getOrCreate(name).remove(label, shard);
}


void BertIndexManager::undelete(const std::string & name, size_t label, const OffsetEntry &entry, size_t shard =0) {
    getOrCreate(name).undelete(label, entry, shard);
}

void BertIndexManager::undelete(const std::string &name, size_t label, size_t shard) {
    auto &sharded = getOrCreate(name);

    if (shard >= sharded.shard_count()) {
        throw std::runtime_error("undelete: invalid shard index");
    }

    // retrieve the OffsetEntry for this label
    OffsetEntry e = sharded.get_offset_entry(shard, label);

    // pass it down to BertIndex
    sharded.get_shard(shard).undelete(label, e);
}


/*
void BertIndexManager::delete_byAddress(const std::string & name, int64_t addr, size_t shard) {
    getOrCreate(name).delete_byAddress(addr, shard);
}

void BertIndexManager::undelete_byAddress(const std::string & name, int64_t addr, size_t shard) {
    getOrCreate(name).undelete_byAddress(addr, shard);
}
*/

/*
Can do 

auto results = mgr.knn("default", "AI research", 3);
for (auto &r : results) {
    std::cout << "Chunk: " << mgr.get_text("default", r, false) << "\n";
    std::cout << "Sentence: " << mgr.get_text("default", r, true) << "\n";
}

*/
std::vector<SearchResult> BertIndexManager::knn(const std::string & name, const std::string & query, size_t k) {
std::cerr << "BertIndexManager::knn(" << name << ", " << query << ")\n";
    return getOrCreate(name).knn(query, k);
}

#if 0
std::vector<SearchResult> BertIndexManager::pknn(const std::string & name, const std::string & query, size_t k) {
    return getOrCreate(name).parallel_knn(query, k==0?cfg.default_k:k);
}
#endif 

std::vector<SearchResult> BertIndexManager::radius(const std::string & name, const std::string & query, float minScore) {
    return getOrCreate(name).radius(query, minScore);
}

#if 0
std::vector<SearchResult> BertIndexManager::pradius(const std::string & name, const std::string & query, float minScore) {
    return getOrCreate(name).parallel_radius(query, minScore<0?cfg.default_radius:minScore);
}
#endif

std::vector<SearchResult> BertIndexManager::relative(const std::string & name, const std::string & query, float alpha) {
    return getOrCreate(name).relative(query, alpha);
}

std::vector<SearchResult> BertIndexManager::adaptive(const std::string & name, const std::string & query,
                                                    float alpha, size_t minN, size_t lookahead, float gapDelta) {
    return getOrCreate(name).adaptive(query, alpha, minN, lookahead, gapDelta);
}

void BertIndexManager::merge(const std::string & name) {
    getOrCreate(name).merge_last_two();
}

void BertIndexManager::flush(const std::string & name) {
    getOrCreate(name).flush();
}

size_t BertIndexManager::shard_count(const std::string & name) {
    return getOrCreate(name).shard_count();
}

std::string BertIndexManager::get_text(const std::string &name, const SearchResult &r, bool full_sentence) {
    // delegate to shards: search for non-empty text in shards
    return getOrCreate(name).get_text(r, full_sentence);
}


std::string BertIndexManager::reconstruct_sid(const std::string & name, int64_t sid) {
    return getOrCreate(name).reconstruct_sid(sid);
}

std::string BertIndexManager::reconstruct_label(const std::string & name, size_t label) {
    return getOrCreate(name).reconstruct_label(label);
}

//

void BertIndexManager::delete_byAddress(const std::string &name, int64_t address, size_t shard) {
    auto &sharded = getOrCreate(name);

    if (shard >= sharded.shard_count()) {
        throw std::runtime_error("delete_byAddress: invalid shard index");
    }

    sharded.delete_byAddress(address, shard);
}

void BertIndexManager::undelete_byAddress(const std::string &name, int64_t address, size_t shard) {
    auto &sharded = getOrCreate(name);

    if (shard >= sharded.shard_count()) {
        throw std::runtime_error("undelete_byAddress: invalid shard index");
    }

    sharded.undelete_byAddress(address, shard);
}

