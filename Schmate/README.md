=== Vector search using HNSW and Sentence Transformers ===

Semantic search with SBERT + GGML Tensor Library + HNSWlib.

See tests/run_test.sh for usage.


This system provides a sentence-embedding search engine built on:
- SBERT (Sentence-BERT) model running via the ggml tensor library (no Python dependency)
- HNSWlib for fast approximate nearest neighbor (ANN) retrieval
- Memory-mapped offset files for persistent, efficient text–embedding linkage
- Automatic sharding, flushing, and adaptive thresholding

We use a significantly enhanced (adding among other features quantized spaces) and turbo-charged (including support for x86 and ARM SIMD) HNSWlib for approximate nearest-neighbor search, and efficient mmap-backed re-scoring and offset storage for text retrieval. The system supports sharded HNSW indices, multiple search modes (kNN, radius, relative, adaptive, epsilon), deletion/undelete, merges, and incremental on-disk flushing. It also includes training for hyperparameter optimization.


All code is implemented in modern C++17, optimized for macOS and Linux.

While this code uses bert.cpp, support for llama.cpp is also provided.

=== Interface to re-Isearch ===

Interface code to re-Isearch is provided by the EmbeddingIndexer class.
It reads the Section "Embedding" in the database configuration (db.ini)
for the project directory ("project").

 [Embedding]

 project=&lt;directory where the project files are located&gt;

Example: project = myproject

It then loads the HNSW Configuration in the order:
1) Global config - /etc/schmate/config.bin (system defaults)
2) Local config - ~/.schmate/config.bin (user preferences)
3) Project config - myproject/config.bin (project-specific)

This design allows us to build searchable indexes using different embedding models by exploiting virtual targets: recall a single searchable virtual index can contain up to 255 physical indexes.

To handle multiple embedding models we'll let each element in the ensemble have its own model.. Eg. a virtual DB with two DBs: A and B. DbA for modelA and DbB for modelB.  A search of the ensembed A+B would search both each with their own model..

NOTE: The tool "config_editor" can be used to view/edit/modify the configuration files.

Example of a configuration (show command):
<pre>
> === HNSW Configuration ===
Default search mode: Knn

Index parameters:
  max_elements: 100000
  M: 16
  ef_construction: 200
  ef_search: 64
  specification: L2-None-Pass
  normalize_embeddings: no

Embedding:
  bert_n_threads: 4

Chunking:
  max_tokens_per_chunk: 128
  overlap_percent: 0.1

Search defaults:
  k (knn): 5
  radius: 0.7
  alpha (relative): 0.8
  minN (adaptive): 3
  lookahead (adaptive): 10
  gapDelta (adaptive): 0.1
  enable_rescoring (quantized): no
  deletion_threshold_pc: 0.2

Epsilon search:
  epsilon: 0.15
  epsilonL2: 1.41
  epsilonIP: 0.5
  min_candidates: 10
  max_candidates_cap: 0

Performance:
  knn_lookahead_scale: 5
  flush_threshold: 100
  flush_offsets_each: no
  parallel_merge: yes
  merge_threads: 10

Tuning:
  auto_tune_ef: no
  auto_tune_eps: no

Debug: enabled
Model: <Undefined>

===   This Platform    ===
OS: Darwin 24.6.0
Hardware: arm64 / 10 cores
SIMD: ARM NEON enabled
</pre>
