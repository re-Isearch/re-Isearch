#sbert_search

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

Interface code to re-Isearch is provided by the EmbeddingIndexer class.
It reads the Section "Embedding" in the database configuration (db.ini)
for the project directory ("project").

 [Embedding]
 project=<directory where the project files are located>

It then loads the HNSW Configuration in the order:
1) Global config - /etc/schmate/config.bin (system defaults)
2) Local config - ~/.schmate/config.bin (user preferences)
3) Project config - myproject/config.bin (project-specific)

This design allows us to build searchable indexes using different embedding models by exploiting virtual targets: recall a single searchable virtual index can contain up to 255 physical indexes.

To handle multiple embedding models we'll let each element in the ensemble have its own model.. Eg. a virtual DB with two DBs: A and B. DbA for modelA and DbB for modelB.  A search of the ensembed A+B would search both each with their own model..
