#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <cmath>
#include "hnswlib.h"

using namespace std;

// Generate test data
vector<vector<float>> generate_test_data(size_t n, size_t dim, int seed = 42) {
    mt19937 rng(seed);
    normal_distribution<float> dist(0.0f, 1.0f);
    
    vector<vector<float>> data(n, vector<float>(dim));
    for (size_t i = 0; i < n; ++i) {
        for (size_t d = 0; d < dim; ++d) {
            data[i][d] = dist(rng);
        }
    }
    return data;
}

// Compute actual L2 distance
float l2_distance(const float* a, const float* b, size_t dim) {
    float sum = 0;
    for (size_t d = 0; d < dim; ++d) {
        float diff = a[d] - b[d];
        sum += diff * diff;
    }
    return sum;
}

void test_quantization_correlation(
    hnswlib::QuantMode qmode,
    hnswlib::OptBinMode bin_mode,
    const string& name) {
    
    cout << "\n=== Testing " << name << " ===" << endl;
    
    size_t dim = 128;
    size_t n_samples = 1000;
    size_t n_test = 100;
    
    // Generate data
    auto train_data = generate_test_data(n_samples, dim, 42);
    auto test_data = generate_test_data(n_test, dim, 123);
    
    // Create and train quantizer
    auto* space = new hnswlib::SpaceQuantized<float>(
        dim, qmode, bin_mode, &train_data, 500
    );
    
    cout << "Bytes per vector: " << space->get_data_size() << endl;
    
    // Quantize test vectors
    vector<vector<uint8_t>> quantized(n_test);
    for (size_t i = 0; i < n_test; ++i) {
        quantized[i].resize(space->get_data_size());
        space->quantize(test_data[i].data(), quantized[i].data());
    }
    
    // Compare distances
    vector<pair<float, float>> distance_pairs; // (L2, quantized)
    
    for (size_t i = 0; i < min(n_test, size_t(50)); ++i) {
        for (size_t j = i + 1; j < min(n_test, size_t(50)); ++j) {
            float l2_dist = l2_distance(
                test_data[i].data(), 
                test_data[j].data(), 
                dim
            );
            
            float quant_dist = space->get_dist_func()(
                quantized[i].data(),
                quantized[j].data(),
                space->get_dist_func_param()
            );
            
            distance_pairs.push_back({l2_dist, quant_dist});
        }
    }
    
    // Compute correlation
    float mean_l2 = 0, mean_q = 0;
    for (const auto& p : distance_pairs) {
        mean_l2 += p.first;
        mean_q += p.second;
    }
    mean_l2 /= distance_pairs.size();
    mean_q /= distance_pairs.size();
    
    float cov = 0, var_l2 = 0, var_q = 0;
    for (const auto& p : distance_pairs) {
        float diff_l2 = p.first - mean_l2;
        float diff_q = p.second - mean_q;
        cov += diff_l2 * diff_q;
        var_l2 += diff_l2 * diff_l2;
        var_q += diff_q * diff_q;
    }
    
    float correlation = cov / sqrt(var_l2 * var_q);
    
    cout << "Distance correlation: " << correlation << endl;
    cout << "L2 distance range: [" 
         << min_element(distance_pairs.begin(), distance_pairs.end())->first
         << ", "
         << max_element(distance_pairs.begin(), distance_pairs.end())->first
         << "]" << endl;
    cout << "Quantized distance range: ["
         << min_element(distance_pairs.begin(), distance_pairs.end(),
                       [](auto& a, auto& b) { return a.second < b.second; })->second
         << ", "
         << max_element(distance_pairs.begin(), distance_pairs.end(),
                       [](auto& a, auto& b) { return a.second < b.second; })->second
         << "]" << endl;
    
    // Check if centroid training worked
    if (bin_mode == hnswlib::OptBinMode::CENTROID) {
        cout << "Centroid training count: " << space->get_centroid_count() << endl;
    }
    
    // Show some example quantizations
    cout << "\nExample vector quantization:" << endl;
    cout << "Original (first 10 dims): ";
    for (size_t d = 0; d < min(dim, size_t(10)); ++d) {
        cout << test_data[0][d] << " ";
    }
    cout << endl;
    
    if (qmode == hnswlib::QuantMode::BIN1) {
        cout << "Binary code (first byte): " 
             << hex << int(quantized[0][0]) << dec << endl;
    } else if (qmode == hnswlib::QuantMode::INT8) {
        cout << "INT8 codes (first 10): ";
        for (size_t d = 0; d < min(dim, size_t(10)); ++d) {
            cout << int(quantized[0][d]) << " ";
        }
        cout << endl;
    }
    
    delete space;
}

void test_centroid_stability() {
    cout << "\n=== Testing Centroid Stability ===" << endl;
    
    size_t dim = 128;
    size_t n_samples = 1000;
    
    auto train_data = generate_test_data(n_samples, dim, 42);
    
    // Test with batch training
    auto* space_batch = new hnswlib::SpaceQuantized<float>(
        dim, hnswlib::QuantMode::BIN1, hnswlib::OptBinMode::CENTROID,
        &train_data, 500
    );
    
    auto centroid_batch = space_batch->get_centroid();
    
    // Test with incremental training
    auto* space_incr = new hnswlib::SpaceQuantized<float>(
        dim, hnswlib::QuantMode::BIN1, hnswlib::OptBinMode::CENTROID,
        nullptr, 500
    );
    
    // Add samples incrementally
    for (size_t i = 0; i < n_samples; ++i) {
        space_incr->add_to_centroid(train_data[i].data());
    }
    space_incr->flush_centroid_buffer();
    
    auto centroid_incr = space_incr->get_centroid();
    
    // Compare centroids
    float max_diff = 0;
    float avg_diff = 0;
    for (size_t d = 0; d < dim; ++d) {
        float diff = abs(centroid_batch[d] - centroid_incr[d]);
        max_diff = max(max_diff, diff);
        avg_diff += diff;
    }
    avg_diff /= dim;
    
    cout << "Batch vs Incremental centroid difference:" << endl;
    cout << "  Max: " << max_diff << endl;
    cout << "  Avg: " << avg_diff << endl;
    cout << "  Batch count: " << space_batch->get_centroid_count() << endl;
    cout << "  Incremental count: " << space_incr->get_centroid_count() << endl;
    
    delete space_batch;
    delete space_incr;
}

void test_distance_preservation() {
    cout << "\n=== Testing Distance Preservation ===" << endl;
    
    size_t dim = 128;
    
    // Create three vectors: A, B close to A, C far from A
    vector<float> vec_a(dim, 0.0f);
    vector<float> vec_b(dim, 0.0f);
    vector<float> vec_c(dim, 0.0f);
    
    for (size_t d = 0; d < dim; ++d) {
        vec_a[d] = 1.0f;
        vec_b[d] = 1.0f + (d % 2 == 0 ? 0.1f : -0.1f); // Close
        vec_c[d] = (d % 2 == 0 ? 1.0f : -1.0f); // Far
    }
    
    float l2_ab = l2_distance(vec_a.data(), vec_b.data(), dim);
    float l2_ac = l2_distance(vec_a.data(), vec_c.data(), dim);
    
    cout << "L2 distances: A-B=" << l2_ab << ", A-C=" << l2_ac << endl;
    
    vector<vector<float>> train_data = {vec_a, vec_b, vec_c};
    
    vector<hnswlib::QuantMode> modes = {
        hnswlib::QuantMode::BIN1,
        hnswlib::QuantMode::INT158,
        hnswlib::QuantMode::INT4,
        hnswlib::QuantMode::INT8
    };
    
    vector<string> mode_names = {"BIN1", "INT158", "INT4", "INT8"};
    
    for (size_t m = 0; m < modes.size(); ++m) {
        auto* space = new hnswlib::SpaceQuantized<float>(
            dim, modes[m], hnswlib::OptBinMode::STANDARD,
            &train_data, 500
        );
        
        vector<uint8_t> qa(space->get_data_size());
        vector<uint8_t> qb(space->get_data_size());
        vector<uint8_t> qc(space->get_data_size());
        
        space->quantize(vec_a.data(), qa.data());
        space->quantize(vec_b.data(), qb.data());
        space->quantize(vec_c.data(), qc.data());
        
        float q_ab = space->get_dist_func()(
            qa.data(), qb.data(), space->get_dist_func_param()
        );
        float q_ac = space->get_dist_func()(
            qa.data(), qc.data(), space->get_dist_func_param()
        );
        
        cout << mode_names[m] << ": A-B=" << q_ab 
             << ", A-C=" << q_ac 
             << " | Order preserved: " << ((q_ab < q_ac) ? "YES" : "NO")
             << endl;
        
        delete space;
    }
}

void test_nearest_neighbor_simple() {
    cout << "\n=== Testing Simple Nearest Neighbor ===" << endl;
    
    size_t dim = 128;
    
    // Create 10 vectors: one query and 9 candidates
    vector<vector<float>> data(10, vector<float>(dim));
    
    // Query vector (all 1.0)
    fill(data[0].begin(), data[0].end(), 1.0f);
    
    // Very similar vector (all 1.1)
    fill(data[1].begin(), data[1].end(), 1.1f);
    
    // Somewhat similar (all 0.5)
    fill(data[2].begin(), data[2].end(), 0.5f);
    
    // Different vectors
    for (size_t i = 3; i < 10; ++i) {
        for (size_t d = 0; d < dim; ++d) {
            data[i][d] = (d % 2 == 0) ? -1.0f : 1.0f;
        }
    }
    
    // Test each quantization mode
    vector<hnswlib::QuantMode> modes = {
        hnswlib::QuantMode::BIN1,
        hnswlib::QuantMode::INT158,
        hnswlib::QuantMode::INT4,
        hnswlib::QuantMode::INT8
    };

    vector<string> mode_names = {"BIN1", "INT158", "INT4",  "INT8"};
    
    for (size_t m = 0; m < modes.size(); ++m) {
        cout << "\n" << mode_names[m] << ":" << endl;
        
        auto* space = new hnswlib::SpaceQuantized<float>(
            dim, modes[m], hnswlib::OptBinMode::STANDARD, &data
        );
        
        // Quantize all vectors
        vector<vector<uint8_t>> quantized(10);
        for (size_t i = 0; i < 10; ++i) {
            quantized[i].resize(space->get_data_size());
            space->quantize(data[i].data(), quantized[i].data());
        }
        
        // Compute distances from query (vec 0) to all others
        vector<pair<float, size_t>> distances;
        for (size_t i = 1; i < 10; ++i) {
            float dist = space->get_dist_func()(
                quantized[0].data(),
                quantized[i].data(),
                space->get_dist_func_param()
            );
            distances.push_back({dist, i});
        }
        
        // Sort by distance
        sort(distances.begin(), distances.end());
        
        cout << "Nearest neighbors to query:" << endl;
        for (size_t i = 0; i < min(size_t(5), distances.size()); ++i) {
            cout << "  " << i+1 << ". Vector " << distances[i].second 
                 << " (dist=" << distances[i].first << ")" << endl;
        }
        
        // Check if vec 1 is nearest
        if (distances[0].second == 1) {
            cout << "  ✓ Most similar vector found!" << endl;
        } else {
            cout << "  ✗ Most similar vector NOT found (got " 
                 << distances[0].second << " instead of 1)" << endl;
        }
        
        delete space;
    }
}

int main() {
    cout << "=== Quantization Diagnostics ===" << endl;
    
    // Test distance correlation for each mode
    test_quantization_correlation(
        hnswlib::QuantMode::BIN1, 
        hnswlib::OptBinMode::STANDARD, 
        "BIN1-STANDARD"
    );
    
    test_quantization_correlation(
        hnswlib::QuantMode::BIN1, 
        hnswlib::OptBinMode::CENTROID, 
        "BIN1-CENTROID"
    );
    
    test_quantization_correlation(
        hnswlib::QuantMode::INT158, 
        hnswlib::OptBinMode::STANDARD, 
        "INT158-STANDARD"
    );
    
    test_quantization_correlation(
        hnswlib::QuantMode::INT8, 
        hnswlib::OptBinMode::STANDARD, 
        "INT8-STANDARD"
    );
    
    test_quantization_correlation(
        hnswlib::QuantMode::INT8, 
        hnswlib::OptBinMode::CENTROID, 
        "INT8-CENTROID"
    );
    
    // Test centroid training
    test_centroid_stability();
    
    // Test distance preservation
    test_distance_preservation();
    
    // NEW: Simple NN test
    test_nearest_neighbor_simple();
    
    return 0;
}
