#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>
#include <map>
#include <set>
#include <algorithm>
#include "hnswlib.h"

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

int main() {
    // Configuration
    const size_t dim = 128;
    const size_t n_data = 50000;
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
    
    // Test baseline with unquantized L2
    cout << "Testing BASELINE-L2 (unquantized)" << endl;
    {
        hnswlib::L2Space space(dim);

	hnswlib::HierarchicalNSW<float> index(&space, n_data, 48, 500);
	index.setEf(500);

        
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
                    d += diff * diff; // EDZ: checked !
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
#if 1

// fill in reverse to preserve nearest-first ordering
for (int i = int(pq.size()) - 1; i >= 0; --i) {
    results[q][i] = pq.top().second;
    if (results[q][i] == query_ids[q]) found_self++;
    pq.pop();
}

#else
            for (int i = pq.size() - 1; i >= 0; --i) {
                size_t id = pq.top().second;
                results[q][pq.size() - 1 - i] = id;
                // Check if query found itself (queries are sampled from data)
		if (id == query_ids[q]) found_self++;
                pq.pop();
            }
#endif
        }
        
        cout << "  Found self: " << found_self << " / " << n_queries << endl;
        
        float recall = calculate_recall(results, gt);
        cout << "  Recall: " << recall << endl << endl;

	// quick sanity test: recall should be exactly 1.0
	float recall_gt = calculate_recall(gt, gt);
	std::cout << "Sanity check: GT vs GT recall = " << recall_gt << std::endl;
	
    }
    
}
