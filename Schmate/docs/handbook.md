# 🧠 SBERT-HNSW Embedding Search Handbook

## Overview

This handbook documents the SBERT-HNSW embedding search system — a C++17 project that combines a GGML-based Sentence-BERT encoder, HNSWlib for approximate nearest-neighbor search, and efficient mmap-backed offset storage for text retrieval. The system supports sharded HNSW indices, multiple search modes (kNN, radius, relative, adaptive), deletion/undelete, merges, and incremental on-disk flushing.

This system provides a sentence-embedding search engine built on:
- SBERT (Sentence-BERT) model running via ggml (no Python dependency)
- HNSWlib for fast approximate nearest neighbor (ANN) retrieval
- Memory-mapped offset files for persistent, efficient text–embedding linkage
- Automatic sharding, flushing, and adaptive thresholding

All code is implemented in modern C++17, optimized for macOS and Linux.

The repository provides the following main components:
- `SBertGGML` — model tokenizer + encoder wrapper (ggml).
- `OffsetFile` — memory-mapped persistent structure mapping HNSW labels to sentence metadata and byte offsets.
- `BertIndex` — single-shard index combining HNSW, OffsetFile, and sentences file; supports append/search/delete/flush.
- `ShardedIndex` — collection of `BertIndex` shards with automated discovery, sharding logic, and parallel search.
- `BertIndexManager` — top-level manager for named indexes and the interactive REPL.

This document explains each class, important functions, configuration options, file formats, and developer notes. It also contains UML diagrams illustrating class relationships and data flow.

---

## Table of Contents

- [Overview](#overview)
- [Classes and APIs](#classes-and-apis)
  - [SBertGGML](#sbertggml)
  - [OffsetFile](#offsetfile)
  - [BertIndex](#bertindex)
  - [ShardedIndex](#shardedindex)
  - [BertIndexManager](#bertindexmanager)
- [Search Modes & Heuristics](#search-modes--heuristics)
- [File Formats](#file-formats)
- [Concurrency & Performance](#concurrency--performance)
- [Operational Notes](#operational-notes)
- [UML Diagrams](#uml-diagrams)
- [Quick Start Examples](#quick-start-examples)
- [Developer Checklist & Tests](#developer-checklist--tests)

---





## Classes and APIs

### `SBertGGML`

**Purpose:** Wraps the GGML-based SBERT model: tokenization, evaluation, and optional normalization for cosine search.

**Key members (example):**
```cpp
struct SBertGGML {
    bert_ctx *ctx = nullptr;       // internal ggml model context
    size_t n_embd = 0;             // embedding dimension
    bool normalize_for_cosine = true;

    explicit SBertGGML(const std::string &model_path);
    ~SBertGGML();

    std::vector<float> encode_text(const std::string &text, bool debug=false);
};
```

**Usage notes:**
- Initialize once per thread or keep a shared instance with thread-safe encoder usage (or guard concurrency).
- When `cfg.metric == "cosine"`, `encode_text()` should normalize embeddings (L2 norm = 1) before returning them. Do **not** normalize if the runtime metric is L2.
- Provide a small debug mode that prints token counts, first N embedding components, and norm.

**Example:**
```cpp
SBertGGML encoder("/path/to/sbert.ggml");
auto emb = encoder.encode_text("He develops in C++", true);
```

---

### `OffsetFile`

**Purpose:** Persistent, memory-mapped table of `OffsetEntry` structures that map HNSW labels to sentence metadata (SID, token spans, file byte offsets).

**Data layout**
```cpp
struct OffsetEntry {
    int64_t sentence_id;       // externally provided persistent ID
    int32_t token_start;
    int32_t token_end;
    int64_t file_start;        // byte offset in sentences file (inclusive)
    int64_t file_end;          // byte offset in sentences file (exclusive)
};
```

**Key methods (example):**
```cpp
class OffsetFile {
public:
    OffsetFile(const std::string &path, size_t max_entries, size_t header_size = 0);
    ~OffsetFile();

    OffsetEntry get(size_t label) const;
    OffsetEntry *get_mut(size_t label);
    void set(size_t label, const OffsetEntry &entry);
    void for_each(const std::function<void(size_t,const OffsetEntry&)> &fn) const;

    // flush single entry or whole file
    void flush(size_t label = SIZE_MAX);
    size_t entry_address(size_t label) const;
    size_t count() const;
};
```

**Behavior & notes:**
- The file is usually created with a small header area (for manifest/meta) followed by an array of `OffsetEntry` entries.
- `get_mut()` returns a pointer directly into the mmap region for in-place updates; remember to `flush(label)` after modifying.
- The API provides a small `for_each()` helper for scanning all entries efficiently without materializing the whole table in memory.

---

### `BertIndex`

**Purpose:** Single-shard index object combining SBERT encoder, HNSW graph, the `OffsetFile`, and the `sentences` text file. Responsible for adding entries, searching, removing, undeleting, flushing and saving to disk.

**Key members (example):**
```cpp
class BertIndex {
public:
    BertIndex(const std::string &base_name, const HnswConfig &cfg);
    ~BertIndex();

    size_t append(const std::string &sentence, int64_t sentence_id);
    void appendid(int64_t sentence_id, const std::string &sentence);
    void remove(size_t label);
    void undelete(size_t label);

    std::vector<SearchResult> knn(const std::string &query, size_t k);
    std::vector<SearchResult> radius(const std::string &query, float radius);
    std::vector<SearchResult> relative(const std::string &query, float alpha);
    std::vector<SearchResult> adaptive(const std::string &query, float alpha, size_t minN, size_t lookahead, float gapDelta);

    void flush();      // flush offsets and optionally save HNSW
    void save(const std::string &path); // persist HNSW + optional metadata
    void load(const std::string &path); // load HNSW + metadata
    size_t size() const;
};
```

**SearchResult struct:**
```cpp
struct SearchResult {
    float score;
    size_t label;
    int64_t sentence_id;
    size_t token_start, token_end;
    int64_t file_start, file_end;
    size_t address;       // byte offset in .offsets file (optional)
    std::string text;     // populated lazily or by caller
};
```

**Important details:**
- `append()` will:
  - chunk long sentences into overlapping spans if configured to do so,
  - write chunk(s) into `sentences.txt` and update an `OffsetEntry` for each chunk,
  - add chunk embedding into the HNSW graph with the returned label,
  - increment `dirty_count` and flush HNSW when `dirty_count >= cfg.flush_every`.
- `remove()` should use HNSW's delete/markDeleted semantics; offsets keep an entry with `file_start==file_end==0` to indicate deleted records.
- `flush()` should msync only changed portions of the offsets mmap and optionally call `hnsw_index->saveIndex()` in batches to avoid constant re-saving after every append.

---

### `ShardedIndex`

**Purpose:** Manages a collection of `BertIndex` shards for scaling past a single HNSW capacity limit. Supports adding shards, choosing a shard for new appends, merging shards, parallel searches and shard-level operations.

**Key methods (example):**
```cpp
class ShardedIndex {
public:
    ShardedIndex(const std::string &base_name, const HnswConfig &cfg);

    void add_shard();
    size_t shard_count() const;
    void append(const std::string &sentence, int64_t sentence_id);
    void remove(size_t label, size_t shard = 0);
    void delete_byAddress(int64_t addr, size_t shard = 0);
    void merge_last_two();        // compaction: merge last two shards to reduce fragmentation
    void flush();

    // search across shards (parallel)
    std::vector<SearchResult> knn(const std::string &q, size_t k);
    std::vector<SearchResult> radius(const std::string &q, float radius);
    std::vector<SearchResult> adaptive(const std::string &q, float alpha, size_t minN, size_t lookahead, float gapDelta);
};
```

**Notes:**
- On initialization, `ShardedIndex` discovers shard files by scanning for `<base>_shard<i>.*` starting at 0 until a missing index is found. It loads all shards into memory (can be parallelized).
- `append()` selects the last shard unless it is full, in which case `add_shard()` is called.
- `merge_last_two()` (optionally parallel) creates a merged shard, then atomically replaces the two old files and updates the shard table.

---

### `BertIndexManager`

**Purpose:** A simple namespace manager mapping names to `ShardedIndex` instances. Provides REPL handling helpers and convenience operations for multiple named datasets.

**Selected API:**
```cpp
class BertIndexManager {
public:
    BertIndexManager(const HnswConfig &cfg);

    ShardedIndex &getOrCreate(const std::string &name);
    void load_or_create(const std::string &name);
    void use(const std::string &name);

    std::vector<SearchResult> knn(const std::string &name, const std::string &q, size_t k);
    void append(const std::string &name, const std::string &sentence);
    void appendid(const std::string &name, int64_t sid, const std::string &sentence);
    void flush(const std::string &name);
};
```

---

### ⚙️ Configuration (HnswConfig)

See struct HnswConfig

---

## Search Modes & Heuristics

**kNN** — Standard top-k retrieval using HNSW's `searchKnnCloserFirst`. Return order depends on metric:
- L2: lower distance is better (ascending sort)
- Cosine / Inner-product: higher value is better (descending sort after converting distances if needed)

**Radius** — Return all candidates with score >= threshold (for cosine) or <= threshold (for L2). Implement by retrieving a large candidate list (`cfg.max_elements` or an upper-bounded `k`) then filtering.

**Relative** — Keep results within `alpha * best_score`.

**Adaptive** — Retrieve a lookahead number of results, apply `minN`, then apply a `gapDelta` heuristic: stop when consecutive score gap exceeds `gapDelta` after at least `minN` results are kept.

**Ranking heuristic suggestions:** combine:
- absolute threshold (minScore)
- relative threshold (alpha × top)
- gap detection (lookahead + gapDelta)
- cluster-aware cutoff (use top-N and cluster-distance heuristics)

---

## 💾 FIle Formats / Persistence

| Component  | Storage File    | Notes                                     |
| ---------- | --------------- | ----------------------------------------- |
| Sentences  | `sentences.txt` | UTF-8 text, newline-delimited             |
| Offsets    | `offsets.bin`   | Binary, mmap-backed                       |
| HNSW index | `index.hix`     | Saved periodically or at flush/destructor |


- `sentences.txt` — newline-delimited UTF-8 sentences or chunks. Each appended chunk's start/end byte offsets are stored in `OffsetEntry`.
- `offsets.bin` — mmap-backed binary file with optional header and an array of `OffsetEntry`. Header may store meta fields: format version, dim, metric, normalized flag, element_count.
- `index.hix` — HNSW binary graph saved by `hnsw_index->saveIndex()`. Can optionally be prefixed by a small header or stored separately as `<base>.hnsw`.

**Metadata header (optional)**
```cpp
struct IndexMeta {
    uint32_t version;
    Metric metric;        // L2, cosine, ip
    bool normalized;        // were embeddings normalized on insert?
    uint32_t dim;
    uint64_t element_count;
};
```

If you prefix the HNSW file with `IndexMeta`, ensure the HNSW loader is able to seek after the header (or use stream-based load API).

---

## Concurrency & Performance

- **Parallel search:** query shards in parallel, then merge top-k results. Use a thread pool or `std::async` and aggregate.
- **Parallel merge:** when merging two shards, collect `(text, sid)` pairs concurrently, then append to the merged index using multiple threads. Guard SBERT inference concurrency to limit per-thread GGML memory pressure.
- **Flush strategy:** keep offsets in mmap and msync single-entry updates; batch HNSW saves using `dirty_count` and `cfg.flush_every` threshold to balance durability and throughput.
- **Memory safety on macOS:** call `relax_macos_malloc_zones()` (or set `MallocNanoZone=0`) before loading large ggml or HNSW buffers to avoid the "nano zone abandoned" message and fragmentation issues.
- Memory-mapped I/O for minimal disk overhead.
- Optional compile-time thread cache.

---

## 🧩 Future Extensions

- Mmap-based SID lookup cache
- Distributed multi-host shard management
- GPU-accelerated encoding (Metal / CUDA)
- Optional LRU-based embedding cache


---

## Operational Notes

- If you change the embedding dimension or metric, existing HNSW files must be rebuilt or carefully validated. Use metadata to detect mismatches at load time.
- Ensure `auto_sentence_id` resumes from the highest SID on disk at load time to preserve global SID uniqueness.
- When upgrading offset file format, implement a loader that detects old layout and upgrades to the new format (or reject old files explicitly).
- Prefer export of indexes and offsets as separate artifacts to allow migration, offline repair, and cross-system portability.

---

## UML Diagrams

- **Class Diagram** — shows relationships between `BertIndex`, `OffsetFile`, `SBertGGML`, `hnswlib::HierarchicalNSW`, and `ShardedIndex`.
- **Data Flow Diagram** — shows appending flow (sentence → chunk → sentences.txt + offsets + HNSW) and search flow (query → encode → HNSW search → offsets lookup → text reconstruction).

Images included in this package:
- `images/class_diagram.svg`
- `images/data_flow.svg`

(See diagrams below.)

---

## Quick Start Examples

**Create index and append:**
```cpp
HnswConfig cfg;
cfg.metric = "cosine";
cfg.max_elements = 100000;
BertIndexManager manager(cfg);
manager.load_or_create("default");
manager.append("He develops in C++");
manager.appendid(1001, "She likes Prolog");
manager.flush("default");
```

**Search (kNN):**
```cpp
auto res = manager.knn("default", "what language does he use?", 5);
for (auto &r : res) {
    std::cout << r.score << " " << r.text << std::endl;
}
```

**Merge last two shards:**
```cpp
auto &sh = manager.getOrCreate("default");
sh.merge_last_two();
sh.flush();
```

---

## Developer Checklist & Tests

**Startup checks:**
- [ ] `offsets.bin` header matches expected format.
- [ ] highest SID is computed and `auto_sentence_id` resumed.
- [ ] HNSW metric/dim validation against metadata.

**Integration tests:**
- Small test corpus (4-8 sentences) and expected ranking results for several queries.
- Test append+restart: append, exit, restart, append more; ensure SID incrementing persists and no collisions.
- Test delete+undelete and reclaim label space.
- Test merge_last_two preserves SIDs and deletes old files safely.

---




## Contact & Contribution

If you extend the project (new heuristics, alternate embedding models, or distributed sharding), please update this handbook with API changes. Contributions welcome via PRs to the repository.

edz@nonmonotonic.net


---
