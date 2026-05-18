
#include "ShardedIndex.hpp"
#include <thread>
#include <future>
#include <algorithm>

#include "Logger.hpp"
using namespace std;
using namespace hnswlib;


// FORMAT   name_NN  where NN is the number of the shard

#define SHARD0 0

#define SHARD(x) "_s" + to_string(x)

std::string ShardedIndex::shard_basename(int shard) const
{
#if !SHARD0
  if (shard <= 0)
    return base_name;
#endif
  return base_name + SHARD(shard);
}

size_t ShardedIndex::discover_shards(const std::string &base_name) const {
    size_t count = 0;
    while (true) {
	// We look at the HNSW index file and offset file
	if (!file_exists(shard_basename(count) + IndexFileExtensions::hnsw ) ||
		!file_exists(shard_basename(count) + IndexFileExtensions::offsets )) break;
        count++;
    }
    // TODO: Look for simple holes: 1 2 4 5...

    return count;
}

void ShardedIndex::add_shard(size_t id, bool searchOnly) {
    auto shard = make_unique<BertIndex>(embedder, cfg, shard_basename(id), searchOnly, opaque_ptr) ;
    shard->set_storage_path_dir (base_dir);
    shards.emplace_back(std::move(shard));
    // For the shard based auto-tuner
    shard_tuners.emplace_back(std::make_unique<EfSearchTuner>(cfg.ef_search));
}

// Creator
ShardedIndex::ShardedIndex(SBertGGML & emb, HnswConfig & c, const string & name,
	bool searchOnly, void *ptr) : embedder(emb), cfg(c), base_name(name), opaque_ptr(ptr) {
    size_t found = discover_shards(base_name);
    if (found == 0) {
        if (searchOnly) {
	   LOG_ERROR_S() << "No Shards in \"" << name << "\". SearchOnly was set to true!";
	   return;
        }
        // No shards exist yet, create one
        add_shard(0);
        if (cfg.debug) LOG_INFO_S() << "Created initial shard: 0";
    } else {
        if (cfg.debug) LOG_INFO_S() << "Found " << found << " existing shards";
        for (size_t i = 0; i < found; ++i) {
            add_shard(i, searchOnly);
        }
    }
}


void ShardedIndex::clear() {
    for (auto &shard : shards) {
        shard->clear();
    }
    shards.clear();
}


BertIndex & ShardedIndex::current_shard() {
    lock_guard<mutex> lock(mtx);
    if (shards.empty()) throw runtime_error("No shards");
    auto &sh = shards.back();
    if (sh->size() >= cfg.max_elements) {
        string newname = shard_basename(shards.size());
        shards.push_back(make_unique<BertIndex>(embedder, cfg,newname, false, opaque_ptr));
    }
    return *shards.back();
}

BertIndex & ShardedIndex::get_shard(size_t i) const {
    lock_guard<mutex> lock(mtx);
    if (i>=shards.size()) throw runtime_error("Invalid shard");
    return *shards[i];
}

size_t ShardedIndex::shard_count() const {
    return shards.size();
}

void ShardedIndex::append(const string_view sentence) {
    current_shard().append(sentence);
}
void ShardedIndex::append(const string_view sentence, int64_t sid, uint32_t span) {
    current_shard().append(sentence, sid, span);
}

void ShardedIndex::remove(size_t label) {
   for (auto &sh : shards) {
     sh->remove(label);
   }
}


std::vector<size_t> ShardedIndex::find_labels_by_sid(int64_t sid, size_t shard) const {
  return get_shard(shard).find_labels_by_sid(sid);
}


void ShardedIndex::remove(size_t label, size_t shard) {
    get_shard(shard).remove(label);
}
void ShardedIndex::undelete(size_t label, const OffsetEntry &entry, size_t shard) {
    get_shard(shard).undelete(label, entry);
}

size_t ShardedIndex::removeDeletedElements(std::function<bool(size_t)>isDeleted, size_t shard)
{
    return get_shard(shard).removeDeletedElements(isDeleted);
}

/*
void ShardedIndex::delete_byAddress(int64_t addr, size_t shard) {
    auto &sh = get_shard(shard);
    // brute: scan offsets
    // simplified
    cerr << "[WARN] delete_byAddress not fully implemented\n";
}



void ShardedIndex::undelete_byAddress(int64_t addr, size_t shard) {
    cerr << "[WARN] undelete_byAddress not fully implemented\n";
}
*/

// --- parallel search helper ---
#include "ShardedIndex.hpp"
#ifdef USE_THREADPOOL
#include "ThreadPool.hpp"
static ThreadPool pool;  // global cached pool
#endif

// --- smarter parallel_search with optional (compile-time) thread-pool ---
template <typename Fn>
std::vector<SearchResult> ShardedIndex::parallel_search(std::vector<std::unique_ptr<BertIndex>> &shards, Fn fn) {
    if (shards.empty()) {
        return {};
    }

    std::vector<std::future<std::vector<SearchResult>>> futs;
    futs.reserve(shards.size());

    for (size_t i = 0; i < shards.size(); ++i) {
       auto shard_ptr = shards[i].get();
       auto tuner_ptr = shard_tuners[i].get();
#ifdef USE_THREADPOOL
       futs.push_back(pool.enqueue([=, &fn]()
#else
       futs.push_back(std::async(std::launch::async, [=, &fn]()
#endif
       {
          shard_ptr->index->setEf(tuner_ptr->current_ef);
          auto start = std::chrono::high_resolution_clock::now();
          auto results = fn(*shard_ptr);
          auto end = std::chrono::high_resolution_clock::now();

          // Tag each result with its shard number
          for (auto& r : results) r.shard = i;

          double elapsed = std::chrono::duration<double, std::milli>(end - start).count();
          tuner_ptr->update_after_query(elapsed, shard_ptr->size(), shard_ptr->cfg.debug);
          return results;
         }));
    }

    std::vector<SearchResult> all;
    for (auto &f : futs) {
        auto part = f.get();
        all.insert(all.end(), part.begin(), part.end());
    }

    std::sort(all.begin(), all.end(), [](auto &a, auto &b){ return a.score > b.score; });
    return all;
}



// search variants

vector<SearchResult> ShardedIndex::search(const string & query) {
    return parallel_search(shards, [&](BertIndex &sh){return sh.search(query);});
}
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
vector<SearchResult> ShardedIndex::epsilon_search(const string & query, float epsilon) {
    return parallel_search(shards, [&](BertIndex &sh){return sh.epsilon_search(query, epsilon);});
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


#if 1

// TODO:
// We need to remove shard B and replace shard A with merged
// when finished.
// Probably need to lock while renaming.
//

#include <future>
#include <thread>

static void remove_safe(const std::string &fname) {
    using namespace std::filesystem;
    try {
        if (exists(fname)) remove(fname);
    } catch (const std::exception &e) {
         LOG_WARN_S() << "remove failed for " << fname << ": " << e.what();
    }
}

static void remove_safe_indexes(const std::string &shard_prefix) {
  remove_safe(shard_prefix + IndexFileExtensions::sentences);
  remove_safe(shard_prefix + IndexFileExtensions::offsets);
  remove_safe(shard_prefix + IndexFileExtensions::hnsw);
}

bool ShardedIndex::Exists(const string &name)
{
  return file_exists (name + IndexFileExtensions::hnsw);
}


#include <future>
#include <thread>

bool ShardedIndex::merge()
{
   if (shards.size() < 2) return false;
    while (shards.size() > 2) {
      if (merge_last_two()) return false;
    }
   return true;
}

bool ShardedIndex::merge_last_two()
{
    const size_t n = shards.size();
    if (n < 2) {
        LOG_WARN_S() << "merge_last_two(): need at least 2 shards to merge.";
        return false;
    }
    size_t available_shards = discover_shards(base_name);
    if (n < available_shards) {
      LOG_ERROR_S() << "Another process already merged (" 
	<< available_shards << "<" << n << ") \"" << base_name << "\"";
      return false;
    } else if (n != available_shards) {
      LOG_ERROR_S() << "Another process is appending to new shards on \""
	<< base_name << "\". Merge not available now.";
      return false;
    }

    if (cfg.parallel_merge) return merge_two_parallel(n);
    return merge_two_serial(n);
}

bool  ShardedIndex::merge_two_parallel(size_t n) {
    if (n < 2) return false;

    // O(NlogN) because we are re-inserting into a new HNSW. 

    size_t first = n - 2;
    size_t second = n - 1;
    auto &A = *shards[first];
    auto &B = *shards[second];

#if 0 /* Don't probably need this check. We are now re-generating the embedding */
    if (A.get_data_size() != B.get_data_size()) {
      LOG_WARN_S() << "[merge] Can't merge " << first << " and " << second << " shards as they have different sizes ("
	<< A.get_data_size() << "!=" << B.get_data_size() << "!";
      return false;
    }

    if (A.metric != B.metric) {
      LOG_WARN_S() << "[merge] Can't merge shards made with different metrics ("
	<< metric_to_string(A.metric)
	<< "!="
	<< metric_to_string(B.metric) << ")";
      return false;
    }
#endif

    if (cfg.debug) LOG_INFO_S()  << "Parallel merging shards " << first << " + " << second << "...";

    size_t sizeA = A.size();
    size_t sizeB = B.size();

    HnswConfig merged_cfg = cfg;
    merged_cfg.max_elements = sizeA + sizeB + 1024; // safety margin

    std::string merged_name = base_name + IndexFileExtensions::merge;
    if (file_exists(merged_name)) {
      LOG_ERROR_S() << "Another process is already doing a merge: " << base_name;
      return false;
    }
    auto merged = std::make_unique<BertIndex>(embedder, merged_cfg, merged_name, false, opaque_ptr);

    // Collect entries first to avoid holding locks
    std::vector<std::pair<std::string, int64_t>> items;
    items.reserve(sizeA + sizeB);

    auto collect = [&](BertIndex &idx) {
        idx.offsets->for_each([&](size_t lbl, const OffsetEntry &e) {
            if (e.file_end <= e.file_start) return;
            std::string text = idx.get_text_by_label(lbl);
            items.emplace_back(std::move(text), e.sid);
        });
    };

    // Collect from both shards in parallel
    auto futA = std::async(std::launch::async, [&]() { collect(A); });
    auto futB = std::async(std::launch::async, [&]() { collect(B); });
    futA.get();
    futB.get();

    if (cfg.debug) LOG_INFO_S() << "Collected " << items.size() << " entries. Building merged shard...";

    // Determine how many threads we can use for appending
    unsigned nthreads = cfg.merge_threads ? cfg.merge_threads
                                      : std::max(2u, std::thread::hardware_concurrency());

    std::atomic<size_t> idx_next{0};

    auto worker = [&]() {
        while (true) {
            size_t i = idx_next.fetch_add(1);
            if (i >= items.size()) break;
            auto &[text, sid] = items[i];
            merged->append(text, sid);
        }
    };

    std::vector<std::thread> threads;
    for (unsigned t = 0; t < nthreads; ++t)
        threads.emplace_back(worker);
    for (auto &t : threads)
        t.join();

    merged->flush();

    if (cfg.debug) LOG_INFO_S()<< "Parallel merge complete. Wrote " << items.size()
              << " items into merged shard.";

    // Replace old shard files
    std::string merged_final_name = shard_basename (first);
    merged->index->saveIndex(merged_final_name);

    shards[first] = std::move(merged);

    // Remove old files
    remove_safe_indexes ( shard_basename(second) );

    shards.pop_back();

    if (cfg.debug) LOG_INFO_S() << "Merge done. Total shards now: " << shards.size();
    return true;
}


bool ShardedIndex::merge_two_serial(size_t n) {
    if (n < 2) return false;

    // Identify the shards
    size_t first = n - 2;
    size_t second = n - 1;
    auto &A = *shards[first];
    auto &B = *shards[second];

    if (cfg.debug) LOG_INFO_S() << "Merging shards " << first << " + " << second << "...";

    // Get current sizes
    size_t sizeA = A.size();
    size_t sizeB = B.size();

    // Create a new merged index
    HnswConfig merged_cfg = cfg;
    merged_cfg.max_elements = sizeA + sizeB + 1024; // some headroom

    std::string merged_name = base_name + IndexFileExtensions::merge;
    if (file_exists(merged_name)) {
      LOG_ERROR_S() << "Another process is already doing a merge: " << base_name;
      return false;
    }

    auto merged = std::make_unique<BertIndex>(embedder, merged_cfg, merged_name, false, opaque_ptr);

    // Merge all sentences and offsets from both
    size_t label = 0;
    A.offsets->for_each([&](size_t lbl, const OffsetEntry &e) {
        if (e.file_end <= e.file_start) return; // skip deleted
        std::string text = A.get_text_by_label(lbl);
        merged->append(text, e.sid); // If we had a race the append has it's own lock 
    });
    B.offsets->for_each([&](size_t lbl, const OffsetEntry &e) {
        if (e.file_end <= e.file_start) return; // skip deleted
        std::string text = B.get_text_by_label(lbl);
        merged->append(text, e.sid);
    });

    merged->flush();

    if (cfg.debug) LOG_INFO_S() << "Merged " << sizeA + sizeB << " items into new shard.";

    // Persist the merged shard to its permanent location
    std::string merged_final_name = shard_basename (first);
    merged->index->saveIndex(merged_final_name);

    // Replace the old first shard with the merged one
    shards[first] = std::move(merged);

    // Delete old second shard files

    remove_safe_indexes ( shard_basename(second) );

    // Drop the pointer from the vector
    shards.pop_back();

    // Recompute shard count
    if (cfg.debug) LOG_INFO_S() << "Shard merge complete. Now have " << shards.size() << " shards.";
    return true;
}


/*

bool ShardedIndex::merge_last_two() {
    if (shards.size() < 2) {
        LOG_WARN_S() << "Not enough shards to merge";
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

    if (cfg.debug)  LOG_INFO_S() << "Merged shards " << a_idx << " + " << b_idx << " -> " << merged_name
         << " (labels ~ " << total_labels << ")";
    return true;
}

*/

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

    if (cfg.debug)  LOG_INFO_S() << "Compacted shards " << a_idx << " and " << b_idx
         << " into " << merged_name << " (" << total << " items)";
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


OffsetEntry ShardedIndex::get_offset_entry(size_t shard, size_t label) {
    return shards[shard]->offsets->get(label);
}


void ShardedIndex::delete_byAddress(int64_t address, size_t shard) {
    if (shard >= shards.size()) {
        throw std::runtime_error("delete_byAddress: invalid shard index");
    }

    auto &index = *shards[shard];
    index.offsets->for_each([&](size_t label, const OffsetEntry &e) {
        if (address >= e.file_start && address < e.file_end) {
            index.remove(label);
        }
    });
}

void ShardedIndex::undelete_byAddress(int64_t address, size_t shard) {
    if (shard >= shards.size()) {
        throw std::runtime_error("undelete_byAddress: invalid shard index");
    }

    auto &index = *shards[shard];
    index.offsets->for_each([&](size_t label, const OffsetEntry &e) {
        if (address >= e.file_start && address < e.file_end) {
            index.undelete(label, e);
        }
    });
}

void ShardedIndex::flush() {
//     std::lock_guard<std::mutex> lock(mu);
    for (auto &sh : shards) sh->flush();
    if (cfg.debug)
        LOG_DEBUG_S() << "Flushed all shards for index '" << base_name << "'";
}


int64_t ShardedIndex::get_sentence_id(size_t label, size_t shard) {
   if (shard >= shards.size()) {
        throw std::runtime_error("get_sentence_id: invalid shard index");
    }
   return shards[shard]->get_sentence_id(label);
}


// Brute-force remove garbage
bool ShardedIndex::unlink(const std::string &path) {
  int  result = BertIndex::unlink(path);
  for (size_t i = 0; i < 50; i++) {
     std::string s = path + SHARD(i);
     // -1 means did not see anything
     if (BertIndex::unlink(s) == -1 && i > 3) break;
   }
  return result == 0;
}



#if 0

bool ShardedIndex::merge_two_parallel_fast(size_t src_idx, size_t dst_idx) {
    auto& source = shards[src_idx];
    auto& dest = shards[dst_idx];

    // Lock both
    std::scoped_lock lock(source.rwmutex, dest.rwmutex);

    // DNA Check
    if (!(source.get_meta() == dest.get_meta())) return false;

    // Execute the unified merge
    return dest.append_from(source) >= 0;
}

#endif
