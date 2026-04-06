# 📘 High-Recall Test Dataset Generator for SBERT/HNSW

This module provides a *clustered synthetic dataset* for benchmarking
HNSW, quantization, and approximate nearest neighbor algorithms.

The dataset is designed to produce:

- High recall values (0.95–1.00 for Float32)
- Stable nearest-neighbor structure
- Smooth distance gradients
- Well-separated clusters
- High-quality ground truth for recall@k testing

---

## 🚀 Features

### ✔ Generates:
- `vectors`: dataset to index
- `queries`: query set
- `ground_truth`: exact top-k neighbors for each query

### ✔ Uses:
- 100 clusters (default)
- 1000 vectors per cluster
- 384D unit vectors (SBERT-like)
- Gaussian noise perturbation (stddev=0.05)

You can adjust all parameters.

---

## 📦 API Summary

### `TestData generate_test_data(...)`

```cpp
TestData generate_test_data(
    size_t num_clusters = 100,
    size_t per_cluster = 1000,
    size_t dim = 384,
    size_t num_queries = 50,
    float noise_std = 0.05f,
    size_t gt_k = 10
);

