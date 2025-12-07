#include <iostream>
#include <random>
#include <chrono>
#include <vector>
#include <cmath>
#include <cassert>

#include "hnswlib/cosine_similarity.h"

static constexpr float EPS = 1e-4f;

// Generate a random vector
void fill_random(std::vector<float>& v) {
    static std::mt19937 rng(123);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    for (float &x : v) x = dist(rng);
}

// Simple compare check with tolerance
bool almost_equal(float a, float b, float eps = EPS) {
    return std::fabs(a - b) <= eps;
}

void run_correctness_tests() {
    std::cout << "Running correctness tests...\n";

    const std::vector<size_t> dims = {1, 3, 4, 7, 8, 16, 31, 64, 1000, 4096};
    for (size_t dim : dims) {
        std::vector<float> a(dim), b(dim);
        fill_random(a);
        fill_random(b);

        float ref =  hnswlib::cosine_scalar(a.data(), b.data(), dim);
        float test = hnswlib::cosine_similarity(a.data(), b.data(), dim);

        if (!almost_equal(ref, test)) {
            std::cerr << "Mismatch! dim=" << dim
                      << " ref=" << ref << " simd=" << test << "\n";
            assert(false);
        }
    }

    // test zeros
    {
        std::vector<float> a(32, 0.f), b(32, 0.f);
        float ref = hnswlib::cosine_scalar(a.data(), b.data(), 32);
        float test = hnswlib::cosine_similarity(a.data(), b.data(), 32);
        assert(almost_equal(ref, test));
    }

    std::cout << "✅ Correctness tests passed!\n";
}


void benchmark(size_t dim, size_t iters = 2'000'000) {
    std::vector<float> a(dim), b(dim);
    fill_random(a);
    fill_random(b);

    auto now = [] { return std::chrono::steady_clock::now(); };

    float sink = 0.0f; // prevents optimizing out calls

    // Warm-ups
    for (int i = 0; i < 1000; ++i)
        sink += hnswlib::cosine_similarity(a.data(), b.data(), dim);

    // Scalar timing
    auto t1 = now();
    for (size_t i = 0; i < iters; ++i)
        sink += hnswlib::cosine_scalar(a.data(), b.data(), dim);
    auto t2 = now();

    // SIMD timing
    for (size_t i = 0; i < iters; ++i)
        sink += hnswlib::cosine_similarity(a.data(), b.data(), dim);
    auto t3 = now();

    auto dt_scalar = std::chrono::duration<double, std::nano>(t2 - t1).count();
    auto dt_simd   = std::chrono::duration<double, std::nano>(t3 - t2).count();

    double ns_scalar = dt_scalar / iters;
    double ns_simd   = dt_simd / iters;

    std::cout << "dim=" << dim
              << " scalar=" << ns_scalar << "ns"
              << " simd="   << ns_simd << "ns"
              << " speedup=" << (ns_scalar / ns_simd)
              << "x\n";

    // Prevent optimizing out computations
    if (sink == 123.456f) std::cerr << "ignore\n";
}



int main() {
    run_correctness_tests();

    std::cout << "\nBenchmark (higher is better):\n";
    benchmark(64);
    benchmark(256);
    benchmark(384);
    benchmark(768);
    benchmark(1024);
    benchmark(4096);
}

