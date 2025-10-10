// SBertGGML.cpp 

#include "SBertGGML.hpp"
#include <algorithm>
#include <cmath>
#include <thread>

/*

NOTE: bert.cpp:

CPU-focused: The library is built on top of ggml (the same backend used by llama.cpp) and is
optimized for CPU inference with quantization

No native GPU support: The current implementation doesn't include CUDA or other GPU acceleration
Designed for efficiency on CPU: Uses 4-bit quantization to make models very small and fast on CPU

If you need GPU (e.g. for NVIDIA Jetsons) acceleration for BERT, consider cuBERT - CUDA-optimized
BERT implementation

Since it's CPU-only a thread optimization strategy has been selected as the best approach. 

General Guidelines:

Single text encoding: Use 25-50% of hardware threads (max 4-8 threads)

Small workloads don't benefit from high parallelism
Thread overhead dominates


Batch processing: Use 70-85% of hardware threads

Leave 1-2 cores for OS and other processes
Example: 16 threads → use 12-14 threads


Hyperthreading consideration:

For BERT (compute-intensive), physical cores often outperform logical threads
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

void SBertGGML::encode( const char * texts, float * embeddings, int batch_size)
{
    int n_threads = calculate_optimal_threads(0 , batch_size);
    
    bert_encode(ctx, n_threads, texts, embeddings);
}

void SBertGGML::eval (bert_vocab_id * tokens, int32_t n_tokens, float * embeddings)
{
    int n_threads = calculate_optimal_threads();

    return bert_eval(ctx, n_threads, tokens, n_tokens, embeddings);
}


