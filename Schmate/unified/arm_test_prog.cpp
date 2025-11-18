// Test program for HNSWlib with NEON support
// Compile: g++ -std=c++17 -O3 -march=armv8-a -I./hnswlib test_neon_support.cpp -o test_neon

#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>
#include <cmath>

// Include the modified headers
#include "hnswlib/hnswlib.h"

#include "sve_bridge.h"

// Test configuration
const size_t DIM = 768;
const size_t NUM_ELEMENTS = 10000;
const size_t NUM_QUERIES = 100;
const size_t K = 10;

// Generate random normalized vectors
std::vector<float> generate_random_vector(size_t dim, std::mt19937& gen) {
    std::normal_distribution<float> dist(0.0f, 1.0f);
    std::vector<float> vec(dim);
    
    float norm = 0.0f;
    for (size_t i = 0; i < dim; i++) {
        vec[i] = dist(gen);
        norm += vec[i] * vec[i];
    }
    
    // Normalize
    norm = std::sqrt(norm);
    for (size_t i = 0; i < dim; i++) {
        vec[i] /= norm;
    }
    
    return vec;
}

// Benchmark distance function
template<typename DistFunc>
double benchmark_distance(DistFunc dist_func, const std::vector<float>& v1, 
                         const std::vector<float>& v2, size_t iterations) {
    size_t dim = v1.size();
    
    auto start = std::chrono::high_resolution_clock::now();
    
    volatile float result = 0.0f; // volatile to prevent optimization
    for (size_t i = 0; i < iterations; i++) {
        result += dist_func(v1.data(), v2.data(), &dim);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    
    return duration / (double)iterations;
}

// Test correctness by comparing results
void test_correctness() {
    std::cout << "=== Correctness Tests ===\n\n";
    
    std::random_device rd;
    std::mt19937 gen(42); // Fixed seed for reproducibility
    
    auto v1 = generate_random_vector(DIM, gen);
    auto v2 = generate_random_vector(DIM, gen);
    
    size_t dim = DIM;
    
    // Test L2 distance
    std::cout << "L2 Distance:\n";
    float l2_scalar = hnswlib::L2Sqr(v1.data(), v2.data(), &dim);
    std::cout << "  Scalar result: " << l2_scalar << "\n";

#ifdef __ARM_FEATURE_SVE
    float l2_sve = hnswlib::L2SqrSVE(v1.data(), v2.data(), &dim);
    std::cout << "  SVE result:   " << l2_sve << "\n";
    float l2_diff_SVE = std::abs(l2_scalar - l2_sve);
    std::cout << "  Difference:    " << l2_diff_SVE << " (should be < 1e-5)\n";
    
    if (l2_diff_SVE < 1e-5f) {
        std::cout << "  ✓ PASSED\n";
    } else {
        std::cout << "  ✗ FAILED\n";
    }
#endif  

#ifdef __ARM_NEON
    float l2_neon = hnswlib::L2SqrNEON(v1.data(), v2.data(), &dim);
    std::cout << "  NEON result:   " << l2_neon << "\n";
    float l2_diff = std::abs(l2_scalar - l2_neon);
    std::cout << "  Difference:    " << l2_diff << " (should be < 1e-5)\n";
    
    if (l2_diff < 1e-5f) {
        std::cout << "  ✓ PASSED\n";
    } else {
        std::cout << "  ✗ FAILED\n";
    }
#else
    std::cout << "  NEON not available\n";
#endif
    
    std::cout << "\n";
    
    // Test Inner Product
    std::cout << "Inner Product:\n";
    float ip_scalar = hnswlib::InnerProduct(v1.data(), v2.data(), &dim);
    std::cout << "  Scalar result: " << ip_scalar << "\n";

#ifdef __ARM_FEATURE_SVE
    float ip_sve = hnswlib::InnerProductSVE(v1.data(), v2.data(), &dim);
    std::cout << "  SVE result:   " << ip_sve << "\n";
    float ip_diff_SVE = std::abs(ip_scalar - ip_sve);
    std::cout << "  Difference:    " << ip_diff_SVE << " (should be < 1e-5)\n";
    if (ip_diff_SVE < 1e-5f) {
        std::cout << "  ✓ PASSED\n";
    } else {
        std::cout << "  ✗ FAILED\n";
    }
#endif
#ifdef __ARM_NEON
    float ip_neon = hnswlib::InnerProductNEON(v1.data(), v2.data(), &dim);
    std::cout << "  NEON result:   " << ip_neon << "\n";
    float ip_diff = std::abs(ip_scalar - ip_neon);
    std::cout << "  Difference:    " << ip_diff << " (should be < 1e-5)\n";
    
    if (ip_diff < 1e-5f) {
        std::cout << "  ✓ PASSED\n";
    } else {
        std::cout << "  ✗ FAILED\n";
    }
#else
    std::cout << "  NEON not available\n";
#endif
    
    std::cout << "\n";
}

// Performance benchmark
void benchmark_performance() {
    std::cout << "=== Performance Benchmarks ===\n\n";
    
    std::random_device rd;
    std::mt19937 gen(42);
    
    auto v1 = generate_random_vector(DIM, gen);
    auto v2 = generate_random_vector(DIM, gen);
    
    const size_t ITERATIONS = 100000;
    
    // L2 Distance benchmark
    std::cout << "L2 Distance (" << DIM << " dimensions, " << ITERATIONS << " iterations):\n";
    
    double l2_scalar_time = benchmark_distance(hnswlib::L2Sqr, v1, v2, ITERATIONS);
    std::cout << "  Scalar: " << std::fixed << std::setprecision(2) << l2_scalar_time << " ns\n";


#ifdef __ARM_FEATURE_SVE
    double l2_sve_time = benchmark_distance(hnswlib::L2SqrSVE, v1, v2, ITERATIONS);
    std::cout << "  SVE   " << std::fixed << std::setprecision(2) << l2_sve_time << " ns\n";
    std::cout << "  Speedup: " << std::fixed << std::setprecision(2)
              << (l2_scalar_time / l2_sve_time) << "x\n";
#endif
#ifdef __ARM_NEON_SVE_BRIDGE
    double l2_bridge_time = benchmark_distance(hnswlib::L2SqrNEON_BridgeCompatible, v1, v2, ITERATIONS);
    std::cout << "  BRIDGE   " << std::fixed << std::setprecision(2) << l2_bridge_time << " ns\n";
    std::cout << "  Speedup: " << std::fixed << std::setprecision(2)
              << (l2_scalar_time / l2_bridge_time) << "x\n";
#endif
#ifdef __ARM_NEON
    double l2_neon_time = benchmark_distance(hnswlib::L2SqrNEON, v1, v2, ITERATIONS);
    std::cout << "  NEON:   " << std::fixed << std::setprecision(2) << l2_neon_time << " ns\n";
    std::cout << "  Speedup: " << std::fixed << std::setprecision(2) 
              << (l2_scalar_time / l2_neon_time) << "x\n";
#endif
#if defined(__ARM_FEATURE_SVE) && defined(__ARM_NEON)
    std::cout << "  Speedup (SVE:NEON): " << std::fixed << std::setprecision(2)
              << (l2_neon_time / l2_sve_time) << "x\n";
#endif
#if defined(__ARM_NEON) && defined (__ARM_NEON_SVE_BRIDGE)
    std::cout << "  Speedup (BRIDGE:NEON): " << std::fixed << std::setprecision(2)
              << (l2_neon_time / l2_bridge_time) << "x\n";
#endif
    std::cout << "\n";
    
    // Inner Product benchmark
    std::cout << "Inner Product (" << DIM << " dimensions, " << ITERATIONS << " iterations):\n";
    
    double ip_scalar_time = benchmark_distance(hnswlib::InnerProduct, v1, v2, ITERATIONS);
    std::cout << "  Scalar: " << std::fixed << std::setprecision(2) << ip_scalar_time << " ns\n";
    
#ifdef __ARM_FEATURE_SVE
    double ip_sve_time = benchmark_distance(hnswlib::InnerProductSVE, v1, v2, ITERATIONS);
    std::cout << "  SVE:   " << std::fixed << std::setprecision(2) << ip_sve_time << " ns\n";
    std::cout << "  Speedup: " << std::fixed << std::setprecision(2)
              << (ip_scalar_time / ip_sve_time) << "x\n";
#endif
#ifdef __ARM_NEON_SVE_BRIDGE
    double ip_bridge_time = benchmark_distance(hnswlib::InnerProductNEON_Universal, v1, v2, ITERATIONS);
    std::cout << "  BRIDGE:   " << std::fixed << std::setprecision(2) << ip_bridge_time << " ns\n";
    std::cout << "  Speedup: " << std::fixed << std::setprecision(2)
              << (ip_scalar_time / ip_bridge_time) << "x\n";
#endif
#ifdef __ARM_NEON
    double ip_neon_time = benchmark_distance(hnswlib::InnerProductNEON, v1, v2, ITERATIONS);
    std::cout << "  NEON:   " << std::fixed << std::setprecision(2) << ip_neon_time << " ns\n";
    std::cout << "  Speedup: " << std::fixed << std::setprecision(2) 
              << (ip_scalar_time / ip_neon_time) << "x\n";
#endif
#if defined(__ARM_FEATURE_SVE) && defined(__ARM_NEON)
    std::cout << "  Speedup (SVE:NEON): " << std::fixed << std::setprecision(2)
              << (ip_neon_time / ip_sve_time) << "x\n";
#endif
#if defined(__ARM_NEON) && defined (__ARM_NEON_SVE_BRIDGE)
    std::cout << "  Speedup (BRIDGE:NEON): " << std::fixed << std::setprecision(2)
              << (ip_neon_time / ip_bridge_time) << "x\n";
#endif
    
    std::cout << "\n";
}

// Full integration test with HNSWlib
void test_integration() {
    std::cout << "=== Integration Test with HNSWlib ===\n\n";
    
    std::random_device rd;
    std::mt19937 gen(42);
    
    // Test L2 space
    std::cout << "Testing L2Space:\n";
    hnswlib::L2Space space_l2(DIM);
    hnswlib::HierarchicalNSW<float> index_l2(&space_l2, NUM_ELEMENTS, 16, 200);
    
    std::cout << "  Adding " << NUM_ELEMENTS << " vectors...\n";
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<std::vector<float>> data;
    for (size_t i = 0; i < NUM_ELEMENTS; i++) {
        data.push_back(generate_random_vector(DIM, gen));
        index_l2.addPoint(data[i].data(), i);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto build_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "  Build time: " << build_time << " ms\n";
    std::cout << "  Throughput: " << (NUM_ELEMENTS * 1000 / build_time) << " vectors/sec\n";
    
    // Test search
    std::cout << "  Running " << NUM_QUERIES << " search queries...\n";
    index_l2.setEf(50);
    
    start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < NUM_QUERIES; i++) {
        auto query = generate_random_vector(DIM, gen);
        auto result = index_l2.searchKnn(query.data(), K);
    }
    end = std::chrono::high_resolution_clock::now();
    
    auto query_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << "  Average query time: " << (query_time / NUM_QUERIES) << " μs\n";
    std::cout << "  QPS: " << (NUM_QUERIES * 1000000 / query_time) << " queries/sec\n";
    
    std::cout << "\n";
    
    // Test Inner Product space
    std::cout << "Testing InnerProductSpace:\n";
    hnswlib::InnerProductSpace space_ip(DIM);
    hnswlib::HierarchicalNSW<float> index_ip(&space_ip, NUM_ELEMENTS, 16, 200);
    
    std::cout << "  Adding " << NUM_ELEMENTS << " vectors...\n";
    start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < NUM_ELEMENTS; i++) {
        index_ip.addPoint(data[i].data(), i);
    }
    
    end = std::chrono::high_resolution_clock::now();
    build_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "  Build time: " << build_time << " ms\n";
    std::cout << "  Throughput: " << (NUM_ELEMENTS * 1000 / build_time) << " vectors/sec\n";
    
    // Test search
    std::cout << "  Running " << NUM_QUERIES << " search queries...\n";
    index_ip.setEf(50);
    
    start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < NUM_QUERIES; i++) {
        auto query = generate_random_vector(DIM, gen);
        auto result = index_ip.searchKnn(query.data(), K);
    }
    end = std::chrono::high_resolution_clock::now();
    
    query_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << "  Average query time: " << (query_time / NUM_QUERIES) << " μs\n";
    std::cout << "  QPS: " << (NUM_QUERIES * 1000000 / query_time) << " queries/sec\n";
    
    std::cout << "\n";
}

int main() {
    std::cout << "HNSWlib NEON Support Test\n";
    std::cout << "==========================\n\n";
    
    // Detect SIMD support
    std::cout << "SIMD Support:\n";
#ifdef __ARM_NEON
    std::cout << "  ✓ ARM NEON enabled\n";
#else
    std::cout << "  ✗ ARM NEON not available\n";
#endif
#ifdef USE_AVX512
    std::cout << "  ✓ AVX-512 enabled\n";
#endif
#ifdef USE_AVX
    std::cout << "  ✓ AVX2 enabled\n";
#endif
    std::cout << "\n";
    
    // Run tests
    test_correctness();
    benchmark_performance();
    test_integration();
    
    std::cout << "=== All Tests Complete ===\n";
    
    return 0;
}
