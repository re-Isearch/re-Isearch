// main.cpp
// Sharded SBert (GGML) + HNSW search with kNN / radius / relative / adaptive
// Build on macOS: link with -lbert -lggml -framework Accelerate, include hnswlib headers

#define STANDALONE 1 /* run this standalone for testing */

// NOTE: Since this code depends upon a number of libs that use modern C++ we'll loosen
// our restrictions and embrace it here.
// The core engine will still compile and run using minimal compilers but the support of
// dense vectors will just demand a modern compiler. It probably makes no sense anyway
// to want to support embeddings on these platforms as they simply won't have the memory.

// This implementation supports shards. When the configured max_elements (see HnswConfig)
// is reached (the reserved capacity) of the HNSW we start a new shard.   We have also a
// number of methods to do search in parallel on these shards. The number of shards should
// probably be at most 2 but should be under the number of CPU cores.
// We also have a method to merge the last two shards: merge_last_two()
// that works by also expanding the max_elements accordingly.
// The method merge() keeps calling merge_last_two() as long as it can, effectively
// creating a single index.
//
// For best performance, depending upon memory and cores, one should effectively limit
// the number of shards on a single machine to a reasonable number and use multiple
// machines to distribute the load.


// How to specify what fields are to be handled as dense embeddings?
// In production:
// we have two logics: inclusion and exclusion.
//    inclusion: create X type indexes for fields so defined.
//    exclusion: create X type indexes for all fields except those defined
// This is addressed by
// [DbInfo]
// DefaultFieldType=<Fieldype to use when one is not defined>
// If this is defined then any field whose fieldtype has not been defined get this as
// its fieldtype.
// So to define HNSW as the default field type it would be set as the default.
// NOTE: All fields irrespective of the types get also indexed as text.
//
// See class FIELDTYPE in attrlist.hxx as well as the code in idbobj.hxx and doctype.cxx
// 

/*
 TODO:

Write the name of the model used for the embeddings in the offset file.
This is important since the HNSW index and embeeding search depend upon
using the same sBert model.

format:
<magic><int8 for length><name> 
magic is a byte: see src/magic.h for list we support 
name is written without the tailing \0 if length is even. This way
the offset is always 2 aligned:  2+strlen(name) + (strlen(name) % 2)
 
NOTE: We use name and not full path as path won't be portable to another
machine.
If in the future we discover problems or a possible adverserial attack is
not just theoretical we can extend the header with a 64-bit checksum.

*/

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <memory>
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <unordered_map>

#include "bert.h"
#include "hnswlib/hnswlib.h"

#ifndef MT
# define MT 1 /* compile Async search over shards methods */
#endif
#ifndef USE_FAST_BYTE_SWAP
# define USE_FAST_BYTE_SWAP 1
#endif


#if MT
# include <future>
# include <thread> /* needed only for std::thread::hardware_concurrency(); */
#endif

// These will probably change in the distribution
static const char sentences_ext[] = ".txt";
static const char offsets_ext[]   = ".bfc";
static const char index_ext[]     = ".hdx";
// NOTE: the sentence file is ONLY here for debuging. The offsets will later be GPs
// encoding an id to the file path and the offset addresses in the 64-bit integer..

static const float epsilon = std::numeric_limits<float>::epsilon() * 100.0f; // A common way to define epsilon

// --------------- Portability helpers ----------

// force little-endian storage

// Instead of
//   fin.read((char*)&s,8);
// we write
//   s = read_int64(fin);
// Instead of  
//   ofs.write((char*)&s,8);
// we write    
//   write_int64(ofs, s);


// Jam these into an unamed namespace
namespace {

/*
alternative (without normed read/write)

inline int64_t read_int64(std::istream &is) {
    uint64_t u = 0;
    fin.read((char*)&s,8);
    return u;
}
inline void  write_int64(std::ostream &os, int64_t u) {
    os.write((char*)&u,8);
}

*/
#if USE_FAST_BYTE_SWAP /* Fast but less readable code */

#if defined(_MSC_VER)
    #include <intrin.h>
    #define bswap64 _byteswap_uint64
#elif defined(__clang__) || defined(__GNUC__)
    #define bswap64 __builtin_bswap64
#else
    // fallback implementation
    inline uint64_t bswap64(uint64_t x) {
        return ((x & 0x00000000000000FFULL) << 56) |
               ((x & 0x000000000000FF00ULL) << 40) |
               ((x & 0x0000000000FF0000ULL) << 24) |
               ((x & 0x00000000FF000000ULL) <<  8) |
               ((x & 0x000000FF00000000ULL) >>  8) |
               ((x & 0x0000FF0000000000ULL) >> 24) |
               ((x & 0x00FF000000000000ULL) >> 40) |
               ((x & 0xFF00000000000000ULL) >> 56);
    }
#endif

inline uint64_t to_le64(uint64_t x) {
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return bswap64(x);
#else
    return x;
#endif
}

inline uint64_t from_le64(uint64_t x) {
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return bswap64(x);
#else
    return x;
#endif
}

// write int64_t in little-endian
inline void write_int64(std::ostream &os, int64_t v) {
    uint64_t u = to_le64(static_cast<uint64_t>(v));
    os.write(reinterpret_cast<const char*>(&u), sizeof(u));
}

// read int64_t from little-endian
inline int64_t read_int64(std::istream &is) {
    uint64_t u = 0;
    is.read(reinterpret_cast<char*>(&u), sizeof(u));
    return static_cast<int64_t>(from_le64(u));
}


#else


inline void write_int64(std::ostream &os, int64_t v) {
    uint64_t u = static_cast<uint64_t>(v);
    for (int i = 0; i < 8; i++) {
        char byte = static_cast<char>((u >> (i * 8)) & 0xFF); // little-endian
        os.put(byte);
    }
}

inline int64_t read_int64(std::istream &is) {
    uint64_t u = 0;
    for (int i = 0; i < 8; i++) {
        int c = is.get();
        if (c == EOF) throw std::runtime_error("Unexpected EOF while reading int64");
        u |= (static_cast<uint64_t>(c) & 0xFF) << (i * 8);
    }
    return static_cast<int64_t>(u);
}

#endif

inline void  write_int128(std::ostream &os, int64_t s, int64_t e) {
    write_int64(os, s);
    write_int64(os, e);
}


inline void offsets_append(std::ostream &os, size_t label, int64_t start, int64_t end) {
  os.seekp((std::streamoff)label * 2*sizeof(int64_t) /* 16 */);
  write_int128(os, start, end);
}

inline void write_zero128 (std::ostream &os) { write_int128(os, 0, 0); }

inline void write_zero128( std::ostream &os, size_t label) {
  offsets_append(os, label, 0, 0);
}


} // end unnamed namespace


// ---------------- SBert wrapper ----------------
class SBertGGML {
    bert_ctx * ctx;
    int dim;
    int max_tokens;
public:
    SBertGGML(const std::string& model_path) {
        ctx = bert_load_from_file(model_path.c_str());
        if (!ctx) throw std::runtime_error("Failed to load model");
        dim = bert_n_embd(ctx);
        max_tokens = bert_n_max_tokens(ctx);
#if STANDALONE
        std::cout << "Loaded SBERT GGML model. dim=" << dim << " max_tokens=" << max_tokens << "\n";
#else
	message_log (LOG_INFO, "Loaded SBERT GGML model '%s'. dim=%d max_tokens=%d",
		model_path.c_str(), dim, max_tokens);
#endif
    }
    ~SBertGGML(){ if(ctx) bert_free(ctx); }
    int embedding_dim() const { return dim; }
    int embedding_capacity() const { return max_tokens; }
    bert_ctx* raw() const { return ctx; }

    std::vector<float> encode_text(const std::string &text, bool debug=false) const {
#if 0
        const int MAX_TOKENS = 512;
        bert_vocab_id tokens[MAX_TOKENS];
#else
        bert_vocab_id tokens[max_tokens];
#endif
        int32_t n_tokens = 0;
        bert_tokenize(ctx, text.c_str(), tokens, &n_tokens, max_tokens);
        if (n_tokens <= 0) throw std::runtime_error("Tokenization failed");

        std::vector<float> emb((size_t)dim);
        bert_eval(ctx, 4, tokens, n_tokens, emb.data());

        double norm = 0.0;
        for (float v : emb) norm += (double)v * (double)v;
        norm = std::sqrt(norm);
        if (norm > 0.0) for (auto &v : emb) v = (float)(v / (float)norm);

        if (debug) std::cerr << "[DEBUG] encode_text(): n_tokens="<<n_tokens<<" norm="<<norm<<"\n";
        return emb;
    }
};

// ---------------- SearchResult ----------------

struct SearchResult {
    float score;
    int64_t start;
    int64_t end;
};

// ---------------- Metric + config ----------------
enum class Metric { L2, InnerProduct, Cosine };

struct HnswConfig {
    std::string default_field = "default";
    std::string model =  "sbert.ggml";
    size_t max_elements = 100000;
    size_t M = 16;
    size_t ef_construction = 200;
    size_t ef_search = 50;
    Metric metric = Metric::L2;
    //
    int max_tokens_per_chunk = 128; // Needs to be less than the max_tokens
    float overlap_percent = 0.1f;
    size_t knn_lookahead_scale = 5;
    //
    float  alpha = 0.8; // relative threshold
    size_t minN = 3; // always return at least 3
    float  gapDelta = 0.1; // treat a 0.1 gap as significant
    size_t adaptive_lookahead = 10 ; // check top 10 results for cluster drop-off
    //
    size_t relative_k = 500; // kNN for relative
    //
    size_t flush_threshold = 100;  // auto-flush after this many inserts/deletes
    // 
    // Use async search on shards when (3* number of cores/2) > number of shards. 
    // ideally we want 1 thread perhaps 2 per core. Here we specify 1.5 times.
    // Given how most machines these days have at least 2 or more cores and 3 shards
    // would already be pushing things on such a machine...
    bool search_async = true; // Only has a function when MT was defined true during compile
    unsigned int processor_count = 1; // This gets filled with the hint by OS

    //
    bool debug = false;
};

// ---------------- Utility ----------------
#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
# include <filesystem>
#elif defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))
# include <unistd.h>
#endif
namespace {

inline bool file_exists(const std::string &p) {
#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
    return std::filesystem::exists(p); // At least C++16
#elif defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))
    return access(p.c_str(), F_OK) != -1;
#else
    std::ifstream f(p); return f.good();
#endif
}

/*
std::vector<size_t> findPositionsInRange(const std::string& filename, int64_t x) {
    std::vector<size_t> positions;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return positions;
    }
    
    int64_t first, second;
    size_t position = 0;
    
    while (file >> first >> second) {
        // Check if x is between first and second (inclusive)
        if ((first <= x && x <= second) || (second <= x && x <= first)) {
            positions.push_back(position);
        }
        position++;
    }
    
    file.close();
    return positions;
}
*/

} // end unamed namespace

// ---------------- BertIndex (single shard) ----------------
class BertIndex {
    SBertGGML &embedder;
    std::unique_ptr<hnswlib::SpaceInterface<float>> space;
    std::unique_ptr<hnswlib::HierarchicalNSW<float>> index;
    std::string sentences_path;
    std::string index_path;
    std::string offsets_path;
    HnswConfig cfg;
    size_t next_label;
    std::vector<size_t> free_labels;
    size_t dirty_count = 0;

public:
    BertIndex(SBertGGML &eb, const std::string &sentences, const std::string &idx, const std::string &offs, const HnswConfig &conf)
        : embedder(eb), sentences_path(sentences), index_path(idx), offsets_path(offs), cfg(conf), next_label(0)
    {
        int dim = embedder.embedding_dim();
        if (cfg.metric == Metric::L2) {
            space = std::make_unique<hnswlib::L2Space>(dim);
        } else {
            space = std::make_unique<hnswlib::InnerProductSpace>(dim);
        }
        // Need to make sure that the configured max chunk is less than max_tokens
        // anything greater would be truncated if we did not chunk it to at most max_tokens!
        int max_tokens = embedder.embedding_capacity(); 
        if (cfg.max_tokens_per_chunk > max_tokens) cfg.max_tokens_per_chunk = max_tokens;
    }

    ~BertIndex() {
        try {
            flush();  // ensure final save
        } catch (...) {
            // don’t throw in destructor, just log
            std::cerr << "[WARN] Failed to save index on destruction\n";
        }
    }

    // Helper methods for merging shards
    // Return current size (max label used so far)
    size_t size() const { return next_label; }

    // Expose dimension and metric space for rebuilds
    hnswlib::SpaceInterface<float>* getSpace() const { return space.get(); }

    // Replace the HNSW index with a new one
    void replaceIndex(std::unique_ptr<hnswlib::HierarchicalNSW<float>> newIdx) {
       index = std::move(newIdx);
       dirty_count++; // mark dirty since index changed
   }

   // Expose paths so ShardedIndex can clean up files
   const std::string& get_sentences_path() const { return sentences_path; }
   const std::string& get_offsets_path()   const { return offsets_path; }
   const std::string& get_index_path()     const { return index_path; }

   //


    // sync the in-memory HNSW index to disk.
   void flush() {
        // We only re-write if the index on disk and in-memory are different
        if (dirty_count && index) index->saveIndex(index_path);
	dirty_count = 0;
    }

    void buildIfMissing() {
        if (file_exists(index_path) && file_exists(offsets_path)) {
            index = std::make_unique<hnswlib::HierarchicalNSW<float>>(space.get(), index_path);
            index->setEf((uint32_t)cfg.ef_search);
            std::ifstream fin(offsets_path, std::ios::binary | std::ios::ate);
            size_t fsize = (size_t)fin.tellg();
            size_t entries = fsize / 16;
            fin.seekg(0);
            for (size_t i = 0; i < entries; ++i) {
                int64_t s = read_int64(fin);
                int64_t e = read_int64(fin);
                if (s==0 && e==0) free_labels.push_back(i);
            }
            next_label = entries;
        } else {
            createEmpty();
        }
    }

    void createEmpty() {
        index = std::make_unique<hnswlib::HierarchicalNSW<float>>(space.get(), cfg.max_elements, cfg.M, cfg.ef_construction);
        index->setEf((uint32_t)cfg.ef_search);
        { std::ofstream ofs(offsets_path, std::ios::binary | std::ios::trunc); }
        { std::ofstream sfs(sentences_path, std::ios::trunc); }
        next_label = 0;
        free_labels.clear();
        dirty_count = 0;
    }

    size_t allocate_label() {
        if (!free_labels.empty()) {
            size_t id = free_labels.back();
            free_labels.pop_back();
            // NOTE: We may want to set a flag to flush() after the new label is
            // used. We don't do this yet but may..
            return id;
        }
        if (next_label >= cfg.max_elements) throw std::runtime_error("Reached max_elements");
        return next_label++;
    }

    std::vector<std::vector<bert_vocab_id>> chunk_tokens(const std::string &sentence) const {
        const int MAX_TOKENS = embedder.embedding_capacity();
//      const int MAX_TOKENS = 512; // BERT is designed for typically 512 
        bert_vocab_id tokens[MAX_TOKENS];
        int32_t n_tokens = 0;
        bert_tokenize(embedder.raw(), sentence.c_str(), tokens, &n_tokens, MAX_TOKENS);

        if (n_tokens <= cfg.max_tokens_per_chunk)
            return { std::vector<bert_vocab_id>(tokens, tokens+n_tokens) };

        int stride = std::max(1, (int)(cfg.max_tokens_per_chunk * (1.0f - cfg.overlap_percent)));
        std::vector<std::vector<bert_vocab_id>> chunks;
        for (int i=0; i<n_tokens; i+=stride) {
            int end = std::min(i+cfg.max_tokens_per_chunk, n_tokens);
            chunks.emplace_back(tokens+i, tokens+end);
            if (end == n_tokens) break;
        }
        return chunks;
    }


#if 0 /* PRODUCTION */


    // For production we don't have a sentences file and the start, end are
    // re-Isearch addresses
    void append(const std::string &sentence, int64_t s, int64_t e) {
        auto chunks = chunk_tokens(sentence);
        int64_t start = s;
        int64_t end   = e;
 	size_t  off;

        std::fstream ofs(offsets_path, std::ios::in | std::ios::out | std::ios::binary);
        if (!ofs) {
            ofs.open(offsets_path, std::ios::out | std::ios::binary); // create if missing
        }

        for (auto &chunk : chunks) {
            size_t label = allocate_label();

	    off = 0;
            // sentence append
            std::string chunk_text(chunk.size(), '\0');
            for (size_t i=0;i<chunk.size();i++) {
                if (chunk_text[i] = (char)chunk[i]) != '\0'); // <-- if you’re writing raw tokens
		  off++;
            }
	    if (start + off < e) end = start + off; 
	    else end = e;

            // offsets append
	    offsets_append(ofs, label, start, end);

	    start = end + 1 - std::max(0, (int)(cfg.max_tokens_per_chunk * (1.0f - cfg.overlap_percent)));

            // embedding
            std::vector<float> emb(embedder.embedding_dim());
            bert_eval(embedder.raw(), 4, chunk.data(), (int32_t)chunk.size(), emb.data());

            double norm=0; for (float v:emb) norm+=v*v; norm=std::sqrt(norm);
            if (norm>0) for (auto &v:emb) v/=norm;

            index->addPoint(emb.data(), (hnswlib::labeltype)label);

            dirty_count++;
            if (dirty_count >= cfg.flush_threshold) {
                flush();
            }
        }
        ofs.close();
    }
#endif
#if 1

    void append(const std::string &sentence) {
        auto chunks = chunk_tokens(sentence);

        std::ofstream sfs(sentences_path, std::ios::binary | std::ios::app);
        std::fstream ofs(offsets_path, std::ios::in | std::ios::out | std::ios::binary);
        if (!ofs) {
            ofs.open(offsets_path, std::ios::out | std::ios::binary); // create if missing
        }

        for (auto &chunk : chunks) {
            size_t label = allocate_label();

            // sentence append
            sfs.seekp(0,std::ios::end);
            int64_t start = (int64_t)sfs.tellp();
            std::string chunk_text(chunk.size(), '\0');
            for (size_t i=0;i<chunk.size();i++) {
                chunk_text[i] = (char)chunk[i]; // <-- if you’re writing raw tokens
            }
            sfs.write(chunk_text.data(), chunk_text.size());
            sfs.put('\n');
            int64_t end = (int64_t)sfs.tellp();

            // offsets append
            ofs.seekp((std::streamoff)label * 16);
            write_int64(ofs, start);
            write_int64(ofs, end);

            // embedding
            std::vector<float> emb(embedder.embedding_dim());
            bert_eval(embedder.raw(), 4, chunk.data(), (int32_t)chunk.size(), emb.data());

            double norm=0; for (float v:emb) norm+=v*v; norm=std::sqrt(norm);
            if (norm>0) for (auto &v:emb) v/=norm;

            index->addPoint(emb.data(), (hnswlib::labeltype)label);

            dirty_count++;
            if (dirty_count >= cfg.flush_threshold) {
                flush();
            }
        }
        ofs.close();
        sfs.close();
    }

#else

    void append(const std::string &sentence) {
        auto chunks = chunk_tokens(sentence);
        for (auto &chunk : chunks) {
            size_t label = allocate_label();

#if 1
            std::ofstream sfs(sentences_path, std::ios::binary | std::ios::app);
#else
            std::fstream sfs(sentences_path, std::ios::in | std::ios::out | std::ios::binary | std::ios::app);
#endif
            sfs.seekp(0, std::ios::end);
            int64_t start = (int64_t)sfs.tellp();
            sfs.write(sentence.data(), sentence.size()); sfs.put('\n');
            int64_t end = (int64_t)sfs.tellp();
            sfs.close();

            std::fstream ofs(offsets_path, std::ios::in | std::ios::out | std::ios::binary);
            if (!ofs) ofs.open(offsets_path, std::ios::out | std::ios::binary);
            ofs.seekp((std::streamoff)label * 16);
            write_int64(ofs, start);
            write_int64(ofs, end);
            ofs.close();

            std::vector<float> emb(embedder.embedding_dim());
            bert_eval(embedder.raw(), 4, chunk.data(), (int32_t)chunk.size(), emb.data());

            double norm=0; for (float v:emb) norm+=v*v; norm=std::sqrt(norm);
            if (norm>0) for (auto &v:emb) v/=norm;

            index->addPoint(emb.data(), (hnswlib::labeltype)label);
            if (++dirty_count > cfg.flush_threshold) flush();
        }
        // save(); // we defer this as its too expensve!
    }
#endif

#if 1

    // Mark deleted in the graph
    void markDelete(size_t label) {
        if (!index) throw std::runtime_error("Index not initialized (delete)");
        index->markDelete((hnswlib::labeltype)label);
        dirty_count++;
    }

    // Remove the label from the graph and from the offsets.
    void remove(size_t label) {
        if (!index) throw std::runtime_error("Index not initialized (remove)");
        index->markDelete((hnswlib::labeltype)label);
        // zero offsets
        std::fstream ofs(offsets_path, std::ios::in | std::ios::out | std::ios::binary);
        ofs.seekp((std::streamoff)label * 16);
        write_int64(ofs, 0);
        write_int64(ofs, 0);
        ofs.close();
        free_labels.push_back(label);
        dirty_count++;

        // We don't flush even if count as we want to defer
        // if (dirty_count >= cfg.flush_threshold) flush();
    }

    // Can undelete but not unremove since the <start,end> was already zapped
    void undelete(size_t label) {
        if (!index) throw std::runtime_error("Index not initialized");
        index->unmarkDelete((hnswlib::labeltype)label);
        dirty_count++;
        // if (dirty_count >= cfg.flush_threshold) flush();
    }

   // Return the labels that have start,end range that contain address
    std::vector<size_t> labels_byAddress(int64_t address) {
        std::vector<size_t> matches;
        std::ifstream fin(offsets_path, std::ios::binary);
        fin.seekg(0, std::ios::end);
        size_t count = fin.tellg() / 16;
        fin.seekg(0);
        for (size_t i=0; i<count; i++) {
            int64_t s = read_int64(fin);
            int64_t e = read_int64(fin);
            if (address >= s && address < e) {
                matches.push_back(i);
            }
        }
        return matches;
    }

    // Return the labels that are included in the range <start,end>
    std::vector<size_t> labels_byAddress(int64_t start, int64_t end) {
        std::vector<size_t> matches;
        std::ifstream fin(offsets_path, std::ios::binary);
        fin.seekg(0, std::ios::end);
        size_t count = fin.tellg() / 16;
        fin.seekg(0);
        for (size_t i=0; i<count; i++) {
            int64_t s = read_int64(fin);
            int64_t e = read_int64(fin);
            if (s >= start && e <= end) {
                matches.push_back(i);
            }
        }
        return matches;
    }


    // Remove by address. Need to go through all the offsets.
    void remove_byAddress(int64_t gp) {
        auto matches = labels_byAddress(gp);
        for (auto lbl : matches) remove(lbl);
    }

    void delete_byAddress(int64_t gp) {
        auto matches = labels_byAddress(gp);
        for (auto lbl : matches) markDelete(lbl);
    }


    // Can undelete but can't unremove
    void undelete_byAddress(int64_t gp) {
        auto matches = labels_byAddress(gp);
        for (auto lbl : matches) undelete(lbl);
    }

#else

    void remove(size_t label) {
        if (!index) throw std::runtime_error("Index not initialized");

        // Mark as deleted in HNSWlib
        index->markDelete((hnswlib::labeltype)label);

        // Zero out offsets on disk
        std::fstream ofs(offsets_path, std::ios::in | std::ios::out | std::ios::binary);
        if (!ofs) throw std::runtime_error("Failed to open offsets file for remove()");
        ofs.seekp((std::streamoff)label * 16);
        write_int64(ofs, 0);
        write_int64(ofs, 0);
        ofs.close();

        // Make label reusable
        free_labels.push_back(label);

        dirty_count++;
#if 0 /* since we have 0,0 in the offset file and the search leaves it off we can defer */
        if (dirty_count >= cfg.flush_threshold) flush();
#endif
    }
#endif

    float score_from_dist(float dist) const {
        if (cfg.metric == Metric::L2) return 1.0f/(1.0f+dist);
        return 1.0f - dist;
    }

    std::vector<SearchResult> search_knn(const std::vector<float> &qemb, size_t k) const {
        std::vector<SearchResult> out;
        int look = (int)(k * cfg.knn_lookahead_scale);
        auto pq = index->searchKnn(qemb.data(), look);
        std::vector<std::pair<float,hnswlib::labeltype>> pairs;
        while (!pq.empty()) { pairs.push_back(pq.top()); pq.pop(); }
        std::sort(pairs.begin(), pairs.end(), [](auto&a,auto&b){return a.first<b.first;});
        std::ifstream fin(offsets_path, std::ios::binary);
        for (auto &pr:pairs) {
            fin.seekg((std::streamoff)pr.second*16);
            const int64_t s = read_int64(fin);
            const int64_t e = read_int64(fin);
            if (s==0 && e==0) continue;
            out.push_back({score_from_dist(pr.first), s, e});
            if (out.size()>=k) break;
        }
        return out;
    }

    std::vector<SearchResult> search_radius(const std::vector<float>&qemb,float minScore,size_t maxK=1000) const {
        auto cands = search_knn(qemb,maxK);
        std::vector<SearchResult> out;
        for(auto&r:cands) if(r.score>=minScore) out.push_back(r);
        return out;
    }

    std::vector<SearchResult> search_relative(const std::vector<float>&q,float alpha = 0.0f){
        auto res=search_knn(q, cfg.relative_k);
        if(res.empty())return{};
        float my_alpha = std::abs(alpha) > epsilon ? alpha : cfg.alpha;
        float cutoff=my_alpha*res.front().score;
        std::vector<SearchResult>out;
        for(auto&r:res)if(r.score>=cutoff)out.push_back(r);
        return out;
    }

    std::vector<SearchResult> search_adaptive(const std::vector<float>&q,float alpha = 0.0f,
		size_t minN =0,size_t lookahead=0,float gapDelta = 0.0f){
        const float my_alpha = std::abs(alpha) > epsilon ? alpha : cfg.alpha;
        const float my_gap = std::abs(gapDelta) > epsilon ? gapDelta : cfg.gapDelta;
        const size_t my_lookahead = lookahead ? lookahead : cfg.adaptive_lookahead;
        const size_t my_minN =  minN ? minN : cfg.minN;

        auto res=search_relative(q,my_alpha);
        if(res.empty())return{};
        size_t stop=std::min(my_minN,res.size());
        for(size_t i=1;i<std::min(my_lookahead,res.size());i++){
            float gap=res[i-1].score-res[i].score;
            if(gap>=my_gap){stop=std::max(my_minN,i);break;}
            stop=std::max(stop,i+1);
        }
        if(stop>res.size())stop=res.size();
        return{res.begin(),res.begin()+stop};
    }


    void save() { flush(); /* We may also want to do something else here as well */ }

    std::string get_text(const SearchResult&r) const {
        std::ifstream f(sentences_path);
        f.seekg(r.start);
        std::string s((size_t)(r.end-r.start),'\0');
        f.read(&s[0], s.size());
        return s;
    }
};

// ---------------- ShardedIndex ----------------
class ShardedIndex {
    SBertGGML &embedder;
    std::string base;
    HnswConfig cfg;
    std::vector<std::unique_ptr<BertIndex>> shards;
    std::string basename (size_t i)const{ return  i ? base+"_"+std::to_string(i) : base;}
    std::string name_sent(size_t i)const{ return basename(i)+sentences_ext;}
    std::string name_idx(size_t i)const{  return basename(i)+index_ext;}
    std::string name_offs(size_t i)const{ return basename(i)+offsets_ext;}
public:
    ShardedIndex(SBertGGML &e,const std::string &b,HnswConfig c):embedder(e),base(b),cfg(c){
        size_t i=0;
#if MT
        // default is at least one CPU/Core.
        if (cfg.processor_count <= 1) cfg.processor_count = std::thread::hardware_concurrency();
#if STANDALONE
        if (cfg.debug) std::cout << "Cores: " << cfg.processor_count << "\n"; 
#endif
#endif
        while(file_exists(name_idx(i))&&file_exists(name_offs(i))){
            auto s=std::make_unique<BertIndex>(embedder,name_sent(i),name_idx(i),name_offs(i),cfg);
            s->buildIfMissing();
            shards.push_back(std::move(s));
            i++;
        }
        if(shards.empty()) add_new();
    }
    void add_new(){
        size_t i=shards.size();
        auto s=std::make_unique<BertIndex>(embedder,name_sent(i),name_idx(i),name_offs(i),cfg);
        s->buildIfMissing();
        shards.push_back(std::move(s));
    }
    void append(const std::string&txt){
        try{shards.back()->append(txt);}
        catch(std::runtime_error&e){
            if(std::string(e.what()).find("max_elements")!=std::string::npos){
                add_new(); shards.back()->append(txt);
            } else throw;
        }
    }

#if 1

    void remove(size_t label, size_t shard=0) {
        if (shard >= shards.size()) throw std::runtime_error("Invalid shard index");
        shards[shard]->remove(label);
    }

    void markDelete(size_t label, size_t shard=0) {
        if (shard >= shards.size()) throw std::runtime_error("Invalid shard index");
        shards[shard]->markDelete(label);
    }

    void undelete(size_t label, size_t shard=0) {
        if (shard >= shards.size()) throw std::runtime_error("Invalid shard index");
        shards[shard]->undelete(label);
    }

    void delete_byAddress(int64_t address, size_t shard=0) {
        if (shard >= shards.size()) throw std::runtime_error("Invalid shard index");
        auto matches = shards[shard]->labels_byAddress(address);
        for (auto lbl : matches) shards[shard]->markDelete(lbl);
    }

    void remove_byAddress(int64_t address, size_t shard=0) {
        if (shard >= shards.size()) throw std::runtime_error("Invalid shard index");
        auto matches = shards[shard]->labels_byAddress(address);
        for (auto lbl : matches) shards[shard]->remove(lbl);
    }     

    void undelete_byAddress(int64_t address, size_t shard=0) {
        if (shard >= shards.size()) throw std::runtime_error("Invalid shard index");
        auto matches = shards[shard]->labels_byAddress(address);
        for (auto lbl : matches) shards[shard]->undelete(lbl);
    }

    size_t shard_count() const { return shards.size(); }

#endif

   void remove(size_t label){
        // for simplicity assume single shard for label mapping
        if(!shards.empty()) shards.back()->remove(label);
    }
    void flush() { for (auto &s : shards) { s->flush(); } }

#ifdef MT

/*
   std::vector<SearchResult> parallel_search(const std::string &query, size_t k) {
    std::vector<std::future<std::vector<SearchResult>>> futures;

    for (auto &shard : shards) {
        futures.push_back(std::async(std::launch::async, [&]() {
            return shard->knn(query, k); // per-shard search
        }));
    }

    std::vector<SearchResult> all;
    for (auto &f : futures) {
        auto partial = f.get();
        all.insert(all.end(), partial.begin(), partial.end());
    }

    // merge top-k results globally
    std::partial_sort(all.begin(), all.begin()+std::min(k,all.size()), all.end(),
                      [](auto &a, auto &b){ return a.score > b.score; });
    if (all.size() > k) all.resize(k);
    return all;
    }
*/

    template <typename SearchFunc> std::vector<SearchResult> parallel_search(SearchFunc search_fn, size_t topN = 0) {
        std::vector<std::future<std::vector<SearchResult>>> futures;

        for (auto &shard : shards) {
            futures.push_back(std::async(std::launch::async, [&]() {
                return search_fn(*shard);
            }));
        }

        std::vector<SearchResult> all;
        for (auto &f : futures) {
            auto partial = f.get();
            all.insert(all.end(), partial.begin(), partial.end());
        }

        if (topN > 0 && all.size() > topN) {
            std::partial_sort(all.begin(), all.begin() + topN, all.end(),
                          [](const SearchResult &a, const SearchResult &b) {
                              return a.score > b.score;
                          });
            all.resize(topN);
        } else {
            std::sort(all.begin(), all.end(),
                  [](const SearchResult &a, const SearchResult &b) {
                      return a.score > b.score;
                  });
        }

        return all;
    }


    // knn in a lamda
    std::vector<SearchResult> parallel_knn(const std::string &q, size_t k) {
        auto qemb=embedder.encode_text(q,cfg.debug);
        return parallel_search( [&](BertIndex &shard) { return shard.search_knn(qemb, k);  /* per-shard logic */ },
        k  /* global top-k */);
    }

    std::vector<SearchResult> parallel_radius(const std::string &q, float minScore) {
        auto qemb=embedder.encode_text(q,cfg.debug);
        return parallel_search( [&](BertIndex &shard) { return shard.search_radius(qemb, minScore); }) ;
    }

    std::vector<SearchResult> parallel_relative(const std::string&q,float alpha = 0.0f){
        auto qemb=embedder.encode_text(q,cfg.debug);
        float my_alpha = std::abs(alpha) > epsilon ? alpha : cfg.alpha;
        return parallel_search( [&](BertIndex &shard) { return shard.search_relative(qemb, my_alpha); }) ;
    }

    std::vector<SearchResult> parallel_adaptive(const std::string&q,
		float alpha = 0.0f,size_t minN =0,size_t lookahead=0,float gapDelta = 0.0f){
        const float my_alpha = std::abs(alpha) > epsilon ? alpha : cfg.alpha;
        const float my_gap = std::abs(gapDelta) > epsilon ? gapDelta : cfg.gapDelta;
        const size_t my_lookahead = lookahead ? lookahead : cfg.adaptive_lookahead;
        const size_t my_minN =  minN ? minN : cfg.minN;
        auto qemb=embedder.encode_text(q,cfg.debug);

        return parallel_search( [&](BertIndex &shard) {
		return shard.search_adaptive(qemb, my_alpha, my_minN, my_lookahead, my_gap); }) ;
    }
#endif

    std::vector<SearchResult> knn(const std::string&q,size_t k){
        auto qemb=embedder.encode_text(q,cfg.debug);
        std::vector<SearchResult>all;
        for(auto&s:shards){auto part=s->search_knn(qemb,k);all.insert(all.end(),part.begin(),part.end());}
        std::sort(all.begin(),all.end(),[](auto&a,auto&b){return a.score>b.score;});
        if(all.size()>k)all.resize(k);return all;
    }
    std::vector<SearchResult> radius(const std::string&q,float minScore){
        auto qemb=embedder.encode_text(q,cfg.debug);
        std::vector<SearchResult>all;
        for(auto&s:shards){auto part=s->search_radius(qemb,minScore);all.insert(all.end(),part.begin(),part.end());}
        std::sort(all.begin(),all.end(),[](auto&a,auto&b){return a.score>b.score;});
        return all;
    }

    std::vector<SearchResult> relative(const std::string&q,float alpha = 0.0f){
        auto res=knn(q, cfg.relative_k);
        if(res.empty())return{};
        float my_alpha = std::abs(alpha) > epsilon ? alpha : cfg.alpha;
        float cutoff=my_alpha*res.front().score;
        std::vector<SearchResult>out;
        for(auto&r:res)if(r.score>=cutoff)out.push_back(r);
        return out;
    }

    std::vector<SearchResult> adaptive(const std::string&q,float alpha = 0.0f,size_t minN =0,size_t lookahead=0,float gapDelta = 0.0f){
        const float my_alpha = std::abs(alpha) > epsilon ? alpha : cfg.alpha;
        const float my_gap = std::abs(gapDelta) > epsilon ? gapDelta : cfg.gapDelta;
        const size_t my_lookahead = lookahead ? lookahead : cfg.adaptive_lookahead;
        const size_t my_minN =  minN ? minN : cfg.minN;

        auto res=relative(q,my_alpha);
        if(res.empty())return{};
        size_t stop=std::min(my_minN,res.size());
        for(size_t i=1;i<std::min(my_lookahead,res.size());i++){
            float gap=res[i-1].score-res[i].score;
            if(gap>=my_gap){stop=std::max(my_minN,i);break;}
            stop=std::max(stop,i+1);
        }
        if(stop>res.size())stop=res.size();
        return{res.begin(),res.begin()+stop};
    }


    std::string get_text(const SearchResult&r){for(auto&s:shards){auto t=s->get_text(r);if(!t.empty())return t;}return"";}

    // We want to be able to merge the last two shards. This is maybe useful when the second shard
    // is relatively small and there is probably sufficient memory (and we're using a fast SSD for
    // swap) 
    bool merge_last_two() {
        if (shards.size() < 2) {
            return false; // throw std::runtime_error("Not enough shards to merge");
        }

        size_t i1 = shards.size() - 2;
        size_t i2 = shards.size() - 1;

        auto &shard1 = shards[i1];
        auto &shard2 = shards[i2];

        // new capacity = combined size
        size_t total_cap = shard1->size() + shard2->size();

        auto newIndex = std::make_unique<hnswlib::HierarchicalNSW<float>>(
		shard1->getSpace(), total_cap, cfg.M, cfg.ef_construction);

        // get base offset for sentences
        std::ifstream sfs1_in(shard1->get_sentences_path(), std::ios::binary | std::ios::ate);
        int64_t base_offset = (int64_t)sfs1_in.tellg();
        sfs1_in.close();

        std::ofstream sfs1_append(shard1->get_sentences_path(), std::ios::app | std::ios::binary);
        std::fstream ofs1(shard1->get_offsets_path(), std::ios::in | std::ios::out | std::ios::binary);

        std::ifstream sfs2(shard2->get_sentences_path(), std::ios::binary);
        std::ifstream ofs2(shard2->get_offsets_path(), std::ios::binary);

        size_t idx = 0;
        while (ofs2.peek() != EOF) {
            int64_t start=0, end=0;
            ofs2.read((char*)&start,8);
            ofs2.read((char*)&end,8);

            if (!ofs2) break;

            if (start == 0 && end == 0) {
                // allocate but mark deleted
                size_t label = shard1->allocate_label();
                std::vector<float> zero(embedder.embedding_dim(), 0.0f);
                newIndex->addPoint(zero.data(), (hnswlib::labeltype)label);
                newIndex->markDelete((hnswlib::labeltype)label);

                ofs1.seekp((std::streamoff)label * 16);
                write_int64(ofs1, 0);
                write_int64(ofs1, 0);
            } else {
                // read sentence
                std::string sentence(end - start, '\0');
                sfs2.seekg(start);
                sfs2.read(&sentence[0], sentence.size());

                // append to shard1 sentences
                sfs1_append.write(sentence.data(), sentence.size());
                sfs1_append.put('\n');

                int64_t newStart = base_offset;
                int64_t newEnd   = base_offset + sentence.size() + 1;
                base_offset = newEnd;

                // update offsets
                size_t label = shard1->allocate_label();
                ofs1.seekp((std::streamoff)label * 16);
                write_int64(ofs1, newStart);
                write_int64(ofs1, newEnd);

                // re-encode and add embedding
                auto emb = embedder.encode_text(sentence, cfg.debug);
                newIndex->addPoint(emb.data(), (hnswlib::labeltype)label);
            }
            idx++;
        }

        ofs1.close();
        sfs1_append.close();

        // swap in new index
        shard1->replaceIndex(std::move(newIndex));

        // remove shard2 files
        std::remove(shard2->get_sentences_path().c_str());
        std::remove(shard2->get_offsets_path().c_str());
        std::remove(shard2->get_index_path().c_str());

        // drop shard2
        shards.pop_back();

        return true; // OK
    }

   // Merge it all!!
   bool merge() {
     size_t count = 0;
     while (merge_last_two()) count++;
     return count ? true : false;
   }

};

// ---------------- Manager ----------------
class BertIndexManager {
    SBertGGML embedder;
    HnswConfig cfg;
    std::unordered_map<std::string,std::unique_ptr<ShardedIndex>> idxs;
public:
    BertIndexManager(HnswConfig c) : BertIndexManager(c.model, c){};
    BertIndexManager(const std::string&model,HnswConfig c):embedder(model),cfg(c){}
    ShardedIndex&get(const std::string&n){
        if(!idxs.count(n)) idxs[n]=std::make_unique<ShardedIndex>(embedder,n,cfg);
        return*idxs[n];
    }
#if 1 /* FOR TESTING */
    void append(const std::string&n,const std::string&s){get(n).append(s);}
#else
    // Production append
    void append(const std::string&n,const std::string&s, int64_t start, int64_end){get(n).append(s, start, end);}
#endif

#if 1
    void remove(const std::string&n,size_t label,size_t shard=0){get(n).remove(label,shard);}
    void markDelete(const std::string&n,size_t label,size_t shard=0){get(n).markDelete(label,shard);}
    void undelete(const std::string&n,size_t label,size_t shard=0){get(n).undelete(label,shard);}
    void delete_byAddress(const std::string&n,int64_t addr,size_t shard=0){get(n).delete_byAddress(addr,shard);}
    void remove_byAddress(const std::string&n,int64_t addr,size_t shard=0){get(n).delete_byAddress(addr,shard);}
    void undelete_byAddress(const std::string&n,int64_t addr,size_t shard=0){get(n).undelete_byAddress(addr,shard);}
    size_t shard_count(const std::string&n){return get(n).shard_count();}
#else
    void remove(const std::string&n,size_t label){get(n).remove(label);}
#endif
    void flush(const std::string&n){get(n).flush();}

    std::vector<SearchResult> knn(const std::string&n,const std::string&q,size_t k=5){return get(n).knn(q,k);}
    std::vector<SearchResult> radius(const std::string&n,const std::string&q,float minScore = 0.0){
        return get(n).radius(q,minScore);
    }
    std::vector<SearchResult> relative(const std::string&n,const std::string&q,float alpha = 0.0){
        return get(n).relative(q,alpha);
    }
    std::vector<SearchResult> adaptive(const std::string&n,const std::string&q,
	float alpha = 0.0,size_t minN = 0,size_t lookahead=0,float gapDelta=0){
       return get(n).adaptive(q,alpha,minN,lookahead,gapDelta);
    }
    std::string text(const std::string&n,const SearchResult&r){return get(n).get_text(r);}

    void merge(const std::string& n) { get(n).merge(); }
    void merge_last_two(const std::string& n) { get(n).merge_last_two(); }
};

#if 1

enum class SearchType { Adaptive, Knn, Radius, Relative };

class EmbeddingIndexer
{
   BertIndexManager         *man;
   HnswConfig                config;
   SearchType                modus;
   std::vector<SearchResult> empty;

   bool              man_init() {
     if (man || (file_exists(config.model) && (man = new BertIndexManager(config)) != NULL))
	return true;
     return false;
   }
public:
   // db_hnsw, db_nsg, db_IVFFlat
   EmbeddingIndexer(HnswConfig& cfg) : man(NULL), modus(SearchType::Knn) {
     HnswConfig      config = cfg;
   };

  ~EmbeddingIndexer() {
    if (man) delete man;
  }
   bool setModelPath(const std::string& path) {
     if (file_exists(path)) {
       config.model = path;
       return true;
     }
     return false;
   }

   void flush(const std::string& fieldname) {
     // We flush all the types we have
     if (man) man->flush(fieldname); // Right now only Bert/HNSW
   }
   void merge_last_two(const std::string& fieldname) {
     flush(fieldname);
     if (man)  man->merge_last_two(fieldname);
   }
   void merge(const std::string& fieldname) {
     flush(fieldname);
     if (man) man->merge(fieldname);
   }
   std::vector<SearchResult>  search_adaptive(const std::string& fieldname, const std::string& query) {
     man_init();
     return man ?  man->adaptive(fieldname, query) : empty;
   }
   std::vector<SearchResult>  search_knn(const std::string& fieldname, const std::string& query) {
     man_init();
     return man ?  man->knn(fieldname, query) : empty;
   }
   std::vector<SearchResult>  search_relative(const std::string& fieldname, const std::string& query) {
     man_init();
     return man ?  man->relative(fieldname, query) : empty;
   }
   std::vector<SearchResult>  search_radius(const std::string& fieldname, const std::string& query) {
     man_init();
     return man ?  man->radius(fieldname, query) : empty;
   }    

   std::vector<SearchResult>  adaptive_search(const std::string& fieldname, const std::string& query) {
     if (query.empty()) return empty;
     std::string index = (fieldname.empty()) ? config.default_field : fieldname;
     switch(modus) {
        case SearchType::Adaptive:  return search_adaptive(index, query);
        case SearchType::Relative:  return search_relative(index, query);
        case SearchType::Radius:  return search_radius(index, query);
        case SearchType::Knn: default:  return search_knn(index, query);
     }
   }

   bool append(const std::string& buffer, const std::string& fieldname, int64_t start, int64_t end, int type) {
       switch (type) {
	case 0: man_init();
#if 0
	// Production code
	if (man) {
	  man->append(buffer, fieldname, start, end);
	  return true;
	}
	break;
#endif
	default: break;
     }
     return false;
   }

} ;


#endif

#if STANDALONE

// ---------------- Main ----------------
int main(int argc,char**argv){
   if(argc<2){
usage:
        std::cerr<<"Usage: "<<argv[0]<<" <sbert.ggml> [--metric l2|ip|cos] [--chunk max_tokens overlap] [--debug]\n";
        return 1;
    }   

    HnswConfig cfg; 

    Metric chosen=Metric::L2; // Metric::Cosine
    for(int i=2;i<argc;i++){
        std::string arg=argv[i];
        if(arg=="--metric" && i+1<argc){
            std::string val=argv[++i];
            if(val=="l2") chosen=Metric::L2;
            else if(val=="ip") chosen=Metric::InnerProduct;
            else if(val=="cos") chosen=Metric::Cosine;
            else std::cerr << "Unknown metric: " << val << "\n";
        } else if(arg=="--debug" || arg == "-d"){
            cfg.debug=true;
        } else if(arg=="--chunk" && i+2<argc){
            // Need to look at max_seq_length of the sBert
            if (( cfg.max_tokens_per_chunk=std::stoi(argv[++i])) > 512)
              std::cerr << "Warning: large chunk size specified (normally at most 128-512 tokens)\n";
            auto overlap = std::stof(argv[++i]);
            if (overlap > 0.1f && overlap < 1.0f) cfg.overlap_percent = overlap;
            else if (overlap < 100) cfg.overlap_percent=overlap/100.0f;
            else {
               std::cerr << "Absurd overlap specified: " << overlap << "% (recomended is 10-20)\n";
               return -1;
            }
        }
    }

    cfg.metric=chosen;
    std::cout<<"Using metric: "<<(cfg.metric==Metric::L2?"L2":cfg.metric==Metric::InnerProduct?"InnerProduct":"Cosine")<<"\n";
    if(cfg.debug) std::cout<<"Debug mode ON\n";

    BertIndexManager man(argv[1],cfg);
    std::string current="default";
    std::cout<<"Commands: append <txt>, knn <k> <q>, radius <minScore> <q>, relative <alpha> <q>, adaptive <alpha> <minN> <lookahead> <gapDelta> <q>, merge, quit\n";
    for(std::string line;std::cout<<"["<<current<<"]> ",std::getline(std::cin,line);){
        if(line=="quit")break;
        if(line.rfind("append ",0)==0){man.append(current,line.substr(7));continue;}
        if(line.rfind("knn ",0)==0){
            std::istringstream iss(line.substr(4));
            size_t k;iss>>k;std::string q;std::getline(iss,q);if(!q.empty()&&q[0]==' ')q=q.substr(1);
            for(auto&r:man.knn(current,q,k))std::cout<<" - [score="<<r.score<<"] "<<man.text(current,r)<<"\n";
            continue;
        }
        if(line.rfind("radius ",0)==0){
            std::istringstream iss(line.substr(7));
            float s;iss>>s;std::string q;std::getline(iss,q);if(!q.empty()&&q[0]==' ')q=q.substr(1);
            for(auto&r:man.radius(current,q,s))std::cout<<" - [score="<<r.score<<"] "<<man.text(current,r)<<"\n";
            continue;
        }
        if(line.rfind("relative ",0)==0){
            std::istringstream iss(line.substr(9));
            float a;iss>>a;std::string q;std::getline(iss,q);if(!q.empty()&&q[0]==' ')q=q.substr(1);
            for(auto&r:man.relative(current,q,a))std::cout<<" - [score="<<r.score<<"] "<<man.text(current,r)<<"\n";
            continue;
        }
        if(line.rfind("adaptive ",0)==0){
            std::istringstream iss(line.substr(9));
            float a,g;size_t m,l;iss>>a>>m>>l>>g;std::string q;std::getline(iss,q);if(!q.empty()&&q[0]==' ')q=q.substr(1);
            for(auto&r:man.adaptive(current,q,a,m,l,g))std::cout<<" - [score="<<r.score<<"] "<<man.text(current,r)<<"\n";
            continue;
        }

        if (line == "merge") {
            if (man.get(current).merge_last_two())
              std::cout << "Merged last two shards of index '" << current << "'.\n";
           else std::cout << "No two shards to merge in '" << current <<  "'.\n";
           continue;
        }

	// Assume a query
        for(auto&r:man.knn(current,line,5))std::cout<<" - [score="<<r.score<<"] "<<man.text(current,r)<<"\n";
            continue;

    }
}
#endif


/*
Notes & caveats

The code assumes the presence of bert.h with the C-style functions:
   bert_load_from_file, bert_tokenize, bert_eval, bert_free, bert_n_embd.
Keep those symbols available at link time.

A working bert is provided with this distribution.

hnswlib::HierarchicalNSW constructors and methods are used as in header-only hnswlib (common distribution).
If your hnswlib is built differently, adjust constructors accordingly (I used both file-based constructor
and parameter-based creation).

A working hnswlib is provided with this distribution

Cosine similarity is implemented by normalizing embeddings — HNSW uses inner-product space, so Cosine behaves
like InnerProduct on normalized vectors.

If you choose --metric cos, set metric to Cosine and the code normalizes. For L2 we use L2Space.

The offsets file stores pairs of 8-byte signed integers (start, end), so each entry is 16 bytes.
Deletion marks offsets as zero and calls markDelete(label) on the HNSW index; freed labels are reused.
When a shard reaches max_elements, a new shard is automatically created.
--debug prints query norms, chunk norms, raw distances and offsets to stderr.

Shards are only numbered after the first. Typically since we are creating a specific index per field
and sharding is only when we have more than (in the default) 100000 elements, we will probably have no
additional shards. NOTE: Performance across shards in linear..

Right now we use sentences and start/end but when its wrapped into re-Isearch we'll dump the sentences
as they are just for development and debugging and instead of start and end we'll store start GP and
end GP, basically a FC. Each start GP encodes not just the start of the field in the file but also the
identity of the file.

*/

