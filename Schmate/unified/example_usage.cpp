// To use: Place unified_hnsw.hpp in same directory as this file
// Compile: g++ -std=c++17 -O3 -march=native -I. -I/path/to/hnswlib example_usage.cpp -o example

#include "unified_hnsw.hpp"
#include <iostream>
#include <cstdlib>

int main() {
    const size_t dim = 384;
    const size_t max_elements = 1000;
    const size_t k = 10;
    
    // Detect SIMD
    auto simd = hnswlib::detect_simd();
    std::cout << "Using ";
    switch (simd) {
        case hnswlib::SimdKind::AVX512: std::cout << "AVX-512"; break;
        case hnswlib::SimdKind::AVX2: std::cout << "AVX2"; break;
        case hnswlib::SimdKind::NEON: std::cout << "ARM NEON"; break;
        default: std::cout << "scalar (no SIMD)"; break;
    }
    std::cout << " SIMD instructions\n\n";
    
    // Generate sample data
    std::vector<std::vector<float>> sample_data(max_elements, std::vector<float>(dim));
    for (auto& vec : sample_data) {
        for (auto& val : vec) {
            val = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
        }
    }
    
    std::vector<float> query(dim);
    for (auto& val : query) {
        val = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
    }
    
    // Example 1: Float index with L2 distance
    std::cout << "=== Float Index (L2 Distance) ===\n";
    hnswlib::UnifiedIndex float_index(dim, max_elements, hnswlib::Metric::L2);
    
    for (size_t i = 0; i < max_elements; i++) {
        float_index.addPoint(sample_data[i].data(), i);
    }
    
    float_index.setEf(50);
    auto results_float = float_index.searchKnn(query.data(), k);
    
    std::cout << "Top " << k << " results:\n";
    while (!results_float.empty()) {
        auto [dist, label] = results_float.top();
        std::cout << "  Label: " << label << ", Distance: " << dist << "\n";
        results_float.pop();
    }
    
    // Example 2: Float index with Cosine similarity
    std::cout << "\n=== Float Index (Cosine Similarity) ===\n";
    hnswlib::UnifiedIndex cosine_index(dim, max_elements, hnswlib::Metric::Cosine);
    
    for (size_t i = 0; i < max_elements; i++) {
        cosine_index.addPoint(sample_data[i].data(), i);
    }
    
    cosine_index.setEf(50);
    auto results_cosine = cosine_index.searchKnn(query.data(), k);
    
    std::cout << "Top " << k << " results:\n";
    while (!results_cosine.empty()) {
        auto [dist, label] = results_cosine.top();
        std::cout << "  Label: " << label << ", Distance: " << dist << "\n";
        results_cosine.pop();
    }
    
    // Example 3: Binary quantized index (STANDARD mode)
    std::cout << "\n=== Binary Quantized Index (STANDARD Mode) ===\n";
    hnswlib::UnifiedIndex binary_standard(
        dim, max_elements, 
        hnswlib::Metric::L2,
        hnswlib::QuantMode::BIN1,
        hnswlib::OptBinMode::STANDARD,
        false  // no rescoring
    );
    
    binary_standard.fit(sample_data);
    
    for (size_t i = 0; i < max_elements; i++) {
        binary_standard.addPoint(sample_data[i].data(), i);
    }
    
    binary_standard.setEf(50);
    auto results_binary_std = binary_standard.searchKnn(query.data(), k);
    
    std::cout << "Top " << k << " results:\n";
    while (!results_binary_std.empty()) {
        auto [dist, label] = results_binary_std.top();
        std::cout << "  Label: " << label << ", Hamming Distance: " << dist << "\n";
        results_binary_std.pop();
    }

    // Example 4: Binary quantized index (RABITQ mode)
    std::cout << "\n=== Binary Quantized Index (RABITQ Mode) ===\n";
    hnswlib::UnifiedIndex binary_rabitq(
        dim, max_elements,
        hnswlib::Metric::L2,
        hnswlib::QuantMode::BIN1,
        hnswlib::OptBinMode::RABITQ,
        false  // no rescoring
    );

    binary_rabitq.fit(sample_data);

    for (size_t i = 0; i < max_elements; i++) {
        binary_rabitq.addPoint(sample_data[i].data(), i);
    }
   
    binary_rabitq.setEf(50);
    auto results_binary_rabitq = binary_rabitq.searchKnn(query.data(), k);

    std::cout << "Top " << k << " results:\n";
    while (!results_binary_rabitq.empty()) {
        auto [dist, label] = results_binary_rabitq.top();
        std::cout << "  Label: " << label << ", Hamming Distance: " << dist << "\n";
        results_binary_rabitq.pop();
    }
   

    
    // Example 5: Binary quantized index (BETTER mode with rescoring)
    std::cout << "\n=== Binary Quantized Index (BETTER Mode with Rescoring) ===\n";
    hnswlib::UnifiedIndex binary_better(
        dim, max_elements, 
        hnswlib::Metric::Cosine,
        hnswlib::QuantMode::BIN1,
        hnswlib::OptBinMode::BETTER,
        true  // enable rescoring
    );
    
    binary_better.fit(sample_data);
    
    for (size_t i = 0; i < max_elements; i++) {
        binary_better.addPoint(sample_data[i].data(), i);
    }
    
    binary_better.setEf(50);
    
    std::cout << "Binary search (fast):\n";
    auto results_binary_fast = binary_better.searchKnn(query.data(), k, false);
    while (!results_binary_fast.empty()) {
        auto [dist, label] = results_binary_fast.top();
        std::cout << "  Label: " << label << ", Distance: " << dist << "\n";
        results_binary_fast.pop();
    }
    
    std::cout << "\nRescored search (accurate):\n";
    auto results_binary_rescore = binary_better.searchKnn(query.data(), k, true);
    while (!results_binary_rescore.empty()) {
        auto [dist, label] = results_binary_rescore.top();
        std::cout << "  Label: " << label << ", Distance: " << dist << "\n";
        results_binary_rescore.pop();
    }
    
    // Example 6: Ternary quantized index
    std::cout << "\n=== Ternary Quantized Index ===\n";
    hnswlib::UnifiedIndex ternary_index(
        dim, max_elements,
        hnswlib::Metric::L2,
        hnswlib::QuantMode::INT158,
        hnswlib::OptBinMode::STANDARD,
        false
    );
    
    ternary_index.fit(sample_data);
    
    for (size_t i = 0; i < max_elements; i++) {
        ternary_index.addPoint(sample_data[i].data(), i);
    }
    
    ternary_index.setEf(50);
    auto results_ternary = ternary_index.searchKnn(query.data(), k);
    
    std::cout << "Top " << k << " results:\n";
    while (!results_ternary.empty()) {
        auto [dist, label] = results_ternary.top();
        std::cout << "  Label: " << label << ", Distance: " << dist << "\n";
        results_ternary.pop();
    }
    
    // Example 7: Search with epsilon stop condition
    std::cout << "\n=== Search with Epsilon Stop Condition ===\n";
    float epsilon = 0.1f;
    size_t min_cand = 5;
    size_t max_cand = 100;
    
    auto epsilon_results = binary_better.searchWithStopCondition(
        query.data(), epsilon, min_cand, max_cand);
    
    std::cout << "Found " << epsilon_results.size() << " results within epsilon " << epsilon << "\n";
    std::cout << "(min_cand=" << min_cand << ", max_cand=" << max_cand << ")\n";
    for (size_t i = 0; i < std::min(size_t(10), epsilon_results.size()); i++) {
        std::cout << "  Label: " << epsilon_results[i].second 
                  << ", Distance: " << epsilon_results[i].first << "\n";
    }
    
    // Save indices
    std::cout << "\n=== Saving Indices ===\n";
    float_index.saveIndex("unified_float_index.bin");
    cosine_index.saveIndex("unified_cosine_index.bin");
    binary_standard.saveIndex("unified_binary_standard.bin");
    binary_rabitq.saveIndex("unified_binary.rabitq.bin");
    binary_better.saveIndex("unified_binary_better.bin");
    ternary_index.saveIndex("unified_ternary_index.bin");
    std::cout << "All indices saved successfully!\n";
    
    // Example 8: Load and verify
    std::cout << "\n=== Loading Binary Better Index ===\n";

    hnswlib::UnifiedIndex loaded_index(
        dim, max_elements,
        hnswlib::Metric::Cosine,
        hnswlib::QuantMode::BIN1,
        hnswlib::OptBinMode::BETTER,
        true
    );
    
    loaded_index.loadIndex("unified_binary_better.bin");
    
    std::cout << "Loaded index with " << loaded_index.getCurrentElementCount() << " elements\n";
    std::cout << "Is quantized: " << (loaded_index.is_quantized() ? "Yes" : "No") << "\n";
    std::cout << "Is binary: " << (loaded_index.is_binary() ? "Yes" : "No") << "\n";
    std::cout << "Is ternary: " << (loaded_index.is_ternary() ? "Yes" : "No") << "\n";
    std::cout << "Rescoring enabled: " << (loaded_index.is_rescoring_enabled() ? "Yes" : "No") << "\n";
    
    loaded_index.setEf(50);
    auto results_loaded = loaded_index.searchKnn(query.data(), k, true);
    
    std::cout << "\nSearch results from loaded index:\n";
    while (!results_loaded.empty()) {
        auto [dist, label] = results_loaded.top();
        std::cout << "  Label: " << label << ", Distance: " << dist << "\n";
        results_loaded.pop();
    }
    
    // Summary
    std::cout << "\n=== Summary ===\n";
    std::cout << "Supported features:\n";
    std::cout << "  ✓ Metrics: L1, L2, IP, Cosine\n";
    std::cout << "  ✓ Quantization: None, Binary, Ternary\n";
    std::cout << "  ✓ Optional Normalization (for cosine similarity in quantized space)\n";
    std::cout << "  ✓ Binary modes: STANDARD (median), BETTER (correlation-optimized)\n";
    std::cout << "  ✓ Rescoring: Optional with original vectors\n";
    std::cout << "  ✓ Epsilon stop condition: searchWithStopCondition()\n";
    std::cout << "  ✓ SIMD: AVX2, AVX-512, ARM NEON, ARM SVE\n";
    std::cout << "  ✓ Single file storage with stream I/O\n";
    std::cout << "\nEach index file contains:\n";
    std::cout << "  1. Magic header ('WSHN' for 64-bit,'HNSW' for 32-bit)\n";
    std::cout << "  2. Version number\n";
    std::cout << "  3. Full metadata\n";
    std::cout << "  4. Quantizer thresholds (if quantized)\n";
    std::cout << "  5. Original vectors (if rescoring enabled)\n";
    std::cout << "  6. HNSW graph structure\n";
    std::cout << "\nAll in a single file!\n";
    
    return 0;
}
