# Vector search using HNSW and Sentence Transformers

**Semantic search with SBERT + GGML Tensor Library + HNSWlib.**

See tests/run_test.sh for usage.

This system provides a sentence-embedding search engine built on:
- SBERT (Sentence-BERT) model running via the ggml tensor library (no Python dependency)
- HNSWlib for fast approximate nearest neighbor (ANN) retrieval
- Memory-mapped offset files for persistent, efficient text–embedding linkage
- Automatic sharding, flushing, and adaptive thresholding

Not only is it probably the most performat vector text engine currently available for use on local hardware  but also most likely the fullest featured. Its also 100% open source (Apache 2.0) "*no strings attached*".

It may be used both inside *re-Isearch* but also without. It has been designed to be used in a host of other applications. 


## HNSW 

Hierarchical Navigable Small World (HNSW) is a graph-based algorithm used in vector databases to find similar items in
high-dimensional datasets. Originally published in 2016 [Efficient and robust approximate nearest neighbor search using Hierarchical Navigable Small World graphs] (https://arxiv.org/abs/1603.09320) by Yury Malkov, his C++ implementation is not only reference but widely regarded as one of the most performant.

Stating with Malkov's HNSWlib as a basis we significantly enhanced (adding among other features quantized spaces) and turbo-charged (including support for x86 and ARM SIMD) it while also adding efficient mmap-backed re-scoring and offset storage for text retrieval. Our system supports sharded HNSW indices, multiple search modes (kNN, radius, relative, adaptive, epsilon), deletion/undelete, merges, and incremental on-disk flushing. It also includes training for hyperparameter optimization.

All code is implemented in modern C++17, optimized for macOS and Linux.

## SBERT: bert.cpp, llama.cpp and the ggml tensor library

Behind our embeddings is our fork of bert.cpp and the mainlined llama.cpp. Behind these is the GGML tensor library.  Unlike mainstream vendor libraries (like PyTorch, TensorFlow, or TensorRT) built for data-center GPUs, GGML runs large transformer models efficiently on commodity hardware.

1. **Superior CPU and Edge Device Execution**.
Mainstream libraries rely heavily on massive GPU VRAM to hold model weights during inference. GGML is written in plain C and C++, making it highly portable.
- *Zero Dependencies*: It requires no heavy third-party stacks (such as CUDA or ROCm) just to run on a standard machine.
- *Hardware Interoperability*: It is optimized for x86 architectures (AVX, AVX2, AVX-512) and Apple Silicon (via native ARM NEON and Metal).
- *System RAM Utilization*: Because most modern consumer computers feature far more system RAM than GPU VRAM, GGML natively bridges the gap by enabling models to run efficiently on standard CPUs or via CPU-GPU offloading.

2. **Accelerators**: GGML features a highly modular backend architecture. This layout allows it to offload computation graph execution to a variety of hardware accelerators via specific hardware drivers, completely bypassing heavy Python stacks.


3. **Advanced, Fine-Grained Quantization**: One of GGML's most compelling features is its pioneering approach to model quantization (compressing floating-point numbers into lower bit-widths, such as 4-bit or 5-bit).
- *K-Quants*: GGML utilizes highly engineered, variable bit-width quantization strategies (like Q4_K_M) that minimize the traditional loss in output accuracy.
- *Memory Compression*: It shrinks the memory footprint of massive models by up to 70-80% without heavily compromising the model's perplexity or reasoning capabilities.


4. **Streamlined Portability** (The GGUF Standard). GGML's associated file format, GGUF, was specifically designed to solve the fragmentation and loading issues of older ML formats.

5. **No Python Overhead**

6. **Specialized Text-Generation Ecosystem**

While mainstream libraries like TensorRT or PyTorch remain undisputed for model training and high-throughput server farms, GGML democratizes access to AI by allowing us to run powerful models locally on mainstream hardware.

While we build upon a heavily refactored bert.cpp we also support llama.cpp for more modern sophisticated models that don't use the BERT architecture such as EuroBERT.

## Accelerators
Our HNSW implementation is, by design, CPU bound (using heavily optimized SIMD instructions) to leave the accelerators free to do the heavy lifting of running the models. Even on an Apple M1 (Pro) we've clocked 13k QPS-- on an M5 Pro we expect 30,000 to 39,000+ QPS-- so we found ample state-of-the-art performance already on CPUs.

(Our Apple reference hardware is the Apple M1 Pro to set a comparatively lower threshold of hardware requirements in 2026 as we don't want to design just to be useable on the "*latest and greatest*". For ARM CUDA our reference platform is the AGX Orin64-- which we can make look like a $250 USD Orin Nano-- rather than AGX Thor or GB10. For x86 we similarly target the beloved NVIDIA GTX-1090 with AVX-2 board rather than a RTX-5090 with AVX-512 as reference. A current AVX-512 machine should get 20-50% higher HNSW QPS and between 15 and 30x faster BERT inferencing.)

1. **NVIDIA CUDA**
- *Quality*: This is the most mature, heavily optimized backend in the GGML ecosystem. It receives immediate day-one updates for new architectures.
- *Performance*: Offers the highest token-per-second generation rates. It handles massive context windows and batched requests effortlessly.
- *Efficiency*: While desktop NVIDIA cards draw significant wattage, the work-per-watt remains high because the GPU completes the generation sequence rapidly and returns to idle.

2. **Apple Metal**
- *Quality*: Flawless integration natively recognized by macOS and iOS.
- *Performance*: Extremely rapid. Because Apple Silicon uses a Unified Memory Architecture, the CPU and GPU share the same physical RAM pool. GGML leverages this via mmap, allowing a model to load instantly without a slow transfer over a PCIe bus.
- *Efficiency*: Unmatched. Users can run quantized 70B models locally on a MacBook with minimal heat production, preserving battery life during background operations.

3. **Vulkan** (The Compatibility Champion)
- *Quality*: Highly robust and serves as the best open-source, vendor-agnostic fallback.
- *Performance*: While it slightly lags behind native CUDA or ROCm tweaks, it delivers impressive matrix-multiplication speeds across diverse hardware, including Steam Decks, Raspberry Pis, and mixed-GPU setups.
- *Efficiency*: Balanced. Vulkan provides hardware acceleration on low-power devices that lack dedicated AI chips.

## Asysmetric Models

### Model Routing and Embedding Prefix Protocol
To maximize retrieval precision across asymmetric vector spaces (matching short, intent-heavy search queries to dense, informational document chunks), the embedding orchestration pipeline utilizes a strongly typed classification framework.

Different deep learning text transformers are trained with highly specific prefix strings or natural language instructions. Passing raw text without these triggers—or applying instructions to models that expect unpolluted text—destabilizes the hidden attention matrices and heavily degrades Top-K retrieval performance.

Our architecture maps internal GGUF metadata string lookups (general.name and general.architecture) down to a lightweight runtime ModelClass enum at instantiation. This guarantees that all downstream string formatting resolves instantly via an optimized jump table with zero allocations during active processing.

<pre>enum ModelClass { VANILLA, BGE, NOMIC, E5, E5_MISTRAL, GTE, GTE_INSTRUCT,
                INSTRUCTOR, SFR, JINA, MXBAI, UNKNOWN}
</pre>

### Supported Embedding Model Families

1. Nomic Embed (NOMIC)
- Architecture Base: Modernized BERT with Rotary Position Embeddings (RoPE), FlashAttention, and Masked Language Modeling (MLM). Natively supports an extended 8192 token context window.
- Asymmetry Strategy: Strict two-sided task classification.

2. Legacy E5 (E5)
- Architecture Base: Traditional BERT-style symmetric bi-encoders (e.g., e5-large-v2).
- Asymmetry Strategy: Direct functional prefixes added to differentiate intention.

3. Jina AI Embeddings (JINA)
- Architecture Base: Symmetric bi-encoders utilizing advanced ALIBI linear position biases to handle a massive 8192 context length cleanly on standard hardware.
- Asymmetry Strategy: Explicit dot-notation task tagging.

4. E5 Mistral Instruct (E5_MISTRAL)
- Architecture Base: Large Language Model (LLM) decoder-only architecture (Mistral-7B) adapted for dense text retrieval.
- Asymmetry Strategy: Autoregressive task conditioning. The model expects an explicit instruction prompt to trigger retrieval behavior on queries, while documents are ingested completely raw to preserve structural data bounds.

5. BGE - Beijing General Embedding (BGE)
- Architecture Base: Highly optimized standard BERT-style bi-encoders (e.g., bge-large-en-v1.5).
- Asymmetry Strategy: Asymmetric instruction activation. It applies an explicit retrieval prompt only during semantic matching queries.

6. Mixedbread AI (MXBAI)
- Architecture Base: High-performance, English-optimized BERT variants tuned specifically for fine-grained retrieval-augmented generation (RAG) pipelines.
- Asymmetry Strategy: Asymmetric instruction activation matching the standard BGE protocol layout.

7. GTE Instruct (GTE_INSTRUCT)
- Architecture Base: LLM decoder-only architectures adapted from the Alibaba Qwen model framework (e.g., gte-Qwen2-7B-instruct).
- Asymmetry Strategy: Strict conversational task-instruction framing.

8. Salesforce SFR-Embedding (SFR)
- Architecture Base: Ultra-large, dense LLM-based embedding models optimized for complex, multi-hop semantic retrieval tracking.
- Asymmetry Strategy: Natural language search-space task constraint.

9. Standard GTE & Vanilla Models (GTE / VANILLA)
- Architecture Base: Standard bi-encoders (e.g., gte-large, all-MiniLM-L6-v2).
- Asymmetry Strategy: Symmetric mapping. These models map text directly to a unified vector space without calculating conversational or positional task headers.

## Building

Build is via the CMAKE build system. Pre-requisite (min) is the bert.cpp code in the 3rdParty folder (from the base directory of re-Isearch).  Since we support other bert.cpp and llama.cpp as well as their different ggml tensor libraries the default is to link from the build directory. This libschmate.dylib (MacOS) or libschmate.so (Linux) should then be copied into a suitable directory for linking. 

When building re-Isearch make sure that the VECTOR_INDEX is defined..

## Quantization Algorithms supported (Added to HNSWlib)

Quantization sizes:
- NONE, BIN1, INT158, INT4, INT8, INT16, FP16, BF16

Quantization Algorithms:
- PASS, STANDARD, BETTER, CENTROID, ROTATIONAL, RABITQ, RABITQ_EXTENDED and MRLQ (in development)

A typical RaBitQ quantization consists of both Algorithm set to "RABITQ" and size of BIN1 (Binary)

### TurboQuant

We don't yet support *TurboQuant*.  When Google published their pre-print [paper](https://arxiv.org/abs/2504.19874) (presented at ICLR 2026 last January)  we wondered "*could this be a better RaBitQ for embeddings*"?  Our results and benchmarks with RaBitQ have been nothing short of spectacular. We were on the fence.  The Apr 2026 paper ["Revisiting RaBitQ and TurboQuant: A Symmetric Comparison of Methods, Theory, and Experiments" ](https://arxiv.org/abs/2604.19528) by Jianyang Gao (the original author of the 2024 RaBitQ paper) et al. reinforced our initial concerns that it was not just slower-- relevant for central design philosophy to be as efficient as possible on mainstream hardware-- but, at best, only marginally "better" or to quote the paper *"TurboQuant offers no clear and consistent advantage over RaBitQ in directly comparable settings."*.

- Accuracy: In direct head-to-head tests, RaBitQ matched or outperformed TurboQuant in nearest-neighbor recall and inner-product stability across most bit widths.
- Speed: When tested on identical GPU hardware, RaBitQ was found to be 1.2x to 1.8x faster than TurboQuant.
- Reproducibility: The independent study found that TurboQuant's reported runtime and recall results could not be reproduced using the officially released implementation on the stated hardware.

That all said: if there is real need it should not be terribly difficult to extend our HNSWlib fork to support TurboQuant. 


### Matryoshka Representation Learning + Quantization (MRLQ)

MRLQ is a special case: Matryoshka Representation Learning (MRL) is a technique creating efficient, resizable AI embeddings. Named after Russian nesting dolls, it allows smaller dimensions to capture broad semantic meaning while larger dimensions encode granular details, all in one model.  MRLQ takes this concept and applies RaBitQ quantization to a slice and re-scoring to the whole vectors. Its having you cake and eating it too.

## Storage Efficiency: The Power of Packing (PASS)

Notice the algorithm "PASS". It means "pass-through". Instead of using a quantization algorithm its function is to handle pre-quantized models and using optimized packing algorithms. A 1024d INT4 (4-bit INT) model gets packed into 1/8 of the space:
512 bytes versus 4096.  This 8:1 compression ratio, for example, allows you to store 8 million vectors in the same 4GB of RAM that would normally hold only 1 million of the same INT4 vectors padded to FP32

For pass-through we support the following storage sizes: BIN1, INT2, INT3, INT4, INT5, INT6, INT8, INT16 INT32, FP16, BF16 and, of course, FLOAT32.  As with our quantization algorithms, the packing code uses SIMD (AVX, NEON, SVE)  whenever beneficial.

Storage Efficiency: The Power of Packing

By utilizing pass-through we dramatically reduce the memory footprint of high-dimensional pre-quantized embeddings without sacrificing any retrieval accuracy.

## Pre-Quantized Models (GGUF) vs. Runtime Quantization

When deploying embeddings within the re-Isearch ecosystem, you have two primary paths for handling high-dimensional data: leveraging pre-quantized GGUF/GGML models or using built-in quantization (like our MRLQ/RaBitQ stack) on raw FP32 output.

GGUF/GGML (GPT-Generated Unified Format) models often come pre-baked with quantization levels (e.g., Q4_K_M, Q8_0). Using these provides several distinct advantages:

- Memory-Mapped Speed: GGUF is designed for fast loading via mmap. By using a pre-quantized model, the "heavy lifting" is done at the inference level. The weights are smaller, leading to faster execution on CPU-based SIMD (Neon/AVX) and reduced memory bandwidth pressure.

- Reduced Inference Latency: Since the model itself is smaller, the initial transformation from text to vector happens faster.

- Predictable Accuracy: Pre-quantized models are often calibrated by the community to minimize "perplexity loss" during the quantization process, ensuring the 4-bit or 8-bit output is as representative as possible.


The Trade-off: The "Re-scoring" Bottleneck

While GGUF models are excellent for "inference-to-disk" speed, they introduce a significant constraint for high-precision search: The Loss of the Original Signal.

- No High-Fidelity Re-scoring: In a standard pipeline, we keep a "Golden" FP32 vector to re-score the top 100 results returned by a fast HNSW search. If the model only outputs quantized data (like INT4), you cannot "re-calculate" the distance with higher precision later. You are effectively locked into the resolution of the quantization.

- Fixed Precision: Built-in quantization in the future with MRLQ will allows the engine to dynamically decide the size (64d, 128d, or 384d). With pre-quantized GGUF outputs, you are often stuck with the bit-depth chosen at the time the model was converted.

- Information Ceiling: Once a vector is compressed to X bits at the model level, any noise introduced is permanent. If your HNSW graph returns a false positive, a re-scoring pass using that same X-bit data cannot correct the ranking.



## Pre-Computed Vector Interface

Alongside using Sentence Transformers (S-BERTS) we accept pre-computed vectors as hex-encoded
float32, base64-encoded binary (MongoDB BSON vector subtype 0x09, $binary with subType: "09")
or JSON-like encoded raw float arrays ([0.123, ...]).

Their primary advantage is speed in re-indexing. The overhead of calculating embeddings is shifted onto data creation pipeline (for example JSON records). Decoding the text encoded vectors is fast and adding them to the graph is fast.


Base64 — the most common standard
- MongoDB Atlas Vector Search — BSON binary subtype 0x09, exported as base64 in Extended JSON
- Elasticsearch/OpenSearch — dense_vector fields serialized as base64 in bulk API
- PostgreSQL pgvector — binary protocol uses base64 when exported via JSON
- Google Vertex AI Vector Search — base64 in REST API payloads
- Amazon OpenSearch — base64 for binary vector serialization
- Pinecone — base64 in their JSON export format
- Weaviate — base64 for binary vector fields in GraphQL/REST responses
- BSON/MessagePack — binary types always base64 when rendered to JSON/XML
- Protocol Buffers over HTTP — bytes fields become base64 in JSON transcoding (per proto3 JSON mapping spec)
- JWT — base64url for all binary payloads
- XML Binary — xs:base64Binary is the W3C standard type for binary in XML Schema


Hexdecimal encoding (mainly niche uses)
- re-Isearch pipeline — deliberate choice for human readability and fast validation.
- Some other internal research pipelines — easier to eyeball and debug than base64
- Faiss — when manually serializing index entries for debugging
- GeoJSON — sometimes hex for geometry binary extensions (non-standard)
- Web3 — 0x-prefixed hex is the universal binary encoding convention, including for
embeddings stored on-chain 


Many modern APIs just use JSON float arrays directly and avoid the binary encoding question entirely:
- OpenAI Embeddings API — returns [0.123, -0.456, ...] float arrays
- Hugging Face Inference API — float arrays
- Cohere Embed API — float arrays
- Most LangChain/LlamaIndex integrations — float arrays
- Qdrant — float arrays in REST, binary in gRPC
- Chroma — float arrays

NOTE: re-Isearch now supports a number of JSON types include extended JSON (e.g. MongoDb).

# Interface to re-Isearch

Interface code to re-Isearch is provided by the EmbeddingIndexer class.

It provides the following 3 main methods to re-Isearch:
1) <pre>inline bool Append(const STRING& buffer, const STRING &fieldname, const FC& fc)</pre>
2) <pre>PIRSET  search(const STRING &fieldname, const STRING &query)</pre>
3) <pre>size_t deleteDeleted(const STRING &fieldname)</pre>

If during a HNSW search a deleted record is encountered it get automatically deleted from the HNSW index.  
Should the number of deleted vectors surpass a cutoff, the search is re-run (now with the just encountered deleted elements also marked in the HNSW index as deleted).

Since the HNSW index can return many deleted elements from the perspective of the re-Isearch index, we need to sometimes make sure that the HNSW index also marks the deleted as deleted. That's the function of the 3rd method: literally to delete in the HNSW index the deleted from the re-Isaerch index.

### Startup.

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

=== HNSW Configuration ===
Default search mode: Knn

Index parameters:
  max_elements: 500000
  M: 16
  ef_construction: 200
  ef_search: 64
  specification: L2-None-Pass
  normalize_embeddings: no
  unified_index: no

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
