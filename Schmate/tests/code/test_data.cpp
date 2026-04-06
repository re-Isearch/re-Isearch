
#include "test_data.hpp"

#include <random>
#include <cmath>
#include <algorithm>
#include <stdexcept>

// ================================================================
// Internal Helpers
// ================================================================

// Generate a normalized random vector (cluster center)
static std::vector<float> rand_unit_vec(size_t dim, std::mt19937 &rng) {
    std::normal_distribution<float> dist(0.0f, 1.0f);
    std::vector<float> v(dim);
    float norm = 0.0f;

    for (size_t i = 0; i < dim; i++) {
        v[i] = dist(rng);
        norm += v[i] * v[i];
    }

    norm = std::sqrt(norm);
    for (auto &x : v) x /= norm;

    return v;
}

// Add Gaussian perturbation and normalize
static std::vector<float> perturb(
    const std::vector<float> &base,
    float stddev,
    std::mt19937 &rng)
{
    std::normal_distribution<float> dist(0.0f, stddev);
    std::vector<float> v(base.size());
    float norm = 0.0f;

    for (size_t i = 0; i < base.size(); i++) {
        v[i] = base[i] + dist(rng);
        norm += v[i] * v[i];
    }

    norm = std::sqrt(norm);
    for (auto &x : v) x /= norm;

    return v;
}

// L2 distance
float l2_distance(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.size() != b.size())
        throw std::runtime_error("l2_distance: dimension mismatch");

    float s = 0.0f;
    for (size_t i = 0; i < a.size(); i++) {
        float d = a[i] - b[i];
        s += d * d;
    }
    return s;
}

// Public utility: normalized random vector
std::vector<float> random_unit_vector(size_t dim) {
    std::mt19937 rng(std::random_device{}());
    return rand_unit_vec(dim, rng);
}

// ================================================================
// Main generator
// ================================================================

TestData generate_test_data(
    size_t num_clusters,
    size_t per_cluster,
    size_t dim,
    size_t num_queries,
    float noise_std,
    size_t gt_k)
{
    std::mt19937 rng(12345);

    TestData data;
    data.vectors.reserve(num_clusters * per_cluster);

    // 1) Generate cluster centers
    std::vector<std::vector<float>> centers;
    centers.reserve(num_clusters);
    for (size_t c = 0; c < num_clusters; c++) {
        centers.push_back(rand_unit_vec(dim, rng));
    }

    // 2) Generate dataset vectors
    for (size_t c = 0; c < num_clusters; c++) {
        for (size_t i = 0; i < per_cluster; i++) {
            auto v = perturb(centers[c], noise_std, rng);
            data.vectors.push_back(v);
        }
    }

    // 3) Generate queries
    for (size_t q = 0; q < num_queries; q++) {
        size_t cluster_id = q % num_clusters;
        auto qv = perturb(centers[cluster_id], noise_std, rng);
        data.queries.push_back(qv);
    }

    // 4) Compute exact ground truth
    for (auto &qv : data.queries) {
        std::vector<std::pair<float, size_t>> dists;
        dists.reserve(data.vectors.size());

        for (size_t i = 0; i < data.vectors.size(); i++) {
            float d = l2_distance(qv, data.vectors[i]);
            dists.emplace_back(d, i);
        }

        std::sort(dists.begin(), dists.end(),
                  [](auto &a, auto &b) { return a.first < b.first; });

        std::vector<size_t> gt;
        for (size_t i = 0; i < gt_k; i++) {
            gt.push_back(dists[i].second);
        }

        data.ground_truth.push_back(gt);
    }

    return data;
}

