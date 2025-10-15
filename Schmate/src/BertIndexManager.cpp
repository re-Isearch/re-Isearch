
#include "BertIndexManager.hpp"
#include <stdexcept>



/* re-Isearch will enter here

  EmbeddingIndexer->append ( DocTypePtr->ParseBuffer(Buffer, FieldName, fc.Start(), fc.End(),type))

NOTE: We write the chunks added to the index to a sentences file. This is for development/debuging.
When we move to full integration into re-Isearch we'll dump the sentences file BUT maintain the
start and end offsets. fc.Start() we will use for the SID. The offset file start will be 0 and end
will be fc.End() - fc.Start() to represent the length;

*/

#if USE_LRUCACHE

/*
| Feature             | Description                                                                                              |
| ------------------- | -------------------------------------------------------------------------------------------------------- |
| **Thread Safety**   | Uses a single mutex. For high contention, could later use `std::shared_mutex` with fine-grained locking. |
| **Eviction Policy** | Least Recently Used (oldest not accessed).                                                               |
| **Eviction Action** | Calls `on_evict` callback to flush and release memory.                                                   |
| **Value Type**      | Uses `shared_ptr<ShardedIndex>` to allow easy sharing across functions without double-loading.           |
| **Performance**     | O(1) for both get and put operations.                                                                    |
| **Extensible**      | Could later add stats (hits, misses, evictions).                                                         |

*/

BertIndexManager::BertIndexManager(SBertGGML & e, HnswConfig & c, size_t max_cached, bool s) : embedder(e), cfg(c),
      searchOnly (s), index_cache(max_cached, [](const std::string &key, std::shared_ptr<ShardedIndex> idx) {
          if (idx) {
              LOG_INFO_S() << "Evicting index: " << key;
              idx->flush();  // flush all shards before eviction
          }
      })
{}


ShardedIndex & BertIndexManager::getOrCreate(const std::string &name)
{
    auto idx = index_cache.get(name);
    if (!idx) {
        auto new_index = std::make_shared<ShardedIndex>(embedder, cfg, name, searchOnly);
        index_cache.put(name, new_index);
        return *new_index;
    }
    return *idx;
}


#else /* NOT LRU_CACHE */
BertIndexManager::BertIndexManager(SBertGGML & e, HnswConfig &c) : embedder(e), cfg(c)
{}

ShardedIndex& BertIndexManager::getOrCreate(const std::string & name) {
    auto it = indexes.find(name);
    if (it != indexes.end()) return *it->second;
    // create new
    indexes[name] = std::make_unique<ShardedIndex>(embedder, cfg, name, searchOnly);
    return *indexes[name];
}
#endif // LRU_CACHE

void BertIndexManager::append(const std::string & name, const std::string & sentence) {
    if (searchOnly) LOG_ERROR_S() << "BertIndexManager::append(" << name << ") called with searchOnly true (1)";
    getOrCreate(name).append(sentence);
}

void BertIndexManager::append(const std::string & name, const std::string & sentence, int64_t sentence_id) {
    if (searchOnly) LOG_ERROR_S() << "BertIndexManager::append(" << name << ") called with searchOnly true (2)";
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

std::vector<SearchResult> BertIndexManager::search(const std::string & name, const std::string & query) {
    return getOrCreate(name).search(query);
}

std::vector<SearchResult> BertIndexManager::knn(const std::string & name, const std::string & query, size_t k) {
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

std::vector<SearchResult> BertIndexManager::epsilon_search(const std::string &name, const std::string &query, float epsilon) {
    return getOrCreate(name).epsilon_search(query, epsilon);
}


void BertIndexManager::merge(const std::string & name) {
    getOrCreate(name).merge_last_two();
}

void BertIndexManager::flush(const std::string & name) {
    getOrCreate(name).flush();
}

#if USE_LRUCACHE
void BertIndexManager::flush_all() {
    index_cache.for_each([](auto &name, auto &idx) {
        idx->flush();
    });
}
#endif


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

