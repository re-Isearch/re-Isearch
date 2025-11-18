#include <iostream>
#include <vector>
#include <random>
#include "hnswlib.h"
#include "space_quantized.h"

using namespace std;

int main() {
    size_t dim = 128;
    size_t n = 1000;
    
    // Generate data
    mt19937 rng(42);
    normal_distribution<float> dist(0.0f, 1.0f);
    
    vector<vector<float>> data(n, vector<float>(dim));
    for (size_t i = 0; i < n; ++i) {
        for (size_t d = 0; d < dim; ++d) {
            data[i][d] = dist(rng);
        }
    }
    
    // Create quantized space
    vector<vector<float>> train_samples(data.begin(), data.begin() + 100);
    auto* space = new hnswlib::SpaceQuantized<float>(
        dim, hnswlib::QuantMode::INT8, hnswlib::OptBinMode::STANDARD,
        &train_samples
    );
    
    cout << "Bytes per vector: " << space->get_data_size() << endl;
    
    // Quantize data
    vector<vector<uint8_t>> quantized(n);
    for (size_t i = 0; i < n; ++i) {
        quantized[i].resize(space->get_data_size());
        space->quantize(data[i].data(), quantized[i].data());
    }
    
    // Test distance function directly
    cout << "\nTesting distance function:" << endl;
    cout << "Distance type: INT8" << endl;
    cout << "Dimension: " << dim << endl;
    
    float min_dist = 1e9, max_dist = 0;
    for (int i = 0; i < 100; ++i) {
        for (int j = i + 1; j < 100; ++j) {
            float dist = space->get_dist_func()(
                quantized[i].data(),
                quantized[j].data(),
                space->get_dist_func_param()
            );
            min_dist = min(min_dist, dist);
            max_dist = max(max_dist, dist);
        }
    }
    
    cout << "Distance range over 100 vectors: [" << min_dist << ", " << max_dist << "]" << endl;
    cout << "Expected range: ~[10000, 100000] for normalized 128D vectors" << endl;
    
    // Show a few sample distances
    for (int i = 0; i < 5; ++i) {
        for (int j = i + 1; j < min(i + 3, 5); ++j) {
            float dist = space->get_dist_func()(
                quantized[i].data(),
                quantized[j].data(),
                space->get_dist_func_param()
            );
            cout << "  dist(" << i << "," << j << ") = " << dist << endl;
        }
    }
    
    // Create HNSW index
    cout << "\nBuilding HNSW index..." << endl;
    auto* index = new hnswlib::HierarchicalNSW<float>(space, n, 16, 200);
    
    for (size_t i = 0; i < n; ++i) {
        index->addPoint(quantized[i].data(), i);
    }
    
    cout << "Index built. Elements: " << index->getCurrentElementCount() << endl;
    cout << "Max level: " << index->maxlevel_ << endl;
    
    // Test queries
    cout << "\nTesting queries:" << endl;
    index->setEf(50);
    
    for (int q = 0; q < 3; ++q) {
        cout << "Query " << q << ": ";
        auto results = index->searchKnn(quantized[q].data(), 5);
        
        cout << "Found " << results.size() << " results: ";
        while (!results.empty()) {
            cout << results.top().second << "(" << results.top().first << ") ";
            results.pop();
        }
        cout << endl;
    }
    
    // Compute exact k-NN for query 0
    cout << "\nExact k-NN for query 0:" << endl;
    vector<pair<float, size_t>> exact_dists;
    for (size_t i = 0; i < n; ++i) {
        float d = space->get_dist_func()(
            quantized[0].data(),
            quantized[i].data(),
            space->get_dist_func_param()
        );
        exact_dists.push_back({d, i});
    }
    sort(exact_dists.begin(), exact_dists.end());
    
    cout << "Exact top 5: ";
    for (int i = 0; i < 5; ++i) {
        cout << exact_dists[i].second << "(" << exact_dists[i].first << ") ";
    }
    cout << endl;
    
    delete index;
    delete space;
    
    return 0;
}
