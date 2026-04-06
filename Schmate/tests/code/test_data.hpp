#pragma once
#include <vector>
#include <cstddef>

// ------------------------------------
// TestData Struct
// ------------------------------------
struct TestData {
    std::vector<std::vector<float>> vectors;       // database vectors
    std::vector<std::vector<float>> queries;       // query vectors
    std::vector<std::vector<size_t>> ground_truth; // exact top-k neighbors
};

// ------------------------------------
// API
// ------------------------------------
TestData generate_test_data(
    size_t num_clusters = 100,
    size_t per_cluster = 1000,
    size_t dim = 384,
    size_t num_queries = 50,
    float noise_std = 0.05f,
    size_t gt_k = 10
);

// Optional utilities
std::vector<float> random_unit_vector(size_t dim);
float l2_distance(const std::vector<float>& a, const std::vector<float>& b);

