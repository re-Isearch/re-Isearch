#include "BertIndex.hpp"
#include "Util.hpp"
#include "OffsetFile.hpp"
#include "Logger.hpp"

#include <sstream>

using namespace std;

#define HNSW_META 1

BertIndex::BertIndex(SBertGGML & emb, HnswConfig & c, const string & n)
: embedder(emb), cfg(c), name(n) {
    sentences_path = name + IndexExtensions::sentences;
    offsets_path   = name + IndexExtensions::offsets;
    index_path     = name + IndexExtensions::hnsw;

#if 1
    // In BertIndex constructor
   if ( file_exists(index_path) ) {
#if HNSW_META
    std::ifstream ifs(index_path, std::ios::binary);
    if (!ifs) throw std::runtime_error("Cannot open " + index_path);

    IndexMeta meta;
    meta.load(ifs);

    if (cfg.debug)
        LOG_DEBUG_S() << "Loaded index meta v." << meta.version << ": metric=" << (int)meta.metric
                      << " normalized=" << meta.normalized
                      << " dim=" << meta.dim
                      << " count=" << meta.count;

    // Validate metric, dim, normalization
    if (meta.dim != embedder.n_embd) {
        LOG_WARN_S() << "[WARN] Index dim mismatch: expected " << embedder.n_embd
                     << ", found " << meta.dim;
    }

    if (meta.metric != cfg.metric) {
        LOG_WARN_S() << "Metric mismatch: index uses '" << (int)meta.metric
                     << "', runtime specified '" << (int)cfg.metric << "'. Changed";
        cfg.metric = meta.metric; 
    }

    bool want_norm = (cfg.metric == Metric::Cosine) ;
    if (meta.normalized != want_norm) {
        LOG_WARN_S() << "[WARN] Normalization mismatch: index normalized="
                     << meta.normalized << ", runtime expects=" << want_norm;
       meta.normalized = want_norm;
    }

    switch (cfg.metric) {
       case Metric::L2:           space = make_unique<hnswlib::L2Space>(embedder.n_embd); break;
       case Metric::InnerProduct: space = make_unique<hnswlib::InnerProductSpace>(embedder.n_embd); break;
       case Metric::Cosine:       space = make_unique<hnswlib::InnerProductSpace>(embedder.n_embd); break;
    }

    index = std::make_unique<hnswlib::HierarchicalNSW<float>>(space.get(), cfg.max_elements);
    index->loadIndex(ifs, space.get()); // proper stream-based load
    ifs.close();
#else

    switch (cfg.metric) {
       case Metric::L2:           space = make_unique<hnswlib::L2Space>(embedder.n_embd); break;
       case Metric::InnerProduct: space = make_unique<hnswlib::InnerProductSpace>(embedder.n_embd); break;
       case Metric::Cosine:       space = make_unique<hnswlib::InnerProductSpace>(embedder.n_embd); break;
    }
    index = std::make_unique<hnswlib::HierarchicalNSW<float>>(
            space.get(), index_path, 0    // <-- note: pass space.get()
                );   
#endif
      if (cfg.debug) LOG_DEBUG_S() << "Loaded existing index: " << index_path;
      next_label = index->cur_element_count;
   } else {
     // Did not have an existing index
      switch (cfg.metric) {
       case Metric::L2:           space = make_unique<hnswlib::L2Space>(embedder.n_embd); break;
       case Metric::InnerProduct: space = make_unique<hnswlib::InnerProductSpace>(embedder.n_embd); break;
       case Metric::Cosine:       space = make_unique<hnswlib::InnerProductSpace>(embedder.n_embd); break;
      }
      // create new index if not found
       index = std::make_unique<hnswlib::HierarchicalNSW<float>>(
        space.get(), cfg.max_elements, cfg.M, cfg.ef_construction);
       if (cfg.debug)  LOG_DEBUG_S() << "Created new index with capacity=" << cfg.max_elements;
    }

    sentences_file.open(sentences_path, ios::in|ios::out|ios::binary | ios::app);
    if (!sentences_file) throw runtime_error("Failed to open sentences file: " + sentences_path);

#else // OLD CODE

    if (file_exists(index_path)) {
      index = std::make_unique<hnswlib::HierarchicalNSW<float>>(space.get(), index_path);
      if (cfg.debug)  LOG_DEBUG_S() << "Loaded existing index " << index_path;
    } else {
      switch (cfg.metric) {
        case Metric::L2:           space = make_unique<hnswlib::L2Space>(embedder.n_embd); break;
        case Metric::InnerProduct: space = make_unique<hnswlib::InnerProductSpace>(embedder.n_embd); break;
        case Metric::Cosine:       space = make_unique<hnswlib::InnerProductSpace>(embedder.n_embd); break;
      }

      index = make_unique<hnswlib::HierarchicalNSW<float>>(
        space.get(), cfg.max_elements, cfg.M, cfg.ef_construction);
    }
#endif

  // Try to load offsets if they exist
  // was    load_offsets();
  offsets = std::make_unique<OffsetFile>(offsets_path, cfg.max_elements);

  if (cfg.debug && !offsets->validate_offsets(/*fix=*/true))
     LOG_WARN_S() << "Offset file contained invalid entries; they were reset.";

   if (cfg.debug) offsets->for_each([](size_t lbl, const OffsetEntry &e){
    LOG_DEBUG_S() << "label=" << lbl << " sid=" << e.sid
              << " file=[" << e.file_start << "," << e.file_end << "]"; });

}


BertIndex::~BertIndex() {
    try {
        flush(); // persist any unsaved changes
    } catch (...) {
        // destructor should not throw
    }
}


// --- chunking ---
#if 1

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



#else
static vector<Chunk> chunk_tokens(const string & sentence,
                                  int max_tokens=128,
                                  float overlap=0.1f) {
    // naive tokenizer: split on spaces
    vector<string> tokens;
    string cur;
    for (char c : sentence) {
        if (isspace((unsigned char)c)) {
            if (!cur.empty()) { tokens.push_back(cur); cur.clear(); }
        } else cur.push_back(c);
    }
    if (!cur.empty()) tokens.push_back(cur);

    int step = max_tokens - int(overlap * max_tokens);
    vector<Chunk> chunks;
    for (size_t i=0;i<tokens.size();i+=step) {
        size_t end = min(tokens.size(), i+max_tokens);
        string text;
        for (size_t j=i;j<end;j++) {
            if (!text.empty()) text.push_back(' ');
            text += tokens[j];
        }
        chunks.push_back({text, (int)i, (int)end});
    }
    return chunks;
}
#endif

/// 

#if 1

#include "BertIndex.hpp"
using namespace std;

// --- existing constructor / helpers left unchanged ---

// Append (auto generate sentence_id) -> returns label



// BUG: TODO: Fix the auto_sentence_id to start somewhere 
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


std::vector<float> BertIndex::encode_text(const std::string& text)
{
   // std::cerr << "BertIndex::encode_text(" << text << ")\n";

   auto emb = embedder.encode_text(text, cfg.debug);
   // ---------------------------------------------------------------------
   // Normalize for cosine similarity if required.
   // ---------------------------------------------------------------------
   if (cfg.metric == Metric::Cosine) {
        float norm = 0.f;
        for (float v : emb) norm += v * v;
        norm = std::sqrt(norm);
        if (norm > 0.f) for (float &v : emb) v /= norm;
        if (cfg.debug) LOG_DEBUG_S() << "embedding normalized (cosine)";
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
    return emb;
}

// Append with explicit sentence_id -> returns label
#if 1

size_t BertIndex::append(const std::string &sentence, int64_t sentence_id) {
    auto chunks = chunk_tokens(sentence);
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
        auto emb = encode_text(chunk.text);
        index->addPoint(emb.data(), (hnswlib::labeltype)label);

        // --- Write OffsetEntry into mmap ---
        OffsetEntry e{ sentence_id,
                       chunk.start_token,
                       chunk.end_token,
                       file_start,
                       file_end };
        offsets->set(label, e);

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
        flush();
    }

    return last_label; // return last label inserted for convenience
}



#elif 0

size_t BertIndex::append(const std::string & sentence, int64_t sentence_id) {
    auto chunks = chunk_tokens(sentence);
    size_t last_label = 0;

    for (auto &chunk : chunks) {
        size_t label = allocate_label();

        // --- write chunk text to sentences file ---
        sfs.seekp(0, std::ios::end);
        int64_t file_start = (int64_t)sfs.tellp();
        sfs.write(chunk.text.data(), chunk.text.size());
        int64_t file_end = (int64_t)sfs.tellp();

        // --- build embedding and add to HNSW index ---
        std::vector<float> emb = embedder.embed(chunk.text);
        if (cfg.normalize) normalize_vector(emb);
        hnsw_index->addPoint(emb.data(), label);

        // --- now persist offset entry ---
        OffsetEntry e { sentence_id,
                        chunk.start_tok,
                        chunk.end_tok,
                        file_start,
                        file_end };
        offsets->set(label, e);

        if (cfg.debug) {
             LOG_DEBUG_S() << "append label=" << label
                      << " sid=" << sentence_id
                      << " tok=[" << chunk.start_tok << "," << chunk.end_tok << ")"
                      << " file=[" << file_start << "," << file_end << ")";
        }

        last_label = label;
        dirty_count++;
    }

    if (dirty_count >= cfg.flush_threshold) {
        flush();
    }

    return last_label; // return last label used
}


#else
size_t BertIndex::append(const string & sentence, int64_t sentence_id) {
    auto chunks = chunk_tokens(sentence);


    // Ensure offsets file exists
    fstream ofs(offsets_path, ios::in | ios::out | ios::binary);
    if (!ofs) {
        // create
        ofstream tmp(offsets_path, ios::binary | ios::app);
        tmp.close();
        ofs.open(offsets_path, ios::in | ios::out | ios::binary);
        if (!ofs) throw runtime_error("Failed to open/create offsets file: " + offsets_path);
    }

    ofstream sfs(sentences_path, ios::binary | ios::app);
    if (!sfs) throw runtime_error("Failed to open sentences file: " + sentences_path);

    size_t last_label = 0;
    for (auto &chunk : chunks) {
        size_t label = allocate_label(); // next_label++; // allocate label

        // write text chunk
        sfs.seekp(0, ios::end);
        int64_t start = (int64_t)sfs.tellp();
        sfs.write(chunk.text.data(), (streamsize)chunk.text.size());
//        sfs.put('\n');
        sfs.flush();
        int64_t end = (int64_t)sfs.tellp();

        // offsets: 16 bytes per label (portable write)
        ofs.seekp((streamoff)label * 16);
        write_int64(ofs, start);
        write_int64(ofs, end);
        ofs.flush();

        // embedding + add
        auto emb = encode_text(chunk.text);
        index->addPoint(emb.data(), (hnswlib::labeltype)label);

        // metadata
        chunk_token_map[label] = { chunk.start_token, chunk.end_token };
        chunk_sentence_map[label] = sentence_id;

        if (cfg.debug) {
             LOG_DEBUG_S() << "append label=" << label
                 << " sid=" << sentence_id
                 << " tok=[" << chunk.start_token << "," << chunk.end_token << ")"
                 << " file=[" << start << "," << end << ")";
        }

        last_label = label;
        ++dirty_count;
        if (cfg.flush_threshold >= 0 && dirty_count >= cfg.flush_threshold) flush();
    }

    return last_label;
}
#endif

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
    sentences_file.flush();
    // INCLUDE HERE CODE (WHEN IMPLEMENTED) to flush offsets
    if (cfg.debug)  LOG_DEBUG_S() <<  "Flushed index + offsets to disk";
  }
}

void BertIndex::save() {
#if HNSW_META
   std::ofstream ofs(index_path, std::ios::binary | std::ios::trunc);
   if (!ofs) throw std::runtime_error("Cannot open " + index_path + " for writing");

   IndexMeta meta;
   meta.version = 1;
   meta.metric = cfg.metric;
   meta.normalized = (cfg.metric == Metric::Cosine);  // or cfg.normalized_embeddings flag
   meta.dim = embedder.n_embd;
   meta.count = this->size();
   meta.save(ofs);
   index->saveIndex(ofs);
   ofs.close();
#else
    index->saveIndex(index_path);
#endif

    dirty_count = 0; // Memory = disk
    if (cfg.debug)  LOG_DEBUG_S() << "saved index " << index_path;
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

#if 1 /* NEW VERSION using Offsets */


/*
for (auto &[dist, label] : topk) {
    float score = dist;
    if (cfg.metric == "cosine" || cfg.metric == "ip" || cfg.metric == "inner_product")
        score = 1.0f - dist;  // convert from distance back to similarity
    ...
}
*/

template<typename FilterFn>
std::vector<SearchResult> BertIndex::filter_knn_results(const std::string &query,
                                                        size_t max_k,
                                                        FilterFn filter) {

    // std::cerr << "QUERY=" << query << std::endl;
    std::vector<float> emb = encode_text(query); // embed(query); // embed(query);
    auto candidates = index->searchKnnCloserFirst(emb.data(), max_k);

    std::vector<SearchResult> results;
    results.reserve(max_k);

    for (auto &[dist, label] : candidates) {
        float score = score_from_dist(dist);
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
//    	(cfg.metric == Metric::Cosine || cfg.metric ==  Metric::InnerProduct);

    std::sort(results.begin(), results.end(),
          [this](const SearchResult &a, const SearchResult &b) {
              switch( cfg.metric) {
		case Metric::Cosine:
		case Metric::InnerProduct:
                  return a.score > b.score;
		case Metric::L2:
                  return a.score < b.score;
	      }
	      return false; // Not defined case???
          });

    return results;
}



std::vector<SearchResult> BertIndex::knn(const std::string &query, size_t k) {
    if (k <= 0) k = cfg.default_k;
    return filter_knn_results(query, k, [](float) { return true; });
}

/*
std::vector<SearchResult> BertIndex::knn(const std::string &query, size_t k) {
    std::vector<float> emb = encode_text(query); // embed(query);

    // Depending on your HNSWlib, this returns a std::vector
    auto topk = index->searchKnnCloserFirst(emb.data(), k);

    std::vector<SearchResult> results;
    results.reserve(k);

    for (auto &[score, label] : topk) {
        OffsetEntry e = offsets->get(label);

        if (!is_valid_entry(e)) continue;

        SearchResult r;
        r.score      = score;
        r.label      = label;
        r.sentence_id= e.sid;
        r.token_start= e.start_tok;
        r.token_end  = e.end_tok;
        r.file_start = e.file_start;
        r.file_end   = e.file_end;
        r.text       = get_text_by_label(label);

        if (!r.text.empty()) {
            results.push_back(std::move(r));
        }
    }

// TODO: rework sort
    std::sort(results.begin(), results.end(),
              [](const SearchResult &a, const SearchResult &b) {
                  return a.score < b.score; // HNSWlib returns smaller distance = better
              });

    return results;
}
*/

#else

vector<SearchResult> BertIndex::knn(const string & query, size_t k) {
    if (k<=0) k = cfg.default_k;
    auto emb = encode_text(query);

    auto result = index->searchKnn(emb.data(), k);
    vector<SearchResult> out;
    while (!result.empty()) {
        auto pr = result.top(); result.pop();
        size_t label = pr.second;
        float score = (cfg.metric==Metric::Cosine) ? 1.0f - pr.first : -pr.first;

        ifstream sfs(sentences_path, ios::binary);
        fstream ofs(offsets_path, ios::in|ios::binary);
        ofs.seekg(label*16);
        int64_t s=read_int64(ofs), e=read_int64(ofs);
        string text(e-s, '\0');
        sfs.seekg(s); sfs.read(&text[0], e-s);

        auto ts=chunk_token_map[label].first;
        auto te=chunk_token_map[label].second;
        auto sid=chunk_sentence_map[label];

        out.push_back({score,s,e,label,ts,te,sid,text});
    }

    return out;
}
#endif

// radius, relative, adaptive are similar; stubbed for brevity

std::vector<SearchResult> BertIndex::radius(const std::string &query, float r) {
    if (r<0) r = cfg.default_radius;
    return filter_knn_results(query, cfg.max_elements, [r](float score) {
        return score <= r;
    });
}

/*
vector<SearchResult> BertIndex::radius(const string & query, float minScore) {
    if (minScore<0) minScore = cfg.default_radius;
    return knn(query, cfg.default_k*cfg.knn_lookahead_scale); // filter later
}
*/


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

/*
vector<SearchResult> BertIndex::relative(const string & query, float alpha) {
    if (alpha<0) alpha = cfg.default_alpha;
    return knn(query, cfg.default_k*cfg.knn_lookahead_scale);
}
*/


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


vector<SearchResult> BertIndex::adaptive(const string & query, float alpha,
                                         size_t minN, size_t lookahead, float gapDelta) {
    if (alpha<0) alpha=cfg.default_alpha;
    if (minN==0) minN=cfg.default_minN;
    if (lookahead==0) lookahead=cfg.default_lookahead;
    if (gapDelta<0) gapDelta=cfg.default_gapDelta;
    return knn(query, minN+lookahead);
}


*/

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

/*



std::string BertIndex::reconstruct_sentence(int64_t sid) {
    // collect all entries with this SID
    std::vector<std::pair<size_t, OffsetEntry>> entries;
    for (size_t label = 0; label < cfg.max_elements; ++label) {
        OffsetEntry e = offsets->get(label);
        if (e.sid == sid) {
            entries.emplace_back(label, e);
        }
    }

    if (entries.empty()) return "";

    std::sort(entries.begin(), entries.end(),
              [](auto &a, auto &b) { return a.second.start_tok < b.second.start_tok; });

    std::string result;
    size_t last_end = 0;

    for (auto &[label, e] : entries) {
        std::string chunk = get_text_by_label(label);

        if (result.empty()) {
            result = chunk;
        } else {
            // drop overlap: only append beyond last_end tokens
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

    return util::trim(result);
}
*/


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

/*

std::string BertIndex::get_text_by_label(size_t label) const {
    if (!offsets) {
        throw std::runtime_error("OffsetFile not initialized");
    }

    OffsetEntry e = offsets->get(label);
    if (e.sid == 0 && e.file_start == 0 && e.file_end == 0) {
        // empty slot
        return "";
    }

    std::ifstream ifs(sentences_path, std::ios::binary);
    if (!ifs) {
        throw std::runtime_error("Failed to open sentences file: " + sentences_path);
    }

    size_t len = e.file_end - e.file_start;
    std::string text(len, '\0');

    ifs.seekg(e.file_start, std::ios::beg);
    ifs.read(&text[0], len);

    return text;
}
*/

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


#if 0
std::string BertIndex::get_text(const SearchResult &r) const {
    if (r.sentence_id >= 0) {
        // Return reconstructed full sentence
        return reconstruct_sentence(r.sentence_id);
    } else {
        // Return raw chunk text
        return get_text_by_label(r.label);
    }
}

#else
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
#endif


#if 0
bool BertIndex::write_offsets(size_t label,
                              int64_t sid,
                              size_t start_tok,
                              size_t end_tok,
                              int64_t file_start,
                              int64_t file_end) {
    std::ofstream ofs(offsets_path, std::ios::binary | std::ios::app);
    if (!ofs) return false; //  throw std::runtime_error("Cannot open offsets file");

    ofs.write(reinterpret_cast<const char*>(&label), sizeof(label));
    ofs.write(reinterpret_cast<const char*>(&sid), sizeof(sid));
    ofs.write(reinterpret_cast<const char*>(&start_tok), sizeof(start_tok));
    ofs.write(reinterpret_cast<const char*>(&end_tok), sizeof(end_tok));
    ofs.write(reinterpret_cast<const char*>(&file_start), sizeof(file_start));
    ofs.write(reinterpret_cast<const char*>(&file_end), sizeof(file_end));

    ofs.close();

    // also update in-memory map
    label_to_entry[label] = { sid, start_tok, end_tok, file_start, file_end };
    return true;
}
#endif

#if 0
bool BertIndex::load_offsets() { 

int fd = open(offsets_path.c_str(), O_RDWR | O_CREAT, 0644);
if (fd < 0) return false; // throw std::runtime_error("open offsets file failed");

struct stat st;
fstat(fd, &st);
size_t filesize = st.st_size;

if (filesize < OffsetEntry.header_size) {
    // new file → write header
    const char magic[8] = "SBIDXv1";
    write(fd, magic, 8);
    uint64_t entry_size = sizeof(OffsetEntry);
    write(fd, &entry_size, sizeof(entry_size));
    filesize = header_size;
    ftruncate(fd, filesize);
}

offsets = (char*)mmap(nullptr, filesize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
if (offsets == MAP_FAILED) return false; // throw std::runtime_error("mmap offsets failed");


/*
size_t entry_size = sizeof(OffsetEntry);
char *base = offsets_map + header_size;
OffsetEntry *entry = (OffsetEntry*)(base + label * entry_size);

if (entry->sid != 0) { // assume 0 means empty
    r.sentence_id = entry->sid;
    r.start_tok   = entry->start_tok;
    r.end_tok     = entry->end_tok;
    r.file_start  = entry->file_start;
    r.file_end    = entry->file_end;
}

*/

}

#elif 0

bool BertIndex::load_offsets() {
    if (!file_exists(offsets_path)) return false;

    std::ifstream ifs(offsets_path, std::ios::binary);
    if (!ifs) return false;

    while (true) {
        size_t label;
        OffsetEntry e;

        ifs.read(reinterpret_cast<char*>(&label), sizeof(label));
        if (!ifs) break;

        ifs.read(reinterpret_cast<char*>(&e.sid), sizeof(e.sid));
        ifs.read(reinterpret_cast<char*>(&e.start_tok), sizeof(e.start_tok));
        ifs.read(reinterpret_cast<char*>(&e.end_tok), sizeof(e.end_tok));
        ifs.read(reinterpret_cast<char*>(&e.file_start), sizeof(e.file_start));
        ifs.read(reinterpret_cast<char*>(&e.file_end), sizeof(e.file_end));
        if (!ifs) break;

        label_to_entry[label] = e;
    }
  return true;
}
#endif

#if 1

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

#else

size_t BertIndex::allocate_label() {
    if (next_label >= cfg.max_elements) {
        throw std::runtime_error("allocate_label: HNSW capacity exceeded");
    }

    return next_label++;
}
#endif

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
