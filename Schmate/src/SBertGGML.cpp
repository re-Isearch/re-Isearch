// SBertGGML.cpp 

#include "SBertGGML.hpp"
#include <algorithm>
#include <cmath>
#include <thread>

/*

NOTE: In our current design we only want to support a single embedding model for an index.
Allowing for multiple embedding models would vastly increase the memory demands. Our design
goals are to keep the footprint as small as we can to run on the most general hardware that
is suitable. We don't want to raise that bar. 
To allow for caching of SBertGGMLs and a field specific embedding would be relatively easy.
The main hinderness is the need to maintain a mapping of field<->model. A simple key/value
would be sufficient. We are already now anyway storing an identity in the index as search
depends upon having the same model as was used during indexing. 
 

NOTE: bert.cpp:

The library is built on top of ggml (the same backend used by llama.cpp) and is optimized for
models with quantization.

At this time bert.cpp has support for CUDA, Metal, Vulkan and CPU which 95% of use cases.

  - CUDA: Secures absolute peak performance on NVIDIA enterprise and consumer GPUs.
  - Metal: Absolute peak performance on Apple Silicon.
  - Vulkan: Covers AMD GPUs, Intel Arc GPUs, older NVIDIA cards, Android devices,
    and Raspberry Pi clusters. Performance is closing the gap with native backends,
    reducing the need for vendor-specific frameworks.
    See: https://www.youtube.com/watch?v=xaQkt3iWsTQ&t=1783s
    (Vulkanised 2026: Vulkan Machine Learning in ggml/llama.cpp)
  - CPU (with support for SIMD)

The up-to-date platforms are defined in bert.cpp as that is the ground truth.

CPU Hyperthreading consideration:

For CPU BERT (compute-intensive), physical cores often outperform logical threads
If you have 16 logical threads, try 6-10 threads first


System load awareness:

If running other applications: reduce by 20-30%
Dedicated server: can use up to 90-95%


Quick Rules:
Hardware Threads | Recommended for BERT
-----------------|-----------------------
1-2              | 1
4                | 2-3
8                | 4-6
16               | 8-12
32               | 16-24
64+              | 32-48

*/


static int calculate_optimal_threads(
    int hardware_threads = 0,
    int batch_size = 1,
    bool is_cpu_only = true,
    double cpu_utilization_target = 0.85
) {
    static int recommended_threads = 0;

    if (recommended_threads) return recommended_threads;

    if (hardware_threads == 0)
      hardware_threads = std::thread::hardware_concurrency();


    if (hardware_threads <= 0) {
        hardware_threads = std::thread::hardware_concurrency();
        if (hardware_threads == 0) hardware_threads = 1;
    }
    
    // Strategy 1: Leave some threads for system overhead
    // Use 85% of available threads by default
    recommended_threads = static_cast<int>(
        std::ceil(hardware_threads * cpu_utilization_target)
    );
    
    // Strategy 2: Consider hyperthreading
    // For CPU-intensive tasks like BERT inference, using physical cores
    // often performs better than using all logical threads
    int physical_cores = hardware_threads / 2;  // Estimate
    
    // If system has hyperthreading (hardware_threads > 4 and even)
    if (hardware_threads > 4 && hardware_threads % 2 == 0) {
        // For compute-intensive workloads, prefer physical cores
        recommended_threads = std::min(recommended_threads, 
                                      physical_cores + physical_cores / 2);
    }
    
    // Strategy 3: Avoid oversubscription
    // Don't exceed hardware thread count
    recommended_threads = std::min(recommended_threads, hardware_threads);
    
    // Strategy 4: Scale down for small batch sizes
    // Using too many threads for small workloads causes overhead
    if (batch_size == 1 && recommended_threads > 4) {
        recommended_threads = std::min(recommended_threads, 
                                      std::max(4, hardware_threads / 2));
    }
    
    // Strategy 5: Always use at least 1 thread
    recommended_threads = std::max(1, recommended_threads);

//    std::cerr << "Using " << recommended_threads << " threads out of "
//          << hardware_threads << " available\n";
    
    return recommended_threads;
}


#include <chrono>

static int find_best_thread_count(struct bert_ctx * ctx, 
                           const char * test_text,
                           float * embeddings) {
    int hw_threads = std::thread::hardware_concurrency();
    int best_threads = 1;
    double best_time = std::numeric_limits<double>::max();
    
    // Test different thread counts
    std::vector<int> test_configs = {
        1,
        hw_threads / 4,
        hw_threads / 2,
        3 * hw_threads / 4,
        hw_threads
    };
    
    for (int n_threads : test_configs) {
        if (n_threads < 1) continue;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Run multiple iterations for stable measurement
        for (int i = 0; i < 10; i++) {
            bert_encode(ctx, n_threads, test_text, embeddings);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(end - start).count();
        
        std::cout << "Threads: " << n_threads 
                  << " Time: " << elapsed << "s" << std::endl;
        
        if (elapsed < best_time) {
            best_time = elapsed;
            best_threads = n_threads;
        }
    }
    
    return best_threads;
}

#if NEW_BERT_API


void SBertGGML::encode(const char ** texts, float ** embeddings, int n_inputs) {
    int n_threads = calculate_optimal_threads(0, n_inputs);

    // Since the float** embeddings is a pointer to an array of pointers,
    // but the new API expects a flat buffer (float*), we need a temporary flat buffer 
    // OR we loop. 
    
    // Strategy: Flatten for the library, then copy back (or update your class to use flat storage)
    std::vector<float> flat_embeddings(n_inputs * n_embd);
    
    bert_encode_batch_c(ctx, texts, flat_embeddings.data(), n_inputs, n_threads);


    // The new API expects a flat: float * embeddings.
    // Copy from flat back to your float** structure
    for (int i = 0; i < n_inputs; ++i) {
        std::memcpy(embeddings[i], flat_embeddings.data() + (i * n_embd), n_embd * sizeof(float));
    }
}


void SBertGGML::encode(const char * text, float * embedding) {
    // 1. Determine threads (usually 1 or 4 is best for single-string Metal)
    int n_threads = calculate_optimal_threads(1);

    // 2. Use the C-style batch helper with an input count of 1
    // &text passes the address of your pointer, effectively a const char**
    bert_encode_batch_c(ctx, &text, embedding, 1, n_threads);
}


#else


void SBertGGML::encode(const char ** texts, float ** embeddings, int n_inputs, int batch_size) {
    // 1. Calculate threads based on the size of the batch
    int n_threads = calculate_optimal_threads(0, n_inputs);

    if (n_inputs == 1) {
        // Use the simpler single-string API if only one input
        bert_encode(ctx, n_threads, texts[0], embeddings[0]);
    } else {
        // Use the batch API for multiple strings
        // n_batch_size: how many to process in parallel (internal GGML batching)
        // n_inputs: the total number of strings in your 'texts' array
        bert_encode_batch(ctx, n_threads, batch_size, n_inputs, texts, embeddings);
    }
}

void SBertGGML::encode( const char * texts, float * embeddings, int batch_size)
{
    int n_threads = calculate_optimal_threads(0 , batch_size);
    bert_encode(ctx, n_threads, texts, embeddings);
}


void SBertGGML::eval (bert_vocab_id * tokens, int32_t n_tokens, float * embeddings)
{
    int n_threads = calculate_optimal_threads();
    bert_eval(ctx, n_threads, tokens, n_tokens, embeddings);
}
#endif


#if defined(BERT_API_VERSION) &&  (BERT_API_VERSION > 1)
extern "C" {
void schmate_log_callback(int level, const char * text, void * user_data) {
   (void)user_data;
   (void)level; 
   LOG_INFO_S() << text << "\n";
};
};
#endif

SBertGGML::SBertGGML(const std::string & model_path, size_t threads) : _threads(threads) {
#ifdef __APPLE__
       relax_macos_malloc_zones();
#endif

#if defined(BERT_API_VERSION) &&   (BERT_API_VERSION > 1)
        ggml_log_set(schmate_log_callback, nullptr);
#endif

	clock_t start = clock();
        ctx = bert_load_from_file(model_path.c_str());
        if (!ctx) throw std::runtime_error("Failed to load model " + model_path);
        name = basename(model_path); // Get the name
        n_embd = bert_n_embd(ctx);
	clock_t end = clock();
	const double factor = 1000.0/CLOCKS_PER_SEC;
        const double cpu_total = end > start ? (end - start)*factor : 0.0;

#if defined(BERT_API_VERSION) &&   (BERT_API_VERSION > 1)
        { const char *ptr = bert_get_model_name(ctx);
	  if (ptr) name = ptr;
	  ptr = bert_get_architecture(ctx);
	  if (ptr) arch = ptr;
        }
#else
        arch = "bert";
#endif
        LOG_INFO_S() << "Loaded SBERT GGML model. dim=" << n_embd << " (" << cpu_total << "ms)" ;

}



/*


std::vector<float> execute_embedding(const std::string& raw_text, const GGUFInfo& info, bool is_query) {
    std::string processed_text = raw_text;

    // Handle task prefixes at the orchestration layer
    if (is_query) {
        if (info.architecture == "nomic-bert-moe" || info.architecture == "nomic-bert") {
            processed_text = "search_query: " + raw_text;
        } 
        else if (info.quant_type.find("bge-large") != std::string::npos) { 
            // Fallback check if your model name/metadata indicates BGE v1.5
            processed_text = "Represent this sentence for searching relevant passages: " + raw_text;
        }
    } else {
        // If it's a document/passage insertion into your HNSW net
        if (info.architecture == "nomic-bert-moe" || info.architecture == "nomic-bert") {
            processed_text = "search_document: " + raw_text;
        }
        // BGE v1.5 needs NO prefix for documents, so it stays raw_text
    }

    // Now route the processed_text safely to the correct engine
    if (info.is_bert_cpp_compatible) {
        return bert_cpp_encode(processed_text);
    } else {
        return llama_cpp_encode(processed_text);
    }
}

*/
