#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>
#include <map>
#include <set>
#include <algorithm>
#include "hnswlib.h"
#include "space_quantized.h"

using namespace std;

// Generate random vectors with clusters for better distance variance
vector<vector<float>> generate_data(size_t n, size_t dim, int seed = 42) {
    mt19937 rng(seed);
    normal_distribution<float> dist(0.0f, 1.0f);
    
    // Create 10 cluster centers
    const size_t n_clusters = 10;
    vector<vector<float>> centers(n_clusters, vector<float>(dim));
    for (size_t c = 0; c < n_clusters; ++c) {
        for (size_t d = 0; d < dim; ++d) {
            centers[c][d] = dist(rng) * 3.0f;  // Spread centers further apart
        }
    }
    
    vector<vector<float>> data(n, vector<float>(dim));
    uniform_int_distribution<size_t> cluster_dist(0, n_clusters - 1);
    normal_distribution<float> noise(0.0f, 0.3f);  // Smaller noise within clusters
    
    for (size_t i = 0; i < n; ++i) {
        size_t cluster = cluster_dist(rng);
        for (size_t d = 0; d < dim; ++d) {
            data[i][d] = centers[cluster][d] + noise(rng);
        }
        
        // Normalize to unit length
        float norm = 0;
        for (size_t d = 0; d < dim; ++d) {
            norm += data[i][d] * data[i][d];
        }
        norm = sqrt(norm);
        if (norm > 0) {
            for (size_t d = 0; d < dim; ++d) {
                data[i][d] /= norm;
            }
        }
    }
    
    // Debug: Check first vector
    if (n > 0) {
        float check_norm = 0;
        for (size_t d = 0; d < dim; ++d) {
            check_norm += data[0][d] * data[0][d];
        }
        cout << "  (First vector norm: " << sqrt(check_norm) << ", " << n_clusters << " clusters)" << endl;
    }
    
    return data;
}

// Compute ground truth using the quantized space distance
vector<vector<size_t>> compute_ground_truth_quantized(
    const vector<vector<float>>& data,
    const vector<vector<float>>& queries,
    hnswlib::SpaceQuantized<float>* space,
    size_t k) {
    
    vector<vector<size_t>> ground_truth(queries.size());
    
    // Quantize all data
    vector<vector<uint8_t>> quantized_data(data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        quantized_data[i].resize(space->get_data_size());
        space->quantize(data[i].data(), quantized_data[i].data());
    }
    
    for (size_t q = 0; q < queries.size(); ++q) {
        // Quantize query
        vector<uint8_t> query_code(space->get_data_size());
        space->quantize(queries[q].data(), query_code.data());
        
        vector<pair<float, size_t>> distances;
        
        for (size_t i = 0; i < data.size(); ++i) {
            float dist = space->get_dist_func()(
                query_code.data(),
                quantized_data[i].data(),
                space->get_dist_func_param()
            );
            distances.push_back({dist, i});
        }
        
        partial_sort(distances.begin(), 
                    distances.begin() + k, 
                    distances.end());
        
        ground_truth[q].resize(k);
        for (size_t i = 0; i < k; ++i) {
            ground_truth[q][i] = distances[i].second;
        }
    }
    
    return ground_truth;
}

// Calculate recall@k
float calculate_recall(
    const vector<vector<size_t>>& results,
    const vector<vector<size_t>>& ground_truth) {
    
    size_t total_found = 0;
    size_t total_expected = 0;
    
    for (size_t q = 0; q < results.size(); ++q) {
        set<size_t> gt_set(ground_truth[q].begin(), ground_truth[q].end());
        
        for (size_t i = 0; i < results[q].size(); ++i) {
            if (gt_set.count(results[q][i])) {
                total_found++;
            }
        }
        total_expected += ground_truth[q].size();
    }
    
    return float(total_found) / float(total_expected);
}

struct BenchmarkResult {
    string mode_name;
    float build_time_ms;
    float query_time_ms;
    float recall;
    size_t bytes_per_vector;
    size_t total_memory_kb;
};

BenchmarkResult run_benchmark(
    const vector<vector<float>>& data,
    const vector<vector<float>>& queries,
    hnswlib::QuantMode qmode,
    hnswlib::OptBinMode bin_mode,
    const string& mode_name,
    size_t k = 10,
    size_t M = 32,
    size_t ef_construction = 400,
    size_t ef_search = 400) {
    
    BenchmarkResult result;
    result.mode_name = mode_name;
    
    size_t dim = data[0].size();
    size_t n = data.size();
    
    // Prepare training samples (10% of data)
    size_t n_train = n / 10;
    vector<vector<float>> train_samples(
        data.begin(), 
        data.begin() + n_train
    );
    
    cout << "  Building index for " << mode_name << "..." << flush;
    
    // Create quantized space
    auto* space = new hnswlib::SpaceQuantized<float>(
        dim, qmode, bin_mode, &train_samples, 1000
    );
    
    result.bytes_per_vector = space->get_data_size();
    result.total_memory_kb = (n * result.bytes_per_vector) / 1024;
    
    // Debug: Check quantized value distribution
    if (qmode == hnswlib::QuantMode::INT8) {
        vector<uint8_t> sample_code(space->get_data_size());
        space->quantize(train_samples[0].data(), sample_code.data());
        int sum = 0, min_val = 255, max_val = 0;
        for (size_t i = 0; i < min(size_t(10), dim); ++i) {
            sum += sample_code[i];
            min_val = min(min_val, (int)sample_code[i]);
            max_val = max(max_val, (int)sample_code[i]);
        }
        cout << "  [INT8 codes first 10: min=" << min_val << " max=" << max_val 
             << " avg=" << (sum/10.0) << "]" << flush;
    }
    
    // Compute ground truth using quantized distances
    cout << "\n  Computing ground truth with quantized distances..." << flush;
    
    // Check distance distribution
    float min_dist = 1e9, max_dist = 0, sum_dist = 0;
    int sample_count = 0;
    for (size_t i = 0; i < min(size_t(100), n_train); ++i) {
        vector<uint8_t> ci(space->get_data_size());
        space->quantize(train_samples[i].data(), ci.data());
        for (size_t j = i+1; j < min(size_t(100), n_train); ++j) {
            vector<uint8_t> cj(space->get_data_size());
            space->quantize(train_samples[j].data(), cj.data());
            float d = space->get_dist_func()(ci.data(), cj.data(), space->get_dist_func_param());
            min_dist = min(min_dist, d);
            max_dist = max(max_dist, d);
            sum_dist += d;
            sample_count++;
        }
    }
    cout << " [dist range: " << min_dist << "-" << max_dist 
         << ", avg: " << (sum_dist/sample_count) 
         << ", ratio: " << (max_dist/min_dist) << "]" << flush;
    
    auto ground_truth = compute_ground_truth_quantized(data, queries, space, k);
    cout << " done!" << flush;
    
    // Sample ground truth distance to verify
    vector<uint8_t> q0(space->get_data_size());
    space->quantize(queries[0].data(), q0.data());
    vector<uint8_t> d0(space->get_data_size());
    space->quantize(data[ground_truth[0][0]].data(), d0.data());
    float gt_dist = space->get_dist_func()(q0.data(), d0.data(), space->get_dist_func_param());
    cout << " [GT dist example: " << gt_dist << "]" << flush;
    
    // Create HNSW index
    auto* index = new hnswlib::HierarchicalNSW<float>(
        space, n, M, ef_construction
    );
    
    // Build index and measure time
    auto build_start = chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < n; ++i) {
        // DON'T add to centroid during index construction - it invalidates the graph!
        // The initial training samples are sufficient.
        
        // Quantize and add to index
        vector<uint8_t> code(space->get_data_size());
        space->quantize(data[i].data(), code.data());
        index->addPoint(code.data(), i);
        
        if ((i + 1) % 10000 == 0) {
            cout << "." << flush;
        }
    }
    
    auto build_end = chrono::high_resolution_clock::now();
    result.build_time_ms = chrono::duration<float, milli>(
        build_end - build_start
    ).count();
    
    cout << " done!" << endl;
    
    // Verify index was built correctly
    cout << "  Index size: " << index->getCurrentElementCount() << " elements" << endl;
    cout << "  Max level: " << index->maxlevel_ << endl;
    
    // Query and measure time
    index->setEf(ef_search);

    index->saveIndex("test.bin");
    cout << "Saved.. " << endl;
    index->loadIndex("test.bin", space);
    
    cout << "  Querying..." << flush;
    auto query_start = chrono::high_resolution_clock::now();
    
    vector<vector<size_t>> results(queries.size());
    float total_dist = 0;
    int dist_count = 0;
    
    for (size_t q = 0; q < queries.size(); ++q) {
        // Quantize query
        vector<uint8_t> query_code(space->get_data_size());
        space->quantize(queries[q].data(), query_code.data());
        
        // Search
        auto pq = index->searchKnn(query_code.data(), k);
        
        results[q].resize(pq.size());
        for (int i = pq.size() - 1; i >= 0; --i) {
            results[q][i] = pq.top().second;  // FIXED: was [pq.size() - 1 - i]
            total_dist += pq.top().first;
            dist_count++;
            pq.pop();
        }
    }
    
    auto query_end = chrono::high_resolution_clock::now();
    result.query_time_ms = chrono::duration<float, milli>(
        query_end - query_start
    ).count();
    
    cout << " done! (avg dist: " << (total_dist / dist_count) << ")" << endl;
    
    // Calculate recall
    result.recall = calculate_recall(results, ground_truth);
    
    delete index;
    delete space;
    
    return result;
}

int main() {
    // Configuration
    const size_t dim = 384;
    const size_t n_data = 10000;
    const size_t n_queries = 1000;
    const size_t k = 10;
    
    cout << "=== Quantization Benchmark ===" << endl;
    cout << "Dimension: " << dim << endl;
    cout << "Data points: " << n_data << endl;
    cout << "Queries: " << n_queries << endl;
    cout << "k: " << k << endl << endl;
    
    // Generate data
    cout << "Generating data..." << endl;
    auto data = generate_data(n_data, dim, 42);
    
    // Generate queries from the SAME data (not a separate generation)
    // This ensures queries follow the same distribution
    vector<vector<float>> queries;
    vector<size_t> query_ids;  // Track which data point each query came from
    uniform_int_distribution<size_t> idx_dist(0, n_data - 1);
    mt19937 query_rng(123);
    for (size_t i = 0; i < n_queries; ++i) {
        size_t data_idx = idx_dist(query_rng);
        queries.push_back(data[data_idx]);
        query_ids.push_back(data_idx);
    }
    cout << "  Queries sampled from data" << endl;
    cout << endl;
    

#if 1
    // Test baseline with unquantized L2
    cout << "Testing BASELINE-L2 (unquantized)" << endl;
    {
        hnswlib::L2Space space(dim);
        hnswlib::HierarchicalNSW<float> index(&space, n_data, 16, 200);
        
        auto start = chrono::high_resolution_clock::now();
        for (size_t i = 0; i < n_data; ++i) {
            index.addPoint(data[i].data(), i);
            if ((i + 1) % 10000 == 0) cout << "." << flush;
        }
        auto end = chrono::high_resolution_clock::now();
        cout << " done!" << endl;
        cout << "  Build time: " << chrono::duration<float, milli>(end - start).count() << " ms" << endl;
        cout << "  Max level: " << index.maxlevel_ << endl;
        
        // Compute ground truth
        vector<vector<size_t>> gt(n_queries);
        for (size_t q = 0; q < n_queries; ++q) {
            vector<pair<float, size_t>> dists;
            for (size_t i = 0; i < n_data; ++i) {
                float d = 0;
                for (size_t dim_i = 0; dim_i < dim; ++dim_i) {
                    float diff = queries[q][dim_i] - data[i][dim_i];
                    d += diff * diff;
                }
                dists.push_back({d, i});
            }
            partial_sort(dists.begin(), dists.begin() + k, dists.end());
            gt[q].resize(k);
            for (size_t i = 0; i < k; ++i) gt[q][i] = dists[i].second;
        }
        
        // Query
        index.setEf(200);
        vector<vector<size_t>> results(n_queries);
        size_t found_self = 0;
        for (size_t q = 0; q < n_queries; ++q) {
            auto pq = index.searchKnn(queries[q].data(), k);
            results[q].resize(pq.size());
            bool found_this_query = false;
            for (int i = pq.size() - 1; i >= 0; --i) {
                size_t id = pq.top().second;
                results[q][i] = id;
                if (id == query_ids[q]) {
                    found_self++;
                    found_this_query = true;
                }
                pq.pop();
            }
            if (!found_this_query && q < 5) {
                cout << "  Query " << q << " (data ID " << query_ids[q] << ") didn't find itself! Results: ";
                for (auto r : results[q]) cout << r << " ";
                cout << endl;
            }
        }
        
        cout << "  Found self: " << found_self << " / " << n_queries << endl;
        
        float recall = calculate_recall(results, gt);
        cout << "  Recall: " << recall << endl << endl;
    }
#endif
    
    // Benchmark configurations
    vector<tuple<hnswlib::QuantMode, hnswlib::OptBinMode, string>> configs = {
#if 1
        {hnswlib::QuantMode::INT8, hnswlib::OptBinMode::STANDARD, "INT8-STANDARD"},
        {hnswlib::QuantMode::INT8, hnswlib::OptBinMode::CENTROID, "INT8-CENTROID"},
        {hnswlib::QuantMode::INT8, hnswlib::OptBinMode::ROTATIONAL, "INT8-ROTATIONAL"},
        {hnswlib::QuantMode::INT4, hnswlib::OptBinMode::STANDARD, "INT4-STANDARD"},
        {hnswlib::QuantMode::INT4, hnswlib::OptBinMode::ROTATIONAL, "INT4-ROTATIONAL"},
#endif
        {hnswlib::QuantMode::BIN1, hnswlib::OptBinMode::STANDARD, "BIN1-STANDARD"},
        {hnswlib::QuantMode::BIN1, hnswlib::OptBinMode::BETTER, "BIN1-BETTER"},
        {hnswlib::QuantMode::BIN1, hnswlib::OptBinMode::CENTROID, "BIN1-CENTROID"},
        {hnswlib::QuantMode::BIN1, hnswlib::OptBinMode::ROTATIONAL, "BIN1-ROTATIONAL"},
        {hnswlib::QuantMode::BIN1, hnswlib::OptBinMode::RABITQ, "BIN1-RABITQ"},
        {hnswlib::QuantMode::BIN1, hnswlib::OptBinMode::RABITQ_EXTENDED, "BIN1-RABITQ-EXT"},
    };
    
    vector<BenchmarkResult> results;
    
    for (const auto& [qmode, bin_mode, name] : configs) {
        cout << "Testing " << name << endl;
        auto result = run_benchmark(
            data, queries,
            qmode, bin_mode, name,
            k, 16, 200, 200  // M=16, ef_construction=200, ef_search=200
        );
        results.push_back(result);
        cout << endl;
    }
    
    // Print results table
    cout << "=== Results ===" << endl << endl;
    cout << left << setw(20) << "Mode"
         << right << setw(12) << "Build (ms)"
         << right << setw(12) << "Query (ms)"
         << right << setw(10) << "Recall"
         << right << setw(10) << "Bytes/Vec"
         << right << setw(12) << "Memory (KB)" << endl;
    cout << string(76, '-') << endl;
    
    for (const auto& r : results) {
        cout << left << setw(20) << r.mode_name
             << right << setw(12) << fixed << setprecision(1) << r.build_time_ms
             << right << setw(12) << fixed << setprecision(2) << r.query_time_ms
             << right << setw(10) << fixed << setprecision(4) << r.recall
             << right << setw(10) << r.bytes_per_vector
             << right << setw(12) << r.total_memory_kb << endl;
    }
    
    cout << endl;
    
    // Summary analysis
    cout << "=== Analysis ===" << endl;
    
    auto best_recall = max_element(results.begin(), results.end(),
        [](const auto& a, const auto& b) { return a.recall < b.recall; });
    
    auto fastest_query = min_element(results.begin(), results.end(),
        [](const auto& a, const auto& b) { return a.query_time_ms < b.query_time_ms; });
    
    auto smallest_memory = min_element(results.begin(), results.end(),
        [](const auto& a, const auto& b) { return a.total_memory_kb < b.total_memory_kb; });
    
    cout << "Best recall: " << best_recall->mode_name 
         << " (" << fixed << setprecision(4) << best_recall->recall << ")" << endl;
    cout << "Fastest query: " << fastest_query->mode_name 
         << " (" << fixed << setprecision(2) << fastest_query->query_time_ms << " ms)" << endl;
    cout << "Smallest memory: " << smallest_memory->mode_name 
         << " (" << smallest_memory->total_memory_kb << " KB)" << endl;
    
    return 0;
}
