# SBERT + HNSWlib Semantic Search (GGML + bert.cpp)

You also want 
  ggml-model-q4_0.bin
this should be renamed to sbert.ggml for convience.


# SBERT + HNSWlib Semantic Search (GGML + bert.cpp)

This project integrates **[bert.cpp](https://github.com/skeskinen/bert.cpp)** for running Sentence-BERT (SBERT) models in GGML format with **[hnswlib](https://github.com/nmslib/hnswlib)** for fast approximate nearest neighbor (ANN) search.

It provides:
- A `BertIndex` class that encapsulates SBERT embeddings + HNSW indexing.
- Persistent storage of sentences, offsets, and HNSW index.
- Multiple search modes: **kNN**, **radius**, **relative threshold**, and **adaptive**.
- Configurable HNSW parameters via a simple `HnswConfig` struct.

---

## 🔧 Build

### Prerequisites
- macOS or Linux
- CMake ≥ 3.15
- Dependencies:
  - [`bert.cpp`](https://github.com/skeskinen/bert.cpp)
  - [`hnswlib`](https://github.com/nmslib/hnswlib)

### Example Build Steps
```bash
# clone dependencies
git clone https://github.com/skeskinen/bert.cpp.git
git clone https://github.com/nmslib/hnswlib.git

# convert a HuggingFace SBERT model to GGML
python3 bert.cpp/convert-bert-hf-to-ggml.py \
  --hf-path sentence-transformers/all-MiniLM-L12-v2 \
  --outfile sbert.ggml

# build your project
mkdir build && cd build
cmake .. \
  -DBERTCPP_DIR=../bert.cpp \
  -DHNSWLIB_DIR=../hnswlib \
  -DGGML_LIB_DIR=../bert.cpp/build
make -j



💬 Interactive Commands
Inside the REPL:
append <sentence> → add a sentence, persist to files + update index.
reset → clear everything (index + files).
<query> → default search (adaptive mode).
knn <k> <query> → top-k nearest neighbors.
radius <minScore> <query> → return all results with similarity ≥ minScore.
relative <alpha> <query> → return all results where score ≥ alpha × bestScore.
adaptive <alpha> <minN> <lookahead> <gapDelta> <query> → advanced adaptive search.
empty line → exit.


Search Modes Explained
kNN search
Always returns top-k neighbors, even if some are weak matches.
Use when you want a fixed number of results.
Radius search
Returns all neighbors above a minimum similarity score.
Use when you prefer quality over quantity.
Relative threshold search
Returns all neighbors whose score ≥ alpha × bestScore.
This adapts per query and avoids arbitrary absolute cutoffs.


Typical Similarity Ranges (SBERT)
≥ 0.8 → very likely semantically equivalent.
0.6 – 0.8 → possibly related.
< 0.6 → usually unrelated.


💬 Interactive Commands
Inside the REPL:
append <sentence> → add a sentence, persist to files + update index.
reset → clear everything (index + files).
<query> → default search (adaptive mode).
knn <k> <query> → top-k nearest neighbors.
radius <minScore> <query> → return all results with similarity ≥ minScore.
relative <alpha> <query> → return all results where score ≥ alpha × bestScore.
adaptive <alpha> <minN> <lookahead> <gapDelta> <query> → advanced adaptive search.
empty line → exit


Example
> adaptive 0.8 3 10 0.1 artificial intelligence
alpha=0.8 → relative threshold (≥ 80% of best hit).
minN=3 → always return at least 3 results.
lookahead=10 → check top 10 results for a score drop.
gapDelta=0.1 → consider a 0.1 drop as significant.
If no explicit command is given, the REPL defaults to adaptive search.

🛠️ Future Extensions
Support for hybrid filtering (metadata + vector search).

