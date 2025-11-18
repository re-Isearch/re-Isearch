#include "sve_bridge.h"

// ============================================================================
// Test Program
// ============================================================================

#include <iostream>
#include <chrono>
#include <vector>
#include <random>

void test_bridge_performance() {
    const size_t dim = 768;
    const size_t iterations = 100000;
    
    std::vector<float> v1(dim), v2(dim);
    std::random_device rd;
    std::mt19937 gen(42);
    std::normal_distribution<float> dist(0.0f, 1.0f);
    
    for (auto& x : v1) x = dist(gen);
    for (auto& x : v2) x = dist(gen);
    
    std::cout << "NEON-SVE Bridge Performance Test\n";
    std::cout << "=================================\n\n";
    
#ifdef __ARM_FEATURE_SVE
    std::cout << "SVE Vector Length: " << svcntw() << " floats (";
    std::cout << (svcntw() * 32) << " bits)\n\n";
#endif
    
#ifdef __ARM_NEON
    // Test NEON/Bridge version
    auto start = std::chrono::high_resolution_clock::now();
    volatile float result = 0.0f;
    for (size_t i = 0; i < iterations; i++) {
        result += hnswlib::L2SqrNEON_BridgeCompatible(v1.data(), v2.data(), &dim);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto bridge_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
        end - start).count() / iterations;
    
    std::cout << "NEON/Bridge: " << bridge_time << " ns\n";
#endif

#ifdef __ARM_FEATURE_SVE
    // Test native SVE version
    start = std::chrono::high_resolution_clock::now();
    result = 0.0f;
    for (size_t i = 0; i < iterations; i++) {
        result += hnswlib::L2SqrSVE_Native(v1.data(), v2.data(), &dim);
    }
    end = std::chrono::high_resolution_clock::now();
    auto native_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
        end - start).count() / iterations;
    
    std::cout << "Native SVE:  " << native_time << " ns\n";
    
    if (bridge_time > 0) {
        float overhead = ((float)bridge_time / native_time - 1.0f) * 100.0f;
        std::cout << "\nBridge overhead: " << overhead << "%\n";
    }
#endif
}

int main() {
    test_bridge_performance();
    return 0;
}

