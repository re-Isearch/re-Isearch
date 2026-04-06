#include "unified_hnsw.hpp"
#include <iostream>
#include <vector>
#include <random>
#include <cmath>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <set>

// Test configuration
struct TestConfig {
    size_t dim = 384;
    size_t num_vectors = 10000;
    size_t num_queries = 100;
    size_t k = 5;
    size_t ef_construction = 200;
    size_t ef_search = 300; // 50;
    size_t M = 16; // 16;
    
    // Ground truth parameters
    bool compute_ground_truth = true;
    size_t ground_truth_ef = 500;  // Higher ef for accurate ground truth
};

// Test results
struct TestResult {
    std::string test_name;
    hnswlib::Metric metric;
    bool quantized;
    hnswlib::QuantMode quant_mode;
    hnswlib::OptBinMode bin_mode;
    
    double build_time_ms;
    double query_time_ms;
    double recall_at_k;
    size_t bytes_per_vector;
    bool save_load_success;
    
    void print() const {
        std::cout << "\n=== " << test_name << " ===\n";
        std::cout << "Metric: " << metric_to_string(metric) << "\n";
        std::cout << "Quantized: " << (quantized ? "Yes" : "No") << "\n";
        if (quantized) {
            std::cout << "Quant Mode: " << quant_mode_to_string(quant_mode) << "\n";
            std::cout << "Bin Mode: " << bin_mode_to_string(bin_mode) << "\n";
        }
        std::cout << "Build Time: " << std::fixed << std::setprecision(2) << build_time_ms << " ms\n";
        std::cout << "Query Time: " << std::fixed << std::setprecision(2) << query_time_ms << " ms\n";
        std::cout << "Recall@" << 10 << ": " << std::fixed << std::setprecision(4) << recall_at_k << "\n";
        std::cout << "Bytes/Vector: " << bytes_per_vector << "\n";
        std::cout << "Save/Load: " << (save_load_success ? "PASS" : "FAIL") << "\n";
    }
    
    static std::string metric_to_string(hnswlib::Metric m) {
        switch(m) {
            case hnswlib::Metric::L2: return "L2";
            case hnswlib::Metric::IP: return "IP";
            case hnswlib::Metric::Cosine: return "Cosine";
            case hnswlib::Metric::L1: return "L1";
            default: return "Unknown";
        }
    }
    
    static std::string quant_mode_to_string(hnswlib::QuantMode m) {
        switch(m) {
            case hnswlib::QuantMode::BIN1:  return "BIN1";
            case hnswlib::QuantMode::INT158:return "INT158";
            case hnswlib::QuantMode::INT4:  return "INT4";
            case hnswlib::QuantMode::INT8:  return "INT8";
	    case hnswlib::QuantMode::NONE:  return "Float32";
            default: return "Unknown";
        }
    }
    
    static std::string bin_mode_to_string(hnswlib::OptBinMode m) {
        switch(m) {
            case hnswlib::OptBinMode::PASS: return "PASS";
            case hnswlib::OptBinMode::STANDARD: return "STANDARD";
            case hnswlib::OptBinMode::BETTER: return "BETTER";
            case hnswlib::OptBinMode::CENTROID: return "CENTROID";
            case hnswlib::OptBinMode::ROTATIONAL: return "ROTATIONAL";
            case hnswlib::OptBinMode::RABITQ: return "RABITQ";
            case hnswlib::OptBinMode::RABITQ_EXTENDED: return "RABITQ_EXTENDED";
            default: return "Unknown";
        }
    }
};

// Generate random vectors
std::vector<std::vector<float>> generate_random_vectors(size_t n, size_t dim, int seed = 42) {
    std::mt19937 rng(seed);
    std::normal_distribution<float> dist(0.0f, 1.0f);
    
    std::vector<std::vector<float>> vectors(n, std::vector<float>(dim));
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < dim; ++j) {
            vectors[i][j] = dist(rng);
        }
    }
    return vectors;
}

// Normalize vector in-place
void normalize_vector(std::vector<float>& vec) {
#if 1
    hnswlib::normalize_l2(vec.data(), vec.size());
#else
    float norm = 0.0f;
    for (float v : vec) norm += v * v;
    norm = std::sqrt(norm);
    if (norm > 1e-9f) {
        for (float& v : vec) v /= norm;
    }
#endif
}

// Compute ground truth using brute force
std::vector<std::vector<hnswlib::labeltype>> compute_ground_truth(
    const std::vector<std::vector<float>>& data,
    const std::vector<std::vector<float>>& queries,
    size_t k,
    hnswlib::Metric metric,
    bool normalize = false) {
    
//    std::cout << "Computing ground truth..." << std::flush;
    
    std::vector<std::vector<hnswlib::labeltype>> ground_truth(queries.size());
    
    for (size_t q = 0; q < queries.size(); ++q) {
        std::vector<std::pair<float, hnswlib::labeltype>> distances;
        distances.reserve(data.size());
        
        std::vector<float> query = queries[q];
        if (normalize) normalize_vector(query);
        
        for (size_t i = 0; i < data.size(); ++i) {
            std::vector<float> vec = data[i];
            if (normalize) normalize_vector(vec);
            
            float dist = 0.0f;
            if (metric == hnswlib::Metric::L2) {
                for (size_t d = 0; d < vec.size(); ++d) {
                    float diff = query[d] - vec[d];
                    dist += diff * diff;
                }
            } else if (metric == hnswlib::Metric::IP) {
                for (size_t d = 0; d < vec.size(); ++d) {
                    dist += query[d] * vec[d];
                }
                dist = 1.0f - dist;  // Convert to distance
            } else if (metric == hnswlib::Metric::Cosine) {
                float dot = 0.0f, norm_q = 0.0f, norm_v = 0.0f;
                for (size_t d = 0; d < vec.size(); ++d) {
                    dot += query[d] * vec[d];
                    norm_q += query[d] * query[d];
                    norm_v += vec[d] * vec[d];
                }
                dist = 1.0f - (dot / (std::sqrt(norm_q * norm_v) + 1e-9f));
            }
            
            distances.push_back({dist, static_cast<hnswlib::labeltype>(i)});
        }
        
        // Sort and get top-k
        std::partial_sort(distances.begin(), 
                         distances.begin() + k,
                         distances.end(),
                         [](const auto& a, const auto& b) { return a.first < b.first; });
        
        ground_truth[q].reserve(k);
        for (size_t i = 0; i < k; ++i) {
            ground_truth[q].push_back(distances[i].second);
        }
    }
    
//    std::cout << " done\n";
    return ground_truth;
}

// Calculate recall@k
#if 1

double calculate_recall(
    const std::vector<std::vector<hnswlib::labeltype>>& results,
    const std::vector<std::vector<hnswlib::labeltype>>& ground_truth,
    size_t k) 
{
    double total_recall = 0.0;

    for (size_t q = 0; q < results.size(); ++q) {

        size_t kk = std::min(k, ground_truth[q].size());
        if (kk == 0) continue;   // no ground truth → skip

        std::unordered_set<hnswlib::labeltype> gt_set(
            ground_truth[q].begin(),
            ground_truth[q].begin() + kk
        );

        size_t hits = 0;
        size_t rk = std::min(k, results[q].size());

        for (size_t i = 0; i < rk; i++) {
            if (gt_set.count(results[q][i])) {
                hits++;
            }
        }

        total_recall += double(hits) / double(kk);
    }

    return total_recall / results.size();
}


#else
double calculate_recall(
    const std::vector<std::vector<hnswlib::labeltype>>& results,
    const std::vector<std::vector<hnswlib::labeltype>>& ground_truth,
    size_t k) {
    
    double total_recall = 0.0;
    
    for (size_t q = 0; q < results.size(); ++q) {
        std::set<hnswlib::labeltype> gt_set(ground_truth[q].begin(), 
                                             ground_truth[q].begin() + std::min(k, ground_truth[q].size()));
        
        size_t hits = 0;
        for (size_t i = 0; i < std::min(k, results[q].size()); ++i) {
            if (gt_set.count(results[q][i])) {
                hits++;
            }
        }
        
        total_recall += static_cast<double>(hits) / std::min(k, ground_truth[q].size());
    }
    
    return total_recall / results.size();
}

#endif

// Run a single test
TestResult run_test(
    const TestConfig& config,
    const std::vector<std::vector<float>>& data,
    const std::vector<std::vector<float>>& queries,
    const std::vector<std::vector<hnswlib::labeltype>>& ground_truth,
    const std::string& test_name,
    hnswlib::Metric metric,
    bool rescore,
    hnswlib::QuantMode quant_mode = hnswlib::QuantMode::BIN1,
    hnswlib::OptBinMode bin_mode = hnswlib::OptBinMode::STANDARD) {

    TestResult result;
    result.test_name = test_name;
    result.metric = metric;
    bool use_quantization = result.quantized = bin_mode != hnswlib::OptBinMode::PASS; 
    result.quant_mode = use_quantization ? quant_mode :  hnswlib::QuantMode::NONE;
    result.bin_mode = use_quantization ?  bin_mode : hnswlib::OptBinMode::PASS;

    bool enable_rescoring = rescore; // use_quantization;
    
    try {
        // Create index
        hnswlib::UnifiedIndex index(
            config.dim,
            config.num_vectors,
            result.metric,
            result.quant_mode,
            result.bin_mode,
            enable_rescoring,
            config.M,
            config.ef_construction
        );
        
        index.setEf(config.ef_search);
        
        // Build index
        auto build_start = std::chrono::high_resolution_clock::now();
        
        // Add training samples for quantization
        if (use_quantization && data.size() >= 100) {
            std::vector<std::vector<float>> training_samples(
                data.begin(), 
                data.begin() + std::min(size_t(1000), data.size())
            );
            index.fit(training_samples);
        }
        
        // Add all vectors
        for (size_t i = 0; i < data.size(); ++i) {
            index.addPoint(data[i].data(), i);
        }
        
        auto build_end = std::chrono::high_resolution_clock::now();
        result.build_time_ms = std::chrono::duration<double, std::milli>(build_end - build_start).count();
        
        // Query
        std::vector<std::vector<hnswlib::labeltype>> results(queries.size());
        
        auto query_start = std::chrono::high_resolution_clock::now();
        
std::cerr << "Run the queries\n";
        for (size_t q = 0; q < queries.size(); ++q) {
            auto p= index.searchKnn(queries[q].data(), config.k);
	    auto res = index.sort_best_first(p);
	    // res is already sorted best → worst. No need to reverse.
            results[q].reserve(res.size());

//          std::cout << "Result list: " << std::endl;
            for (const auto& p : res) {
                float dist = p.first;
                size_t label = p.second;
//                float score = index.score_from_dist(dist);
//		std::cout << "Label#" << label << " score=" << score << "   ";
                results[q].push_back(label);
            }
	    // std::cout << std::endl;
        }
        
        auto query_end = std::chrono::high_resolution_clock::now();
        result.query_time_ms = std::chrono::duration<double, std::milli>(query_end - query_start).count();
        
std::cerr << "Calc recall\n";
        // Calculate recall
        if (!ground_truth.empty()) {
            result.recall_at_k = calculate_recall(results, ground_truth, config.k);
        } else {
            result.recall_at_k = -1.0;  // Not computed
        }
        
        // Get bytes per vector
        result.bytes_per_vector = index.bytes_per_vector();
        
        // Test save/load
        std::string temp_file = "/tmp/temp_index.bin";
        result.save_load_success = false;

        // index.printStats();
        
        if (index.saveIndex(temp_file)) {
            hnswlib::UnifiedIndex loaded_index(
                config.dim,
                config.num_vectors,
                metric,
                quant_mode,
                bin_mode,
                rescore,
                config.M,
                config.ef_construction
            );
std::cerr << "NOW LOAD" << std::endl;

            if (loaded_index.loadIndex(temp_file)) {
                // Verify loaded index works
              auto p = loaded_index.searchKnn(queries[0].data(), config.k);
	      auto test_result = index.sort_best_first(p);
              result.save_load_success = (test_result.size() == config.k);
            }
        }

std::cerr << "Cleanup" << std::endl;
        // Cleanup
        std::remove(temp_file.c_str());
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << "\n";
        result.recall_at_k = -1.0;
        result.save_load_success = false;
    }
    
    return result;
}

// Print summary table
void print_summary(const std::vector<TestResult>& results) {
    std::cout << "\n\n";
    std::cout << "========================================================================================================\n";
    std::cout << "                                      TEST SUMMARY                                                      \n";
    std::cout << "========================================================================================================\n";
    std::cout << std::left 
              << std::setw(35) << "Test"
              << std::setw(10) << "Metric"
              << std::setw(12) << "Build(ms)"
              << std::setw(12) << "Query(ms)"
              << std::setw(12) << "Recall@"
              << std::setw(10) << "Bytes/Vec"
              << std::setw(10) << "Save/Load"
              << "\n";
    std::cout << "--------------------------------------------------------------------------------------------------------\n";
    
    for (const auto& result : results) {
        std::cout << std::left
                  << std::setw(35) << result.test_name
                  << std::setw(10) << TestResult::metric_to_string(result.metric)
                  << std::setw(12) << std::fixed << std::setprecision(1) << result.build_time_ms
                  << std::setw(12) << std::fixed << std::setprecision(2) << result.query_time_ms
                  << std::setw(12) << std::fixed << std::setprecision(4) 
                  << (result.recall_at_k >= 0 ? std::to_string(result.recall_at_k) : "N/A")
                  << std::setw(10) << result.bytes_per_vector
                  << std::setw(10) << (result.save_load_success ? "PASS" : "FAIL")
                  << "\n";
    }
    std::cout << "========================================================================================================\n";
}

int main() {
    TestConfig config;
    std::vector<TestResult> all_results;
    
    std::cout << "===========================================\n";
    std::cout << "  UnifiedIndex Exhaustive Test Suite\n";
    std::cout << "===========================================\n\n";
    std::cout << "Configuration:\n";
    std::cout << "  Dimensions: " << config.dim << "\n";
    std::cout << "  Vectors: " << config.num_vectors << "\n";
    std::cout << "  Queries: " << config.num_queries << "\n";
    std::cout << "  k: " << config.k << "\n";
    std::cout << "  M: " << config.M << "\n";
    std::cout << "  ef_construction: " << config.ef_construction << "\n";
    std::cout << "  ef_search: " << config.ef_search << "\n\n";
    
    // Generate data
    std::cout << "Generating data...\n";
    auto data = generate_random_vectors(config.num_vectors, config.dim);
    auto queries = generate_random_vectors(config.num_queries, config.dim, 123);
    
    // Test configurations
    std::vector<hnswlib::Metric> metrics = {
        hnswlib::Metric::L2,
        hnswlib::Metric::IP,
        hnswlib::Metric::Cosine
    };
    
    std::vector<hnswlib::QuantMode> quant_modes = {
        hnswlib::QuantMode::BIN1,
        hnswlib::QuantMode::INT4,
        hnswlib::QuantMode::INT8
    };
    
    std::vector<hnswlib::OptBinMode> bin_modes = {
        hnswlib::OptBinMode::STANDARD,
//        hnswlib::OptBinMode::BETTER,
//        hnswlib::OptBinMode::CENTROID,
        hnswlib::OptBinMode::RABITQ
    };
    
    for (auto metric : metrics) {
        std::cout << "\n\n========== Testing " << TestResult::metric_to_string(metric) << " ==========\n";
        
        // Compute ground truth
        std::vector<std::vector<hnswlib::labeltype>> ground_truth;
        if (config.compute_ground_truth) {
            ground_truth = compute_ground_truth(data, queries, config.k, metric, 
                                                metric == hnswlib::Metric::Cosine);
        }
        
        // Test non-quantized
        std::cout << "\nTesting non-quantized...\n";
        auto result = run_test(config, data, queries, ground_truth, "Float32",
		metric, false, hnswlib::QuantMode::NONE, hnswlib::OptBinMode::PASS);

        result.print();
        all_results.push_back(result);
        
        // Test quantized versions

      
        for (auto qmode : quant_modes) {
            
            for (auto bmode : bin_modes) {
                // Skip invalid combinations
                if (qmode != hnswlib::QuantMode::BIN1 && 
                    (bmode == hnswlib::OptBinMode::BETTER || 
                     bmode == hnswlib::OptBinMode::RABITQ)) {
                    continue;
                }


                
                std::string test_name = TestResult::quant_mode_to_string(qmode) + "-" +
                                       TestResult::bin_mode_to_string(bmode);
                
                auto result = run_test(config, data, queries, ground_truth, test_name, metric, true, qmode, bmode);
                result.print();
                all_results.push_back(result);
            }
        }
    }
    
    // Print summary
    print_summary(all_results);
    
    // Identify best configurations
    std::cout << "\n\n=== RECOMMENDATIONS ===\n";
    
    for (auto metric : metrics) {
        std::cout << "\nFor " << TestResult::metric_to_string(metric) << ":\n";
        
        // Find best recall
        auto best_recall = std::max_element(all_results.begin(), all_results.end(),
            [metric](const TestResult& a, const TestResult& b) {
                if (a.metric != metric || b.metric != metric) return false;
                return a.recall_at_k < b.recall_at_k;
            });
        
        if (best_recall != all_results.end() && best_recall->metric == metric) {
            std::cout << "  Best Recall: " << best_recall->test_name 
                     << " (" << std::fixed << std::setprecision(4) << best_recall->recall_at_k << ")\n";
        }
        
        // Find best speed (quantized only)
        auto best_speed = std::min_element(all_results.begin(), all_results.end(),
            [metric](const TestResult& a, const TestResult& b) {
                if (a.metric != metric || b.metric != metric) return false;
                if (!a.quantized || !b.quantized) return false;
                return a.query_time_ms < b.query_time_ms;
            });
        
        if (best_speed != all_results.end() && best_speed->metric == metric && best_speed->quantized) {
            std::cout << "  Fastest Query: " << best_speed->test_name 
                     << " (" << std::fixed << std::setprecision(2) << best_speed->query_time_ms << " ms)\n";
        }
        
        // Find smallest memory (quantized only)
        auto smallest = std::min_element(all_results.begin(), all_results.end(),
            [metric](const TestResult& a, const TestResult& b) {
                if (a.metric != metric || b.metric != metric) return false;
                if (!a.quantized || !b.quantized) return false;
                return a.bytes_per_vector < b.bytes_per_vector;
            });
        
        if (smallest != all_results.end() && smallest->metric == metric && smallest->quantized) {
            std::cout << "  Smallest Memory: " << smallest->test_name 
                     << " (" << smallest->bytes_per_vector << " bytes/vec)\n";
        }
    }
    
    std::cout << "\nAll tests completed!\n";
    
    return 0;
}
