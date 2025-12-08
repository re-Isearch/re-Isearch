#pragma once
#include <algorithm>
#include <cmath>

// Distance to score conversion functions
namespace scoring {
    
    // For L2 (Euclidean) distance - normalized vectors
    // Assumes vectors are normalized (magnitude = 1)
    float l2_to_similarity_normalized(float squared_dist) {
        // For normalized vectors: squared_dist is in range [0, 4]
        // similarity = 1 - (dist / max_dist)
        float dist = std::sqrt(squared_dist);
        return std::max(0.0f, 1.0f - (dist / 2.0f));
    }
    
    // For L2 distance - general case with known max distance
    float l2_to_similarity(float squared_dist, float max_squared_dist) {
        return std::max(0.0f, 1.0f - std::sqrt(squared_dist / max_squared_dist));
    }
    
    // Exponential decay for L2 distance
    // Commonly used in similarity search, sigma controls decay rate
    float l2_exponential_similarity(float squared_dist, float sigma = 1.0f) {
        return std::exp(-squared_dist / (2.0f * sigma * sigma));
    }
    
    // RBF (Radial Basis Function) kernel
    float l2_rbf_similarity(float squared_dist, float gamma = 1.0f) {
        return std::exp(-gamma * squared_dist);
    }
    
    // For Inner Product distance (cosine similarity for normalized vectors)
    // Inner product distance = 1 - cosine_similarity
    // So: cosine_similarity = 1 - distance
    float inner_product_to_cosine(float inner_product_dist) {
        // For normalized vectors, distance is in range [0, 2]
        // where 0 = identical, 2 = opposite
        float cosine_sim = 1.0f - inner_product_dist;
        return cosine_sim; // Range: [-1, 1]
    }
    
    // Convert cosine similarity to [0, 1] range
    float cosine_to_normalized(float cosine_sim) {
        return (cosine_sim + 1.0f) / 2.0f; // Maps [-1,1] to [0,1]
    }
    
    // Inverse distance scoring (for unnormalized L2)
    float inverse_distance(float squared_dist, float offset = 1.0f) {
        return 1.0f / (1.0f + std::sqrt(squared_dist) + offset);
    }
}

/*

#include <vector>
#include <iostream>
#include "hnswlib/hnswlib.h"

int main() {
    int dim = 128;
    int max_elements = 10000;
    
    // Example 1: L2 Space with normalized vectors
    std::cout << "=== Example 1: L2 Space (Normalized Vectors) ===\n";
    hnswlib::L2Space space_l2(dim);
    hnswlib::HierarchicalNSW<float>* alg_l2 = 
        new hnswlib::HierarchicalNSW<float>(&space_l2, max_elements, 16, 200);
    
    // Add normalized data
    std::vector<float> data(dim);
    for (int i = 0; i < 1000; i++) {
        float norm = 0.0f;
        for (int d = 0; d < dim; d++) {
            data[d] = static_cast<float>(rand()) / RAND_MAX - 0.5f;
            norm += data[d] * data[d];
        }
        norm = std::sqrt(norm);
        for (int d = 0; d < dim; d++) {
            data[d] /= norm; // Normalize
        }
        alg_l2->addPoint(data.data(), i);
    }
    
    // Normalized query
    std::vector<float> query(dim);
    float query_norm = 0.0f;
    for (int d = 0; d < dim; d++) {
        query[d] = static_cast<float>(rand()) / RAND_MAX - 0.5f;
        query_norm += query[d] * query[d];
    }
    query_norm = std::sqrt(query_norm);
    for (int d = 0; d < dim; d++) {
        query[d] /= query_norm;
    }
    
    // Epsilon search with L2
    float epsilon = 1.5;
    float epsilon2 = epsilon * epsilon;
    hnswlib::EpsilonSearchStopCondition<float> stop_l2(epsilon2, 10, max_elements);
    auto results_l2 = alg_l2->searchStopConditionClosest(query.data(), stop_l2);
    
    std::cout << "Found " << results_l2.size() << " results\n";
    for (size_t i = 0; i < std::min(size_t(5), results_l2.size()); i++) {
        auto [sq_dist, label] = results_l2[i];
        float score1 = scoring::l2_to_similarity_normalized(sq_dist);
        float score2 = scoring::l2_exponential_similarity(sq_dist, 0.5f);
        float score3 = scoring::l2_rbf_similarity(sq_dist, 0.5f);
        
        std::cout << "Label: " << label 
                  << ", Dist: " << std::sqrt(sq_dist)
                  << ", Linear Score: " << score1
                  << ", Exp Score: " << score2
                  << ", RBF Score: " << score3 << "\n";
    }
    
    // Example 2: Inner Product (Cosine Similarity)
    std::cout << "\n=== Example 2: Inner Product Space ===\n";
    hnswlib::InnerProductSpace space_ip(dim);
    hnswlib::HierarchicalNSW<float>* alg_ip = 
        new hnswlib::HierarchicalNSW<float>(&space_ip, max_elements, 16, 200);
    
    // Add normalized data for cosine similarity
    for (int i = 0; i < 1000; i++) {
        float norm = 0.0f;
        for (int d = 0; d < dim; d++) {
            data[d] = static_cast<float>(rand()) / RAND_MAX - 0.5f;
            norm += data[d] * data[d];
        }
        norm = std::sqrt(norm);
        for (int d = 0; d < dim; d++) {
            data[d] /= norm;
        }
        alg_ip->addPoint(data.data(), i);
    }
    
    // For InnerProduct, smaller (more negative) distance = more similar
    // epsilon should be the maximum distance you want (e.g., 0.5 for moderately similar)
    float ip_epsilon = 0.5f;
    hnswlib::EpsilonSearchStopCondition<float> stop_ip(ip_epsilon, 10, max_elements);
    auto results_ip = alg_ip->searchStopConditionClosest(query.data(), stop_ip);
    
    std::cout << "Found " << results_ip.size() << " results\n";
    for (size_t i = 0; i < std::min(size_t(5), results_ip.size()); i++) {
        auto [dist, label] = results_ip[i];
        float cosine_sim = scoring::inner_product_to_cosine(dist);
        float normalized_score = scoring::cosine_to_normalized(cosine_sim);
        
        std::cout << "Label: " << label 
                  << ", IP Dist: " << dist
                  << ", Cosine Sim: " << cosine_sim
                  << ", Normalized Score [0,1]: " << normalized_score << "\n";
    }
    
    delete alg_l2;
    delete alg_ip;
    return 0;
}

/*
 * SCORING METHODS SUMMARY:
 * 
 * 1. L2 Distance (Euclidean):
 *    - Linear: 1 - (dist / max_dist)
 *      Simple, but assumes you know max distance
 *    
 *    - Exponential: exp(-dist^2 / (2*sigma^2))
 *      Most common for similarity, sigma controls sensitivity
 *      sigma = 0.5 → steep decay, sigma = 2.0 → gradual decay
 *    
 *    - RBF Kernel: exp(-gamma * dist^2)
 *      Similar to exponential, gamma = 1/(2*sigma^2)
 *    
 *    - Inverse: 1 / (1 + dist)
 *      Simple, always in [0,1], no parameters needed
 * 
 * 2. Inner Product (for normalized vectors = Cosine Similarity):
 *    - Cosine similarity = 1 - distance, range [-1, 1]
 *    - To normalize to [0, 1]: (cosine + 1) / 2
 *    - Interpretation:
 *      1.0 = identical direction
 *      0.5 = orthogonal
 *      0.0 = opposite direction
 * 
 * 3. Choosing the right method:
 *    - Use exponential/RBF for most ML applications (smooth decay)
 *    - Use linear if you have a known maximum distance
 *    - Use cosine similarity for text/embeddings (direction matters)
 *    - Tune sigma/gamma based on your distance distribution
 * 
 * 4. Parameter tuning:
 *    - Analyze your distance distribution (mean, std dev)
 *    - Set sigma ≈ std deviation of distances
 *    - Or choose sigma so that average distance → score of ~0.6
 */
