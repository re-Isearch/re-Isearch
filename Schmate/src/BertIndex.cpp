#include "BertIndex.hpp"
#include "Util.hpp"
#include "OffsetFile.hpp"
#include "Logger.hpp"
#include "StringUtils.hpp"
#include "StringEmbedding.hpp"

#include <sstream>
#include <chrono>


using namespace std;

// If you are using a standard distribution of HNSWlib you must set this to 0
// as it ONLY WORKS with our modified code. We added the possibility to save
// and load the graphs to a stream.
#define HNSW_META 1 /* THIS USES OUR MODIFIED HNSWlib !! */


// Check if valid query
// ============================================================================
/*
 * Edge cases handled:
 *    - Empty strings
 *    - Whitespace-only (ASCII and some Unicode)
 *    - Punctuation-only
 *    - Mixed ASCII + UTF-8
 *    - Emoji and special characters
 *    - Invalid UTF-8 sequences (graceful degradation)
 */
inline bool is_valid_query(const std::string& query) {
    // Empty check
    if (query.empty()) return false;
    
    // Whitespace-only check
    if (is_empty_or_whitespace_utf8(query)) return false;
    
    // Has meaningful content (works with UTF-8)
    if (!has_printable_content(query)) return false;
    
    return true;
}


// ---------------------------------------------------------------------------
// Convert raw HNSW distance to a normalized score in [0,1]
// Handles cosine, L2, and inner product safely.
// Automatically clamps and guards against NaN/inf values.
// ---------------------------------------------------------------------------
float BertIndex::score_from_dist(float dist) const {
    // Safety first: guard against invalid or extreme distances
    if (!std::isfinite(dist) || dist > 1e6f) return 0.0f;
    if (dist < -1e6f) return 1.0f;

    switch (metric) {
        case MetricSpace::L2:
            // For L2 distance: smaller = closer. Map inversely.
            // This keeps results in (0,1] for any reasonable range.
            return 1.0f / (1.0f + dist);

	case MetricSpace::Binary: /* Need to confirm is correct! Oct 2025 */
        case MetricSpace::Cosine:
            // For cosine: distance = 1 - cosine_similarity
            // → similarity = 1 - distance
            // Clamp to [0,1] to avoid minor numeric drift.
	    // return (std::clamp(1.0f - dist, 0.0f, 1.0f) + 1.0f)/2.0f;
	    return (2.0f - dist)/2.0f;

        case MetricSpace::InnerProduct:
            // Inner product: higher = closer. HNSWlib may return negatives
            // if embeddings aren't normalized. Clamp to [-1,1].
            return std::clamp(dist, -1.0f, 1.0f);

        default:
            // Unknown metric
            return 0.0f;
    }
}

inline std::unique_ptr<hnswlib::SpaceInterface<float>> AllocateSpace(MetricSpace metric, size_t dim)
{
    switch (metric) {
       case MetricSpace::L2:           return make_unique<hnswlib::L2Space>(dim);
       case MetricSpace::InnerProduct: return make_unique<hnswlib::InnerProductSpace>(dim);
       case MetricSpace::Cosine:       return make_unique<hnswlib::InnerProductSpace>(dim);
/*
       case MetricSpace::Binary:       return make_unique<hnswlib::BinarySpace>(dim);
*/
       case MetricSpace::Binary:       LOG_ERROR_S() << "AllocateSpace(Binary, " << dim << ") should not be called";
       default: break;
    }
    throw std::runtime_error("Allocate space unknown metric!");
}

BertIndex::BertIndex(SBertGGML & emb, HnswConfig & c, const string & n, bool searchOnly) : embedder(emb), cfg(c), name(n)
{
    size_t max_elements = cfg.max_elements;
    sentences_path = name + IndexFileExtensions::sentences;
    offsets_path   = name + IndexFileExtensions::offsets;
    index_path     = name + IndexFileExtensions::hnsw;


    search_ctrl.adaptive_ef = cfg.auto_tune_ef;
    search_ctrl.adaptive_epsilon = cfg.auto_tune_eps;
    if (cfg.ef_search) search_ctrl.set_ef(cfg.ef_search);

    // In BertIndex constructor
   if ( file_size(index_path) > (signed int)sizeof(IndexMeta)  ) {

     // If auto tune enabled, load persist file if exists
     search_ctrl.load(name);

// std::cerr << "FILE SIZE=" << file_size(index_path) << "  META=" << sizeof(IndexMeta) << std::endl;

#if HNSW_META
    std::ifstream ifs(index_path, std::ios::binary);
    if (!ifs) throw std::runtime_error("Cannot open " + index_path);

    IndexMeta meta;
    meta.load(ifs);

    auto [expected_count, max_from_file] = hnswlib::peek_index_elements(ifs);

    // Use the max from file (it already includes the saved max_elements)
    max_elements = searchOnly ? max_from_file : std::max(max_from_file, cfg.max_elements);

#if 0
    if (cfg.debug)
        LOG_DEBUG_S() << "Loaded index meta v." << meta.version << ": metric=" << (int)meta.metric
                      << " normalized=" << meta.normalized
                      << " dim=" << meta.dim
                      << " count=" << meta.count << " index_count=" << expected_count
		      << " max from file=" << max_from_file;
#endif


    // Validate metric, dim, normalization
    if (meta.dim != embedder.n_embd) {
        LOG_WARN_S() << "[WARN] Index dim mismatch: expected " << embedder.n_embd
                     << ", found " << meta.dim;
    }

    metric = meta.metric; // Set the metric for this BertIndex

    if (metric != cfg.metric) {
        LOG_WARN_S() << "Metric mismatch: index uses '"
		<< cfg.metric_space_to_string(metric)
		<< "', runtime specified '"
		<< cfg.metric_space_to_string(cfg.metric) << "'. Changed";
        cfg.metric = metric; 
    }

    bool want_norm = (metric == MetricSpace::Cosine) ;
    if (meta.normalized != want_norm) {
        LOG_WARN_S() << "[WARN] Normalization mismatch: index normalized="
                     << meta.normalized << ", runtime expects=" << want_norm;
       meta.normalized = want_norm;
    }

    space = AllocateSpace(metric, embedder.n_embd);

#if 1
    // This saves a malloc!
    index = std::make_unique<hnswlib::HierarchicalNSW<float>>(space.get(), ifs, max_elements);
#else
    index = std::make_unique<hnswlib::HierarchicalNSW<float>>(space.get(), max_elements);
    index->loadIndex(ifs, space.get()); // proper stream-based load
#endif
    ifs.close();

    if (cfg.debug) {
        // Number of elements currently in the index:  index->cur_element_count
        // Maximum capacity: index->max_elements_
        LOG_DEBUG_S() << "Index contains " << index->cur_element_count << " / " <<  index->max_elements_ << " elements";
        LOG_DEBUG_S() << "Fill ratio: " << (100.0 * index->cur_element_count /  index->max_elements_)
		<< (searchOnly ? "%" : "% searchOnly Mode");
    }
#else
    space = AllocateSpace(cfg.metric, embedder.n_embd);
    index = std::make_unique<hnswlib::HierarchicalNSW<float>>(
            space.get(), index_path, 0    // <-- note: pass space.get()
                );   
#endif
      if (cfg.debug) LOG_DEBUG_S() << "Loaded existing index: " << index_path;
      next_label = index->cur_element_count; // Number of elements in the existing HNSW index
      max_elements = next_label + 1;
   } else {
      // Did not have an existing index
      // if searchOnly we could return here.. But for now we'll assume otherwise
      if (searchOnly)
	LOG_ERROR_S() <<
	"BertIndex SearchOnly specified for a non-existant index: \"" << name << "\"?!";

      metric = cfg.metric; // Use the configured metric

      space = AllocateSpace(metric, embedder.n_embd);
      // create new index if not found
       index = std::make_unique<hnswlib::HierarchicalNSW<float>>(
        space.get(), max_elements, cfg.M, cfg.ef_construction);
       if (cfg.debug)  LOG_DEBUG_S() << "Created new index with capacity=" << max_elements;
    }

    if (index && search_ctrl.adaptive_ef) index->setEf(search_ctrl.get_ef());

    sentences_file.open(sentences_path, ios::in|ios::out|ios::binary | ios::app);
    if (!sentences_file) throw runtime_error("Failed to open sentences file: " + sentences_path);


  // Try to load offsets if they exist
  // was    load_offsets();
  offsets = std::make_unique<OffsetFile>(offsets_path, max_elements);

  if (cfg.debug && !offsets->validate_offsets(/*fix=*/true))
     LOG_WARN_S() << "Offset file contained invalid entries; they were reset.";

#if 0
   if (cfg.debug) offsets->for_each([](size_t lbl, const OffsetEntry &e){
    LOG_DEBUG_S() << "label=" << lbl << " sid=" << e.sid
              << " file=[" << e.file_start << "," << e.file_end << "]"; });
#endif
}

// TODO: add a reset_search_ctrl()
//
// This would reset the epsilon and ef to the values in the main
// configuration
//

void BertIndex::clear() {
    acquire_lock(); // 🔒 Ensure no one else writes
    ScopedLock guard([&]() { release_lock(); }); // auto-unlock

    if (cfg.debug)
        LOG_INFO_S() << "[clear] Resetting index at " << name;

    // --- Step 1: Close open resources ---
    if (offsets) {
        offsets->flush();
        offsets.reset();
    }

    if (sentences_file.is_open()) {
        sentences_file.flush();
        sentences_file.close();
    }

    // --- Step 2: Remove existing files ---
    auto try_remove = [&](const std::string &path) {
        std::error_code ec;
        if (std::filesystem::exists(path)) {
            std::filesystem::remove(path, ec);
            if (ec && cfg.debug)
                LOG_WARN_S() << "[clear] Could not remove " << path << ": " << ec.message();
        }
    };

    try_remove(index_path);       // e.g., default.hnsw
    try_remove(offsets_path);     // e.g., default.off
    try_remove(sentences_path);   // e.g., default.sfs

    // Need to remove our adaptive tuning files too
    try_remove(name + IndexFileExtensions::tuner);
    try_remove(name + IndexFileExtensions::eps);

    // Reset to HNSWConfig values
    search_ctrl.set_ef(cfg.ef_search);
    search_ctrl.set_epsilon(0.0f);

    // --- Step 3: Reinitialize new, empty files ---
    offsets = std::make_unique<OffsetFile>(offsets_path, cfg.max_elements);
    sentences_file.open(sentences_path, std::ios::out | std::ios::trunc | std::ios::binary);
    sentences_file.close(); // leave empty

    // Recreate an empty HNSW index
    index = std::make_unique<hnswlib::HierarchicalNSW<float>>(
        space.get(), cfg.max_elements, cfg.M, cfg.ef_construction);

    // --- Step 4: Reset runtime counters ---
    next_label = 0;
    while (!free_labels.empty()) free_labels.pop();  // clear all recycled labels
    dirty_count = 0;
    auto_sentence_id = 0;

    // --- Step 5: Optionally persist meta, hyperparameters etc ---
    // save_meta();

    if (cfg.debug)
        LOG_INFO_S() << "[clear] Index \"" << name << "\" successfully reset.";
}




BertIndex::~BertIndex() {
    search_ctrl.save(name);
    try {
        flush(); // persist any unsaved changes
        remove_lockfile(); // We ignore if it fails since it is probably 0 from another process
    } catch (...) {
        // destructor should not throw
    }
}


// --- chunking ---
std::vector<Chunk> BertIndex::chunk_tokens(const std::string &sentence) {
    std::vector<Chunk> chunks;

    // Naive tokenization: split by whitespace
    // (replace with actual tokenizer if available)
    std::istringstream iss(sentence);
    std::vector<std::string> tokens;
    std::string tok;
    while (iss >> tok) {
        tokens.push_back(tok);
    }

    size_t n_tokens = tokens.size();
    size_t max_tokens = cfg.max_tokens_per_chunk;   // configurable
    size_t overlap = (size_t)(cfg.overlap_percent * max_tokens / 100.0);

    size_t start = 0;
    while (start < n_tokens) {
        size_t end = std::min(start + max_tokens, n_tokens);

        // Build chunk text from tokens[start:end)
        std::string text;
        for (size_t i = start; i < end; i++) {
            if (!text.empty()) text += " ";
            text += tokens[i];
        }

        chunks.push_back({text, start, end});

        if (end == n_tokens) break;
        start = end - overlap;  // slide with overlap
    }

    return chunks;
}


/// 

#if 1

// Append (auto generate sentence_id) -> returns label

size_t BertIndex::append(const string & sentence) {
   if (auto_sentence_id == 0) {
     // compute max SID
     offsets->for_each([&](size_t, const OffsetEntry &e) {
        if (e.sid > auto_sentence_id) auto_sentence_id = e.sid; });
    }

    int64_t sid = ++auto_sentence_id;
    if (cfg.debug) LOG_DEBUG_S() << "Append @" << sid << ": " << sentence;
    return append(sentence, sid);
}


bool BertIndex::acquire_lock() const {
    if (!file_lock) {
        std::string lock_path = name + IndexFileExtensions::lock;
        file_lock = std::make_unique<FileLock>(lock_path);
    }

    if (!file_lock->try_lock()) {
        throw std::runtime_error("Index locked by another process: " + name);
        return false;
    }

    if (cfg.debug)
        LOG_DEBUG_S() << "Lock acquired for write: " << name;
    return true;
}

void BertIndex::release_lock() const {
    if (file_lock) {
        file_lock->unlock();
        if (cfg.debug)
            LOG_DEBUG_S() << "Lock released for write: " << name;
    }
}


#include <signal.h>

// Is the process running by someone else???
static bool pid_active_other(pid_t pid) {
#if defined(_MSDOS) || defined(_WIN32)
  /* Note: Could do better - maybe later.  */
  //Only need to check processes that are no me!
  if(pid == GetCurrentProcessId()) return false;
  HANDLE hProc = NULL;
  if(!(hProc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid))) return false;
  CloseHandle(hProc);
  return true;
#else
    while(waitpid(-1, 0, WNOHANG) > 0) {
        // Wait for defunct....
    } 
  // Only need to check processes that are not me!
  if (pid <= 0 || pid == ::getpid()) return false; // Its me
  if (kill (pid, 0) == -1 &&  errno == ESRCH)
    return false;
  return true; // Looks OK
#endif  
}       

int BertIndex::wait_lock() const {
    std::string lock_path = name + IndexFileExtensions::lock;
    if (file_size (lock_path) > 0) {
      std::ifstream ifs(lock_path);
      int pid = -1;
      ifs >> pid;
      if (!pid_active_other(pid))
	return 0; 
      // File locked by another process
      LOG_INFO_S() << "Index \"" << name
	<< "\" locked by another process " << pid << " waiting";
      if (!wait_for_file_removal(lock_path, std::chrono::seconds(60)))
        return pid;
     LOG_INFO_S() << "Index \"" << name << "\" append continuing.";
    }
    if (!acquire_lock()) return -1;
    return 0;
}

bool BertIndex::remove_lockfile() const {
   const std::string lock_path = name + IndexFileExtensions::lock;
   if (file_exists(lock_path)) {
     return  std::filesystem::remove(lock_path);
   }
   return true;
}


std::vector<float> BertIndex::encode_text(const std::string& text)
{
   // std::cerr << "BertIndex::encode_text(" << text << ")\n";
   if (text.empty()) return {}; // Empty text 

   // Need to check/set lock

   auto emb = embedder.encode_text(text, cfg.debug);

#if !defined(UNIFIED_INDEX_) || UNIFIED_INDEX_ == 0
// When we shift over to the UnifiedIndex we need to remove this
// code 
   // ---------------------------------------------------------------------
   // Normalize for cosine similarity if required.
   // ---------------------------------------------------------------------
   if (metric == MetricSpace::Cosine) {
        float norm = 0.f;
        for (float v : emb) norm += v * v;
        norm = std::sqrt(norm);
        if (norm > 0.f) for (float &v : emb) v /= norm;
//        if (cfg.debug) LOG_DEBUG_S() << "embedding normalized (cosine)";
    } else if (cfg.debug) {
        float norm = 0.f;
        for (float v : emb) norm += v * v;
        LOG_DEBUG_S() << "embedding norm=" << std::sqrt(norm);
    }

#if 0
  if (cfg.debug) {
    float norm = 0;
    LOG_DEBUG_S() << "Embedding preview:";
    for (int i = 0; i < 10; ++i) {
        LOG_DEBUG_S() << "  e[" << i << "]=" << emb[i];
        norm += emb[i] * emb[i];
    }
    LOG_DEBUG_S() << "embedding norm=" << sqrt(norm);
   }
#endif
#endif
    return emb;
}

// Append with explicit sentence_id -> returns label

// Takes 1) a string of any length. Longer strings get sliced into chunks
// 2) A hex encoded string representing the Float32 vector of the embedding.
// This can be potentially useful in some edge use cases such as when it is
// desireable to store the embedding vector in the document. A typical application
// is for image retrieval. Its main constraint is that the dimension of the
// vector MUST be the same as the dimension specified for the index. 
// Should something else be desired we'd have to handle that in the ShardedIndex
// class by creating a new shard with the appropriate dimension.  
size_t BertIndex::append(const std::string &sentence, int64_t sentence_id) {

    if (cfg.lock_on_append && wait_lock()) {
        LOG_FATAL_S() << "Can't append, other process competing (race).";
        return 0;
    }
    bool insert_raw = false;
    std::vector<Chunk> chunks;
    // NEW: If the sentence passed is a string containing a hex encoded
    // Float32 vector of the right dim then treat as such (pass-through)
    if (schmate_util::isHexFloat32Vector(sentence, embedder.n_embd)) {
      // Looks like a Float32 encoded vector: just hex and right length
      chunks.push_back({sentence, 0, sentence.length()}); 
      insert_raw = true;
    } else chunks = chunk_tokens(sentence);

    size_t last_label = 0;

    // Make sure monotonic
    if (sentence_id > auto_sentence_id) auto_sentence_id = sentence_id; 

    for (auto &chunk : chunks) {
        size_t label = allocate_label();

        // --- Write sentence chunk to sentences file ---
        sentences_file.seekp(0, std::ios::end);
        int64_t file_start = (int64_t)sentences_file.tellp();
        sentences_file.write(chunk.text.data(), chunk.text.size());
        int64_t file_end = (int64_t)sentences_file.tellp();

        // --- Insert into HNSW index ---
        // encode & add
        if (insert_raw) {
          auto emb = schmate_util::hexToFloat32 (chunk.text);
          index->addPoint(emb.data(), (hnswlib::labeltype)label);
        } else {
          auto emb = encode_text(chunk.text);
          index->addPoint(emb.data(), (hnswlib::labeltype)label);
        }

        // --- Write OffsetEntry into mmap ---
        OffsetEntry e{ sentence_id,
                       chunk.start_token,
                       chunk.end_token,
                       file_start,
                       file_end };
	// std::cout << "Set label " << label << "\n";
        offsets->set(label, e); // In-memory only: Writes into mmap region directly

        // Incremental durability (the offset file is always consistent on disk)
        // while deferring heavy I/O (HNSW saves) until necessary.

        if (cfg.flush_offsets_each)
	   offsets->flush(label); // Syncs only 16 bytes (1 entry) 

        if (cfg.debug) {
             LOG_DEBUG_S() << "append label=" << label
                      << " sid=" << sentence_id
                      << " tok=[" << chunk.start_token
                      << "," << chunk.end_token << ")"
                      << " file=[" << file_start
                      << "," << file_end << ")";
        }

        last_label = label;
    }

    dirty_count++;
    if (dirty_count >= cfg.flush_threshold) {
        // dirty_count  Triggers index flush: Used to throttle HNSW saves
        flush();
    }

    return last_label; // return last label inserted for convenience
}


// remove / undelete (unchanged except usage)
#if 1

void BertIndex::remove(size_t label) {
    index->markDelete(label);

    // clear offset entry
    OffsetEntry empty{};
    offsets->set(label, empty);

    // recycle label
    free_labels.push(label);

    if (++dirty_count > cfg.flush_threshold) flush();
}

void BertIndex::undelete(size_t label, const OffsetEntry &entry) {
    // restore offset entry
    offsets->set(label, entry);

    // restore in HNSW graph
    index->unmarkDelete(label);

    if (++dirty_count >= cfg.flush_threshold) flush();
}


#else
    void BertIndex::remove(size_t label) {
        if (index) index->markDelete((hnswlib::labeltype)label);

        // zero offsets (mark deleted persistently)
        fstream ofs(offsets_path, ios::in | ios::out | ios::binary);
        if (ofs) {
            ofs.seekp((streamoff)label * 16);
            write_int64(ofs, 0);
            write_int64(ofs, 0);
            ofs.flush();
        }

        dirty_count++;
    }

    void BertIndex::undelete(size_t label) {
        if (index) index->unmarkDelete((hnswlib::labeltype)label);
        // Note: offsets file still zero for deleted; if you want to undelete you must re-write offsets.
        dirty_count++;
    }

    // accessor: number of labels allocated (next_label)
    size_t BertIndex::label_count() const {
        return next_label;
    }
#endif

#else

void BertIndex::append(const string & sentence) {
    int64_t sid = ++auto_sentence_id;
    append(sentence, sid);
}

void BertIndex::append(const string & sentence, int64_t sentence_id) {
    auto chunks = chunk_tokens(sentence, cfg.max_tokens_per_chunk, cfg.overlap_percent);

    fstream ofs(offsets_path, ios::in|ios::out|ios::binary|ios::app);
    if (!ofs) ofs.open(offsets_path, ios::out|ios::binary);

    ofstream sfs(sentences_path, ios::app|ios::binary);

    for (auto &chunk : chunks) {
        size_t label = next_label++;

        // write text
        sfs.seekp(0, ios::end);
        int64_t start = (int64_t)sfs.tellp();
        sfs.write(chunk.text.data(), chunk.text.size());
        sfs.put('\n');
        int64_t end = (int64_t)sfs.tellp();

        // write offsets
        ofs.seekp(label*16);
        write_int64(ofs, start);
        write_int64(ofs, end);

        // encode & add
        auto emb = encode_text(chunk.text);
        index->addPoint(emb.data(), (hnswlib::labeltype)label);

        chunk_token_map[label] = {chunk.start_token, chunk.end_token};
        chunk_sentence_map[label] = sentence_id;

        if (cfg.debug) {
             LOG_DEBUG_S() << "append label=" << label
                 << " sid=" << sentence_id
                 << " tok=[" << chunk.start_token << "," << chunk.end_token << ")"
                 << " file=[" << start << "," << end << ")";
        }

        if (++dirty_count >= cfg.flush_threshold) flush();
    }
}

///


void BertIndex::remove(size_t label) {
    index->markDelete(label);
    dirty_count++;
}

void BertIndex::undelete(size_t label) {
    index->unmarkDelete(label);
    dirty_count++;
}
#endif

void BertIndex::flush() {
 // Only need to flush when we have a diff with the HNSW on disk
  if (dirty_count) {
    save();

    // Flush offsets ONLY if not already sync'd 
    if (!cfg.flush_offsets_each && offsets)
      offsets->flush(); // msync during add

    sentences_file.flush();

    if (cfg.debug)  LOG_DEBUG_S() <<  "Flushed index + sentences to disk";
  } else if (size() == 0) {
    // Nothing done but also no contents so we can delete its droppings
  }
}

void BertIndex::save() {

// Rewrites entire index: normally done in batches after X inserts
  if (size() > 0) {
#if HNSW_META
       std::ofstream ofs(index_path, std::ios::binary | std::ios::trunc);
       if (!ofs) throw std::runtime_error("Cannot open " + index_path + " for writing");

       IndexMeta meta;
       meta.version = 1;
       meta.metric = metric;
       meta.normalized = (metric == MetricSpace::Cosine);  // or cfg.normalized_embeddings flag
       meta.dim = embedder.n_embd;
       meta.count = this->size();

       meta.save(ofs);
       index->saveIndex(ofs);
       ofs.close();
#else
        index->saveIndex(index_path);
#endif
    } else if (file_size(index_path) >= 0) {
      // since != -1 we know it exists
      unlink(index_path.c_str());
    }
    dirty_count = 0; // Memory = disk
    if (cfg.debug)  LOG_DEBUG_S() << "saved index " << index_path;
    release_lock();
}

/*
void BertIndex::load() {
    string idx_path = name + "_index.bin";
    ifstream f(idx_path);
    if (!f.good()) return;
    f.close();

    index = make_unique<hnswlib::HierarchicalNSW<float>>(space.get(), idx_path, false);
    next_label = index->cur_element_count;
    if (cfg.debug)  LOG_DEBUG_S() << "loaded index " << idx_path
                        << " with " << next_label << " items";
}
*/


/// Search Methods

template<typename FilterFn>
std::vector<SearchResult> BertIndex::filter_knn_results(const std::string &query,
                                                        size_t max_k,
                                                        FilterFn filter) {
    if (size() == 0 || !is_valid_query(query))
        return {}; // Nothing to do 

    // std::cerr << "QUERY=" << query << std::endl;
    std::vector<float> emb = encode_text(query); 

    if (search_ctrl.adaptive_ef) index->setEf(search_ctrl.get_ef());

    auto beg = std::chrono::high_resolution_clock::now();
    auto candidates = index->searchKnnCloserFirst(emb.data(), max_k);
    auto end = std::chrono::high_resolution_clock::now();
    auto latency_ms = duration_cast<std::chrono::microseconds>(end - beg).count();
    search_ctrl.update_after_knn(latency_ms, cfg.debug);

    std::vector<SearchResult> results;
    results.reserve(max_k);

    for (auto &[dist, label] : candidates) {
        float score = score_from_dist(dist);
std::cerr << "DIST=" << dist << "  score=" << score << std::endl;
        if (!filter(score)) continue;

        OffsetEntry e = offsets->get(label);
        if (!is_valid_entry(e)) continue;

        if (cfg.debug) LOG_DEBUG_S() << "label=" << label << " sid=" << e.sid
              << " score=" << score << " file=[" << e.file_start << "," << e.file_end << "]";
        SearchResult r;
        r.score      = score;
        r.label      = label;
        r.sentence_id= e.sid;
        r.token_start= e.start_tok;
        r.token_end  = e.end_tok;
        r.file_start = e.file_start;
        r.file_end   = e.file_end;
        r.text       = get_text_by_label(label);

        if (!r.text.empty()) results.push_back(std::move(r));
    }


/*
| Metric            | Meaning            | Best value         | Sort order       |
| ----------------- | ------------------ | ------------------ | ---------------- |
| L2 / Euclidean    | smaller distance   | → smaller = better | ascending (`<`)  |
| Cosine similarity | larger cosine      | → larger = better  | descending (`>`) |
| Inner product     | larger dot product | → larger = better  | descending (`>`) |
*/
//    const bool higher_is_better =
//    	(metric == MetricSpace::Cosine || metric ==  MetricSpace::InnerProduct);

    std::sort(results.begin(), results.end(),
          [this](const SearchResult &a, const SearchResult &b) {
              switch(metric) {
		case MetricSpace::Cosine:
		case MetricSpace::InnerProduct:
                  return a.score > b.score;
		case MetricSpace::L2:
                  return a.score < b.score;
		default: break;
	      }
	      return false; // Not defined case???
          });

    return results;
}


std::vector<SearchResult> BertIndex::search(const std::string &query) {
  switch(cfg.default_search_mode) {
	case SearchModes::Knn:      return knn(query);
        case SearchModes::Radius:   return radius(query);
        case SearchModes::Relative: return relative(query);
        case SearchModes::Adaptive: return adaptive(query);
        case SearchModes::Epsilon:  return epsilon_search(query);
  }
  LOG_ERROR_S() << "Unknown default search mode for BertIndex::search()";
  return {};
}


std::vector<SearchResult> BertIndex::knn(const std::string &query, size_t k) {
    if (k <= 0) k = cfg.default_k;
    return filter_knn_results(query, k, [](float) { return true; });
}



// radius, relative, adaptive are similar; stubbed for brevity

std::vector<SearchResult> BertIndex::radius(const std::string &query, float r) {
    if (r<0) r = cfg.default_radius;
    return filter_knn_results(query, cfg.max_elements, [r](float score) {
        return score <= r;
    });
}


std::vector<SearchResult> BertIndex::relative(const std::string &query, float alpha, size_t max_k) {
    if (alpha<0) alpha = cfg.default_alpha;
    if (max_k <=0) max_k = cfg.default_k*cfg.knn_lookahead_scale;

    std::vector<float> emb = encode_text(query); // embed(query);
    auto topk = index->searchKnnCloserFirst(emb.data(), max_k);
    if (topk.empty()) return {};

    float best = topk.front().first;
    float threshold = best * alpha;

    // reuse the helper but with captured threshold
    return filter_knn_results(query, max_k, [threshold](float score) {
        return score <= threshold;
    });
}


std::vector<SearchResult> BertIndex::adaptive(const std::string &query,
                                                     float alpha,
                                                     size_t minN,
                                                     size_t lookahead,
                                                     float gapDelta) {
    if (alpha<0) alpha=cfg.default_alpha;
    if (minN==0) minN=cfg.default_minN;
    if (lookahead==0) lookahead=cfg.default_lookahead;
    if (gapDelta<0) gapDelta=cfg.default_gapDelta;

    std::vector<float> emb = encode_text(query);// embed(query);
    auto topk = index->searchKnnCloserFirst(emb.data(), lookahead);
    if (topk.empty()) return {};

    float last_score = -1;
    size_t count = 0;
    std::vector<float> accepted;

    for (auto &[score, _] : topk) {
        if (count >= minN && last_score > 0 && (score - last_score) > gapDelta)
            break;
        accepted.push_back(score);
        last_score = score;
        count++;
    }

    if (accepted.empty())
        return {};

    float threshold = accepted.back();
    return filter_knn_results(query, lookahead, [threshold](float score) {
        return score <= threshold;
    });
}


/*

In epsilon search we don't tune the ef_search!

| Search Type          | Stopping Criterion                  | ef_search relevance                       |
| -------------------- | ----------------------------------- | ----------------------------------------- |
| **kNN**              | after collecting *k* best items     | ⚡ high (controls recall/latency)          |
| **radius / epsilon** | after exploring all within distance | ⚠️ limited (distance threshold dominates) |

*/


std::vector<SearchResult> BertIndex::epsilon_search(const std::string &query, float epsilon) {
    size_t cur_count = size(); // index->cur_element_count;
    if (cur_count == 0 || !is_valid_query(query) )
        return {}; // Empty index or invalid query

    size_t min_candidates = std::min(cfg.min_candidates, cur_count);
    size_t max_candidates = cfg.max_candidates_cap > 0
                        ? std::min(cfg.max_candidates_cap, cur_count)
                        : cur_count;

    if (cfg.auto_tune_eps) epsilon = search_ctrl.get_epsilon();
    if (epsilon <= 0.0f)   epsilon = cfg.get_epsilon(metric);
 
    if (!cfg.auto_tune_eps && metric == MetricSpace::L2) epsilon = epsilon * epsilon; 

    if (max_candidates == min_candidates && max_candidates > 3) min_candidates = max_candidates - 2;

#if 0
    if (cfg.debug) LOG_DEBUG_S() << "BEFORE SEARCH: query=" << query << " min_candidates=" << min_candidates
              << " max_candidates=" << max_candidates
              << " epsilon=" << epsilon;
#endif

    std::vector<float> emb = encode_text(query);

/*
KEY PARAMETERS EXPLAINED:

epsilon: Distance threshold
  - Only candidates with distance <= epsilon are returned
  - Squared distance for L2Space, so epsilon should be squared distance

min_candidates: Minimum number of results to return
  - If fewer than min_candidates are within epsilon, search expands
  - Set to 0 for strict epsilon search (may return empty)
  - Set to 1+ to guarantee at least N results

max_candidates: Maximum number of results to return
  - Caps the number of returned candidates
  - Prevents returning too many results
  - Set high (e.g., max_elements) for unbounded search

TYPICAL USE CASES:
1. Strict radius search: min=0, max=high, appropriate epsilon
2. Flexible search: min=k, max=high, epsilon as soft threshold
3. Bounded search: min=k, max=k*10, epsilon for quality control
*/
    // Create a stop condition
    hnswlib::EpsilonSearchStopCondition<float> stop_condition( epsilon, min_candidates, max_candidates);
    auto candidates = index->searchStopConditionClosest(emb.data(), stop_condition);

    // Update tuner based on result density
/*
    | Situation                          | Behavior                             |
    | ---------------------------------- | ------------------------------------ |
    | Too few results (<80% of target)   | Increase ε slightly (expand radius). |
    | Too many results (>120% of target) | Shrink ε slightly (tighten radius).  |
    | Stable result density              | ε converges.                         |
*/
    search_ctrl.update_after_epsilon(candidates.size(), cfg.debug);

#if 0
    LOG_DEBUG_S() << "=== RAW candidates from searchStopConditionClosest ===";
    LOG_DEBUG_S() << "Returned " << candidates.size() << " candidates";
    for (size_t i = 0; i < candidates.size(); i++) {
        LOG_DEBUG_S() << "  [" << i << "] dist=" << candidates[i].first 
                  << " label=" << candidates[i].second;
    }
#endif

    std::vector<SearchResult> results;
    results.reserve(candidates.size());

    for (auto &[dist, label] : candidates) {

        if (cfg.debug) LOG_DEBUG_S() << "dist=" << dist << " epsilon = " << epsilon
		  << " metric=" << static_cast<int>(metric)
                  << " score(before clamp)=" << (1.0f - dist);

        if (metric == MetricSpace::L2) dist = sqrt(dist);
        float score = score_from_dist(dist);
        
        OffsetEntry e = offsets->get(label);
        if (!is_valid_entry(e)) continue;
        
        if (cfg.debug) LOG_DEBUG_S() << "label=" << label << " sid=" << e.sid
              << " score=" << score << " file=[" << e.file_start << "," << e.file_end << "]";
        SearchResult r;
        r.score      = score;
        r.label      = label;
        r.sentence_id= e.sid;
        r.token_start= e.start_tok;
        r.token_end  = e.end_tok;
        r.file_start = e.file_start;
        r.file_end   = e.file_end;
        r.text       = get_text_by_label(label);

        if (!r.text.empty()) results.push_back(std::move(r));
    }
    std::sort(results.begin(), results.end(),
              [](auto &a, auto &b) { return a.score > b.score; });

    return results;
}

///
// Reconstruct sentence
#if 1
std::string BertIndex::reconstruct_sentence(int64_t sid) const {
    auto entries = offsets->find_by_sid(sid);
    if (entries.empty()) return "";

    std::sort(entries.begin(), entries.end(),
              [](auto &a, auto &b) { return a.second.start_tok < b.second.start_tok; });

    std::string result;
    size_t last_end = 0;

    for (auto &[label, e] : entries) {
        std::string chunk = get_text_by_label(label);

        if (chunk.empty())  LOG_ERROR_S() << "CHUNK is EMPTY in reconstruct sentence";

        if (result.empty()) {
            result = chunk;
        } else {
            // handle overlap
            size_t overlap = (last_end > e.start_tok) ? (last_end - e.start_tok) : 0;
            size_t skip_chars = 0;

            for (size_t i = 0; i < overlap && skip_chars < chunk.size(); ++i) {
                auto pos = chunk.find(' ', skip_chars);
                if (pos == std::string::npos) {
                    skip_chars = chunk.size();
                } else {
                    skip_chars = pos + 1;
                }
            }

            if (skip_chars < chunk.size()) {
                if (!result.empty()) result += " ";
                result += chunk.substr(skip_chars);
            }
        }

        last_end = e.end_tok;
    }

//    return util::trim(result);
   return result;
}


#else

string BertIndex::reconstruct_sentence(int64_t sentence_id) const {
#if 1
    // Collect all chunks for this sentence
    std::vector<std::pair<int, std::string>> parts;

    for (auto &entry : chunk_sentence_map) {
        size_t label = entry.first;
        int64_t sid = entry.second;

        if (sid != sentence_id) continue;

        auto tok_it = chunk_token_map.find(label);
        if (tok_it == chunk_token_map.end()) continue;

        int start_tok = tok_it->second.first;
        int end_tok   = tok_it->second.second;

        std::string chunk = get_text_by_label(label);
        if (chunk.empty()) continue; // skip deleted

        parts.emplace_back(start_tok, chunk);
    }

    if (parts.empty()) return "";

    // Sort chunks by starting token index
    std::sort(parts.begin(), parts.end(),
              [](auto &a, auto &b){ return a.first < b.first; });

    // Concatenate
    std::string result;
    for (auto &p : parts) {
        if (!result.empty()) result += " ";
        result += p.second;
    }
    return result;

#else
    struct Piece {int ts, te; string text;};
    vector<Piece> pcs;

    for (auto &kv : chunk_sentence_map) {
        if (kv.second != sentence_id) continue;
        size_t label = kv.first;

        fstream ofs(offsets_path, ios::in|ios::binary);
        ofs.seekg(label*16);
        int64_t s=read_int64(ofs), e=read_int64(ofs);
        if (s==0 && e==0) continue;

        ifstream sfs(sentences_path, ios::binary);
        string text(e-s, '\0');
        sfs.seekg(s); sfs.read(&text[0], e-s);

        auto tok=chunk_token_map.find(label);
        if (tok!=chunk_token_map.end())
            pcs.push_back({tok->second.first,tok->second.second,text});
    }

    sort(pcs.begin(),pcs.end(),[](auto&a,auto&b){return a.ts<b.ts;});
    string result;
    int last=-1;
    for (auto &p:pcs) {
        if (!result.empty() && p.ts>=last) result+=' ';
        result+=p.text;
        last=max(last,p.te);
    }
    return result;
#endif
}
#endif





#if 1
// Debugging version (to solve core dump)
std::string BertIndex::get_text_by_label(size_t label) const {
    if (!offsets) {
        throw std::runtime_error("OffsetFile not initialized");
    }

    OffsetEntry e = offsets->get(label);

    // Defensive checks
    if (e.sid == 0 && e.file_start == 0 && e.file_end == 0) {
        if (cfg.debug) {
             LOG_DEBUG_S() << "get_text_by_label(" << label << "): empty entry";
        }
        return "";
    }
    if (e.file_end <= e.file_start) {
        if (cfg.debug) {
             LOG_DEBUG_S() << "get_text_by_label(" << label << "): invalid offsets "
                      << e.file_start << "->" << e.file_end;
        }
        return "";
    }

    size_t len = static_cast<size_t>(e.file_end - e.file_start);
    if (len > 10'000'000) { // arbitrary sanity cap
        if (cfg.debug) {
             LOG_DEBUG_S() <<  "get_text_by_label(" << label << "): length too large " << len;
        }
        return "";
    }

    std::ifstream ifs(sentences_path, std::ios::binary);
    if (!ifs) {
        throw std::runtime_error("Failed to open sentences file: " + sentences_path);
    }

    ifs.seekg(e.file_start, std::ios::beg);
    std::string text(len, '\0');
    ifs.read(&text[0], len);

    return text;
}


#else 

std::string BertIndex::get_text_by_label(size_t label) const {
    auto it = chunk_token_map.find(label);
    if (it == chunk_token_map.end()) return "";

    int64_t sid = -1;
    auto sid_it = chunk_sentence_map.find(label);
    if (sid_it != chunk_sentence_map.end())
        sid = sid_it->second;

    std::ifstream ifs(sentences_path, std::ios::binary);
    if (!ifs.is_open()) return "";

    // look up start/end offsets from offsets file
    std::ifstream ofs(offsets_path, std::ios::binary);
    if (!ofs.is_open()) return "";

    ofs.seekg(label * 16); // two int64_t per entry
    int64_t start = read_int64(ofs);
    int64_t end   = read_int64(ofs);

    if (start == 0 && end == 0) {
        // deleted entry
        return "";
    }

    ifs.seekg(start);
    std::string text(end - start, '\0');
    ifs.read(&text[0], end - start);

    return text;
}
#endif


int64_t BertIndex::get_sentence_id(size_t label) const {
    auto it = chunk_sentence_map.find(label);
    if (it == chunk_sentence_map.end())
        return -1;
    return it->second;
}


std::string BertIndex::get_text(const SearchResult &r, bool full_sentence) const {
    if (full_sentence && r.sentence_id >= 0) {
        return reconstruct_sentence(r.sentence_id);
    }

    if (r.sentence_id >= 0) {
        // If we have a sentence_id, prefer full reconstruction unless told not to
        return full_sentence ? reconstruct_sentence(r.sentence_id)
                             : get_text_by_label(r.label);
    }

    // Fallback: just chunk text
    return get_text_by_label(r.label);
}

size_t BertIndex::allocate_label() {
    if (!free_labels.empty()) {
        size_t lbl = free_labels.front();
        free_labels.pop();
        return lbl;
    }

    if (next_label >= cfg.max_elements) {
        throw std::runtime_error("allocate_label: HNSW capacity exceeded");
    }

    return next_label++;
}


// accessor: number of labels allocated (next_label)
size_t BertIndex::label_count() const {
  return next_label;
} 

/*

OffsetEntry e = offsets->get(label);
result.sentence_id = e.sid;
result.start_tok   = e.start_tok;
result.end_tok     = e.end_tok;
result.file_start  = e.file_start;
result.file_end    = e.file_end;


*/
