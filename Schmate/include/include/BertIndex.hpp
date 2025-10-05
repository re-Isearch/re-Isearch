#pragma once
#include "SBertGGML.hpp"
#include "HnswConfig.hpp"
#include "Util.hpp"
#include "hnswlib/hnswlib.h"

#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <fstream>
#include <iostream>
#include <atomic>
#include <algorithm>

struct SearchResult {
    float score;
    int64_t file_start;
    int64_t file_end;
    size_t label;
    int token_start;
    int token_end;
    int64_t sentence_id;
    std::string text; // optional
};

struct Chunk {
    std::string text;
    size_t start_token;
    size_t end_token;
};

// This structure helps up maintain the sentence IDs
// these are the GPs in re-Isearch
struct OffsetEntry {
    int64_t sid;         // persistent sentence ID
    size_t  start_tok;   // token start in original sentence
    size_t  end_tok;     // token end in original sentence
    int64_t file_start;  // byte offset in sentences file
    int64_t file_end;    // byte offset in sentences file
};




class BertIndex {
    SBertGGML & embedder;
    HnswConfig & cfg;
    std::unique_ptr<hnswlib::HierarchicalNSW<float>> index;
    std::unique_ptr<hnswlib::SpaceInterface<float>> space;

    std::string name;
    std::string sentences_path;
    std::string offsets_path;
    std::string index_path;

    size_t next_label = 0;
    std::unordered_map<size_t, std::pair<int,int>> chunk_token_map;
    std::unordered_map<size_t,int64_t> chunk_sentence_map;
    std::atomic<int64_t> auto_sentence_id{0};

    std::vector<Chunk> chunk_tokens(const std::string &sentence);

    // MAP to manage SIDs
    std::unordered_map<size_t, OffsetEntry> label_to_entry;

    size_t dirty_count = 0;

public:
    BertIndex(SBertGGML & emb, HnswConfig & c, const std::string & n);
    ~BertIndex();

#if 1

    // append returns the label assigned to the new chunk
    size_t append(const std::string & sentence);
    size_t append(const std::string & sentence, int64_t sentence_id);

    // removal
    void remove(size_t label);
    void undelete(size_t label);

    // accessors used by merge/reconstruct
    size_t label_count() const;               // how many labels were allocated (next_label)

#else
    void append(const std::string & sentence);
    void append(const std::string & sentence, int64_t sentence_id);

    void remove(size_t label);
    void undelete(size_t label);
#endif

    std::vector<SearchResult> knn(const std::string & query, size_t k=0);
    std::vector<SearchResult> radius(const std::string & query, float minScore=-1.0f);
    std::vector<SearchResult> relative(const std::string & query, float alpha=-1.0f);
    std::vector<SearchResult> adaptive(const std::string & query, float alpha=-1.0f,
                                       size_t minN=0, size_t lookahead=0, float gapDelta=-1.0f);

    std::string reconstruct_sentence(int64_t sentence_id) const;

    void flush();
    void save();
    void load();

    size_t size() const { return index ? (size_t) index->cur_element_count : 0; }

    std::string get_text_by_label(size_t label) const;
    std::string reconstruct_label(size_t label) const { return get_text_by_label(label);}

    int64_t get_sentence_id(size_t label) const;

    // Get text for a search result (chunk or sentence-level)
    // If full_sentence==true, always reconstruct the sentence.
    // Otherwise return chunk text (unless sentence_id is set).
    std::string get_text(const SearchResult &r, bool full_sentence=false) const;

private:
    bool write_offsets(size_t, int64_t, size_t, size_t, int64_t, int64_t) ;
    bool load_offsets();
};


