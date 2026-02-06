# Vector Quantization Documentation

## Overview

This library provides multiple quantization schemes to compress high-dimensional vectors for efficient similarity search. Quantization reduces memory usage and speeds up distance computations while maintaining search accuracy.

---

## Quantization Modes (QuantMode)

These determine how many bits are used per dimension to represent vectors.

### BIN1 (1-bit Binary)
- **Encoding**: Each dimension becomes 1 bit (0 or 1)
- **Storage**: dim/8 bytes per vector
- **Distance**: Hamming distance (count of differing bits)
- **Use case**: Maximum compression when memory is critical
- **Trade-off**: Lowest accuracy, fastest computation

**Example (384 dims)**: 384 bits = 48 bytes (8× compression vs float32)

---

### INT158 (1.58-bit Ternary)
- **Encoding**: Each dimension becomes one of 3 values: {-1, 0, +1}
- **Storage**: 2 bits per dimension (dim/4 bytes)
- **Distance**: Squared differences of ternary values
- **Use case**: Better than binary while still very compact
- **Trade-off**: Moderate accuracy, good compression

**Example (384 dims)**: 768 bits = 96 bytes (4× compression)

---

### INT4 (4-bit Integer)
- **Encoding**: Each dimension becomes 0-15 integer
- **Storage**: dim/2 bytes per vector (2 values packed per byte)
- **Distance**: Sum of squared integer differences
- **Use case**: Good balance of accuracy and compression
- **Trade-off**: High accuracy (~99%), moderate compression

**Example (384 dims)**: 1536 bits = 192 bytes (2× compression)

---

### INT8 (8-bit Integer)
- **Encoding**: Each dimension becomes 0-255 integer
- **Storage**: dim bytes per vector
- **Distance**: Sum of squared integer differences
- **Use case**: Near-perfect accuracy with 4× memory savings
- **Trade-off**: Best accuracy (~99%), least compression of quantized modes

**Example (384 dims)**: 384 bytes (4× compression vs float32)

---

## Optimization Modes (OptBinMode)

These modify how quantization thresholds are computed and can add additional information.

### STANDARD
- **Method**: Uses median (50th percentile) for BIN1, min/max range for INT4/INT8
- **Pros**: Simple, fast, deterministic
- **Cons**: Doesn't adapt to data distribution
- **Recommended for**: General use, baseline comparisons

---

### BETTER
- **Method**: For BIN1 only - searches 99 percentiles to find threshold with maximum Pearson correlation between binary codes and original values
- **Pros**: Theoretically optimal for preserving information
- **Cons**: Very slow to compute, often performs worse on normalized/clustered data
- **Recommended for**: ⚠️ **Not recommended** - use STANDARD or CENTROID instead

---

### CENTROID
- **Method**: Computes mean (centroid) from training data and uses it as threshold/center point
- **Pros**: Adapts to data distribution, can be updated incrementally
- **Cons**: Requires training data
- **Recommended for**: When you have representative training samples
- **Best results**: INT8-CENTROID typically best INT8 variant

**Incremental Training**: Supports online learning by accumulating statistics as vectors are added:
```cpp
space->add_to_centroid(vector);  // Add each new vector
space->flush_centroid_buffer();  // Retrain after batch
```

---

### ROTATIONAL
- **Method**: Applies learned orthogonal rotation matrix before quantization
- **How it works**: 
  1. Learns rotation via random orthogonal matrix (or PCA)
  2. Rotates all vectors before quantizing
  3. Decorrelates dimensions for better quantization
- **Pros**: Can improve quantization quality
- **Cons**: Expensive rotation computation (dim² operations), requires training
- **Recommended for**: ⚠️ **Not recommended** - overhead usually outweighs benefits

---

### RABITQ (Residual-Aware Binary Quantization)
- **Method**: For BIN1 only - stores binary code plus residuals for top-K error dimensions
- **How it works**:
  1. Quantize to binary (1 bit/dim)
  2. Compute reconstruction error per dimension
  3. Store full float values for 16 highest-error dimensions
  4. Distance = weighted combination of Hamming + residual L2 distance
- **Storage**: (dim/8) + 64 bytes
- **Pros**: Near-perfect accuracy with minimal overhead
- **Cons**: Slightly slower than pure binary, needs more memory than BIN1-STANDARD
- **Recommended for**: ⭐ **Best overall choice** - combines speed, accuracy, and memory efficiency

**Example (384 dims)**: 48 (binary) + 64 (residuals) = 112 bytes

---

### RABITQ_EXTENDED
- **Method**: Like RABITQ but stores 64 residual dimensions instead of 16
- **Storage**: (dim/8) + 256 bytes
- **Pros**: Maximum accuracy for binary quantization
- **Cons**: More memory than RABITQ
- **Recommended for**: When recall must be absolutely maximized and memory allows

**Example (384 dims)**: 48 (binary) + 256 (residuals) = 304 bytes

---

## Performance Comparison

Based on 384-dimensional normalized vectors with 10 clusters, 50K vectors:

| Mode | Build (ms) | Query (ms) | Recall | Memory (KB) | Recommendation |
|------|-----------|-----------|--------|-------------|----------------|
| **BIN1-RABITQ** | 4,298 | **75** | **99.99%** | 5,468 | ⭐ **Best Overall** |
| BIN1-RABITQ-EXT | ~5,000 | ~90 | ~100% | 8,203 | Use if recall must be perfect |
| BIN1-STANDARD | 4,859 | 92 | 85% | 2,343 | Ultra-low memory |
| INT8-CENTROID | 18,394 | 363 | 98.6% | 18,750 | Highest quality |
| INT8-STANDARD | 20,132 | 386 | 98.8% | 18,750 | - |
| INT4-STANDARD | 77,476 | 1,326 | 99% | 9,375 | ⚠️ Too slow |
| BIN1-BETTER | 5,851 | 16 | 38% | 2,343 | ❌ Broken |
| BIN1-ROTATIONAL | 8,806 | 132 | 84.5% | 2,343 | ❌ No benefit |

---

## Quick Start Guide

### For Maximum Speed + Excellent Accuracy
```cpp
QuantMode qmode = QuantMode::BIN1;
OptBinMode bin_mode = OptBinMode::RABITQ;
// Result: 99.99% recall, 75ms queries, 112 bytes/vector
```

### For Maximum Accuracy
```cpp
QuantMode qmode = QuantMode::INT8;
OptBinMode bin_mode = OptBinMode::CENTROID;
// Result: 98.6% recall, 363ms queries, 384 bytes/vector
```

### For Minimum Memory
```cpp
QuantMode qmode = QuantMode::BIN1;
OptBinMode bin_mode = OptBinMode::STANDARD;
// Result: 85% recall, 92ms queries, 48 bytes/vector
```

---

## Usage Example

```cpp
#include "space_quantized.h"

// 1. Prepare training data (10% of dataset)
std::vector<std::vector<float>> train_samples;
// ... fill with training vectors ...

// 2. Create quantized space
size_t dim = 384;
auto* space = new hnswlib::SpaceQuantized<float>(
    dim,
    hnswlib::QuantMode::BIN1,           // Use binary quantization
    hnswlib::OptBinMode::RABITQ,        // With residuals
    &train_samples,                      // Training data
    1000                                 // Buffer size for incremental learning
);

// 3. Create HNSW index
auto* index = new hnswlib::HierarchicalNSW<float>(
    space, 
    max_elements,
    M,                  // Number of connections
    ef_construction     // Build quality
);

// 4. Add vectors
std::vector<uint8_t> code(space->get_data_size());
for (size_t i = 0; i < n_vectors; ++i) {
    space->quantize(vectors[i].data(), code.data());
    index->addPoint(code.data(), i);
}

// 5. Query
index->setEf(ef_search);
std::vector<uint8_t> query_code(space->get_data_size());
space->quantize(query.data(), query_code.data());
auto results = index->searchKnn(query_code.data(), k);

// 6. Save/load
space->save_centroid("quantizer.bin");
index->saveIndex("index.bin");
```

---

## Implementation Details

### SIMD Optimizations
All quantization modes use SIMD instructions when available:
- **AVX2** (x86): 8-wide float operations
- **NEON** (ARM): 4-wide float operations
- **Automatic fallback**: Scalar implementation when SIMD unavailable

Enable with compilation flags:
```bash
clang -O3 -march=native -o program program.cpp
```

### Distance Computations
- **BIN1**: Hamming distance via `popcount` instruction
- **INT4/INT8**: Squared L2 distance with SIMD
- **RABITQ**: Hybrid = 0.7×Hamming + 0.3×Residual_L2
- **RABITQ_EXTENDED**: Hybrid = 0.5×Hamming + 0.5×Residual_L2

### Persistence
Quantizer state (thresholds, centroids, rotation matrices, residual info) can be saved:
```cpp
space->save_centroid("quantizer.bin");
space->load_centroid("quantizer.bin");
```

---

## Choosing the Right Configuration

### Decision Tree

1. **Need highest accuracy (>98%)?**
   - Yes → Use **INT8-CENTROID**
   - No → Continue

2. **Memory extremely limited?**
   - Yes → Use **BIN1-STANDARD** (85% recall)
   - No → Continue

3. **Want best speed/accuracy/memory balance?**
   - Yes → Use **BIN1-RABITQ** ⭐ (99.99% recall)

4. **Building real-time search?**
   - Yes → Use **BIN1-RABITQ** (75ms queries)
   - No → Consider INT8-CENTROID for quality

### Common Mistakes to Avoid
❌ Don't use BIN1-BETTER - performs poorly on normalized data  
❌ Don't use ROTATIONAL modes - overhead not worth it  
❌ Don't use INT4 - too slow for the accuracy gain  
❌ Don't forget to provide training samples for CENTROID/RABITQ modes  

---

## License & Attribution

Based on HNSWlib with extensions for advanced quantization techniques including RaBitQ-inspired residual encoding.
