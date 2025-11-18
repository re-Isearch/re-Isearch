 sbert_search

Semantic search with SBERT + GGML + HNSWlib.

See tests/run_test.sh for usage.


This system provides a sentence-embedding search engine built on:
- SBERT (Sentence-BERT) model running via ggml (no Python dependency)
- HNSWlib for fast approximate nearest neighbor (ANN) retrieval
- Memory-mapped offset files for persistent, efficient text–embedding linkage
- Automatic sharding, flushing, and adaptive thresholding

We use a significantly enhanced (adding among other features quantized spaces) and turbo-charged (including support for x86 and ARM SIMD) HNSWlib for approximate nearest-neighbor search, and efficient mmap-backed re-scoring and offset storage for text retrieval. The system supports sharded HNSW indices, multiple search modes (kNN, radius, relative, adaptive, epsilon), deletion/undelete, merges, and incremental on-disk flushing. It also includes training for hyperparameter optimization.


All code is implemented in modern C++17, optimized for macOS and Linux.

While this code uses bert.cpp, support for llama.cpp is also provided.
