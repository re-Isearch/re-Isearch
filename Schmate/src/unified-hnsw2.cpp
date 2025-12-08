#include "unified-hnsw2.hpp"

/*

L1 (Manhattan): Uses 1/(1+dist) like L2.
L2: Euclidean distance squared

Both are unbounded distance metrics, so inverse mapping works well.


Binary (Hamming):

Range: [0, dimension] (e.g., 0-128 for 128D)
1/(1+dist) works well and matches the L2 approach
Alternative: (dim - dist) / dim gives linear similarity in [0,1]

Ternary (L2 squared):

Range: [0, 4×dimension] (e.g., 0-512 for 128D)
Values can differ by -1, 0, or +1, squared gives 0, 1, or 4
1/(1+dist) works well for consistency with L2

Why 1/(1+dist) for distance metrics?

Maps [0, ∞] → (0, 1]
Smooth decay: close distances get high scores
dist=0 → score=1.0 (perfect match)
dist=10 → score≈0.09
dist=100 → score≈0.01


Alternative:
case Metric::Binary:
    return (float)(dim - dist) / dim;  // [0,1] linear

case Metric::Ternary:
    return (float)(4*dim - dist) / (4*dim);  // [0,1] linear

*/

float UnifiedIndex::score_from_dist(float dist) const {
    // Safety first: guard against invalid or extreme distances
    if (!std::isfinite(dist)) return 0.0f;
    if (dist < -1e6f || dist > 1e6f) return 0.0f;

    switch (metric_) {
        case MetricSpace::L1:
            // L1 (Manhattan): smaller distance = closer
            // Same mapping as L2
            return 1.0f / (1.0f + dist);

        case MetricSpace::L2:
            // L2 distance: smaller = closer
            return 1.0f / (1.0f + dist);

        case MetricSpace::Binary:
#if 1
            return (float)(dim_ - dist) / dim_;  // [0,1] linear
#else
            // Binary (Hamming): distance is number of differing bits [0, dim]
            // For 128D: range is [0, 128]
            // Convert to similarity: (max_dist - dist) / max_dist
            // Or use inverse mapping like L2
            return 1.0f / (1.0f + dist);
#endif

        case MetricSpace::Ternary:
#if 1
             return (float)(4*dim_ - dist) / (4*dim_);  // [0,1] linear

#else
            // Ternary L2 squared: range [0, 4*dim]
            // For 128D: range is [0, 512]
            // Use inverse mapping like L2
             
            return 1.0f / (1.0f + dist);
#endif

        case MetricSpace::Cosine:
            // Cosine: distance = 1 - cosine_similarity, range [0,2]
            // similarity = 1 - distance
            // Map to [0,1]: (2 - dist) / 2
            return (2.0f - dist) / 2.0f;

        case MetricSpace::InnerProduct:
            // Inner product: higher (more negative) = closer
            // HNSWlib returns negative distances for similarity
            // Already in reasonable range, just clamp
            return std::clamp(dist, -1.0f, 1.0f);

        default:
            return 0.0f;
    }
}
