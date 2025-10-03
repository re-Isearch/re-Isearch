

#include "BertIndex.hpp"

using namespace std;

BertIndex::BertIndex(SBertGGML & emb, HnswConfig & c, const string & n)
: embedder(emb), cfg(c), name(n) {
    sentences_path = name + "_sentences.txt";
    offsets_path   = name + "_offsets.bin";

    switch (cfg.metric) {
        case Metric::L2:           space = make_unique<hnswlib::L2Space>(embedder.n_embd); break;
        case Metric::InnerProduct: space = make_unique<hnswlib::InnerProductSpace>(embedder.n_embd); break;
        case Metric::Cosine:       space = make_unique<hnswlib::InnerProductSpace>(embedder.n_embd); break;
    }

    index = make_unique<hnswlib::HierarchicalNSW<float>>(
        space.get(), cfg.max_elements, cfg.M, cfg.ef_construction);

    // Try to load offsets if they exist
    load();
}

// --- chunking ---
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

/// 

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

void BertIndex::flush() {
    save();
    dirty_count = 0;
}

void BertIndex::save() {
    string idx_path = name + "_index.bin";
    index->saveIndex(idx_path);
    if (cfg.debug) cerr << "[DEBUG] saved index " << idx_path << endl;
}

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


