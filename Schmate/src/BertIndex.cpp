#include "BertIndex.hpp"
#include "Util.hpp"

#include <sstream>

using namespace std;

static const char sentences_ext[] = ".txt";
static const char offsets_ext[]   = ".obn";
static const char index_ext[]     = ".hix";


BertIndex::BertIndex(SBertGGML & emb, HnswConfig & c, const string & n)
: embedder(emb), cfg(c), name(n) {
    sentences_path = name + sentences_ext;;
    offsets_path   = name + offsets_ext;
    index_path     = name + index_ext;;

      switch (cfg.metric) {
        case Metric::L2:           space = make_unique<hnswlib::L2Space>(embedder.n_embd); break;
        case Metric::InnerProduct: space = make_unique<hnswlib::InnerProductSpace>(embedder.n_embd); break;
        case Metric::Cosine:       space = make_unique<hnswlib::InnerProductSpace>(embedder.n_embd); break;
      }

#if 1
    // In BertIndex constructor
    if (file_exists(index_path)) {
        index = std::make_unique<hnswlib::HierarchicalNSW<float>>(
            space.get(), index_path, false   // <-- note: pass space.get()
        	);
        if (cfg.debug) {
            std::cerr << "[DEBUG] Loaded existing index: " << index_path << "\n";
        }
       next_label = index->cur_element_count;
    } else {
      // create new index if not found
       index = std::make_unique<hnswlib::HierarchicalNSW<float>>(
        space.get(), cfg.max_elements, cfg.M, cfg.ef_construction);
       if (cfg.debug) std::cerr << "[DEBUG] Created new index with capacity=" << cfg.max_elements << "\n";
    }

#else

    if (file_exists(index_path)) {
      index = std::make_unique<hnswlib::HierarchicalNSW<float>>(space.get(), index_path);
      if (cfg.debug) std::cerr << "[DEBUG] Loaded existing index " << index_path << "\n";
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
    load_offsets();
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
size_t BertIndex::append(const string & sentence) {
    int64_t sid = ++auto_sentence_id;
    return append(sentence, sid);
}

// Append with explicit sentence_id -> returns label
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
        size_t label = next_label++; // allocate label

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
        auto emb = embedder.encode_text(chunk.text, cfg.debug);
        index->addPoint(emb.data(), (hnswlib::labeltype)label);

        // metadata
        chunk_token_map[label] = { chunk.start_token, chunk.end_token };
        chunk_sentence_map[label] = sentence_id;

        if (cfg.debug) {
            cerr << "[DEBUG] append label=" << label
                 << " sid=" << sentence_id
                 << " tok=[" << chunk.start_token << "," << chunk.end_token << ")"
                 << " file=[" << start << "," << end << ")\n";
        }

        last_label = label;
        ++dirty_count;
        if (cfg.flush_threshold >= 0 && dirty_count >= cfg.flush_threshold) flush();
    }

    return last_label;
}

// remove / undelete (unchanged except usage)
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
        auto emb = embedder.encode_text(chunk.text, cfg.debug);
        index->addPoint(emb.data(), (hnswlib::labeltype)label);

        chunk_token_map[label] = {chunk.start_token, chunk.end_token};
        chunk_sentence_map[label] = sentence_id;

        if (cfg.debug) {
            cerr << "[DEBUG] append label=" << label
                 << " sid=" << sentence_id
                 << " tok=[" << chunk.start_token << "," << chunk.end_token << ")"
                 << " file=[" << start << "," << end << ")\n";
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
    // INCLUDE HERE CODE (WHEN IMPLEMENTED) to flush offsets
    if (cfg.debug) std::cerr << "[DEBUG] Flushed index + offsets to disk\n";
  }
}

void BertIndex::save() {
    index->saveIndex(index_path);
    dirty_count = 0; // Memory = disk
    if (cfg.debug) cerr << "[DEBUG] saved index " << index_path << endl;
}

/*
void BertIndex::load() {
    string idx_path = name + "_index.bin";
    ifstream f(idx_path);
    if (!f.good()) return;
    f.close();

    index = make_unique<hnswlib::HierarchicalNSW<float>>(space.get(), idx_path, false);
    next_label = index->cur_element_count;
    if (cfg.debug) cerr << "[DEBUG] loaded index " << idx_path
                        << " with " << next_label << " items\n";
}
*/


/// Search Methods

vector<SearchResult> BertIndex::knn(const string & query, size_t k) {
    if (k==0) k = cfg.default_k;
    auto emb = embedder.encode_text(query, cfg.debug);

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

// radius, relative, adaptive are similar; stubbed for brevity
vector<SearchResult> BertIndex::radius(const string & query, float minScore) {
    if (minScore<0) minScore = cfg.default_radius;
    return knn(query, cfg.default_k*cfg.knn_lookahead_scale); // filter later
}
vector<SearchResult> BertIndex::relative(const string & query, float alpha) {
    if (alpha<0) alpha = cfg.default_alpha;
    return knn(query, cfg.default_k*cfg.knn_lookahead_scale);
}
vector<SearchResult> BertIndex::adaptive(const string & query, float alpha,
                                         size_t minN, size_t lookahead, float gapDelta) {
    if (alpha<0) alpha=cfg.default_alpha;
    if (minN==0) minN=cfg.default_minN;
    if (lookahead==0) lookahead=cfg.default_lookahead;
    if (gapDelta<0) gapDelta=cfg.default_gapDelta;
    return knn(query, minN+lookahead);
}


///
// Reconstruct sentence

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

