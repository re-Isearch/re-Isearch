#pragma once
#include "space_quantized_ip.h"

/*
Cosine Similarity Quantized Space for HNSW

This space handles non-normalized vectors and computes cosine similarity:
  cosine_sim(a, b) = (a·b) / (||a|| * ||b||)
  distance = 1 - cosine_sim

Key insight: Normalize BEFORE quantization, then quantize the normalized vector.
The quantized representation (e.g., BIN1) represents the normalized vector.
Distance between quantized vectors ≈ cosine distance of original vectors.

No need to store norms since we're quantizing normalized vectors!
*/

namespace hnswlib {

static constexpr float cEPS = 1e-9f;

template<typename T=float>
class SpaceQuantizedCosine : public SpaceInterface<float> {
public:
    using DISTFUNC_TYPE = DISTFUNC<float>;

private:
    size_t dim_;
    SpaceQuantizedIP<T> inner_space_;
    
    // SIMD-optimized normalization
    void normalize_vector(const T* input, T* output) const {
        T norm_sq = 0;
        
#if defined(HNSW_SIMD_AVX2)
        size_t i = 0;
        __m256 sum = _mm256_setzero_ps();
        
        for (; i + 7 < dim_; i += 8) {
            __m256 v = _mm256_loadu_ps(input + i);
            sum = _mm256_fmadd_ps(v, v, sum);
        }
        
        alignas(32) float tmp[8];
        _mm256_store_ps(tmp, sum);
        norm_sq = tmp[0] + tmp[1] + tmp[2] + tmp[3] + tmp[4] + tmp[5] + tmp[6] + tmp[7];
        
        for (; i < dim_; ++i) {
            norm_sq += input[i] * input[i];
        }
#elif defined(HNSW_SIMD_NEON)
        size_t i = 0;
        float32x4_t sum = vdupq_n_f32(0.0f);
        
        for (; i + 3 < dim_; i += 4) {
            float32x4_t v = vld1q_f32(input + i);
            sum = vmlaq_f32(sum, v, v);
        }
        
        float tmp[4];
        vst1q_f32(tmp, sum);
        norm_sq = tmp[0] + tmp[1] + tmp[2] + tmp[3];
        
        for (; i < dim_; ++i) {
            norm_sq += input[i] * input[i];
        }
#else
        for (size_t i = 0; i < dim_; ++i) {
            norm_sq += input[i] * input[i];
        }
#endif
        
        T norm = std::sqrt(norm_sq);
        
        if (norm > cEPS) {
            T inv_norm = T(1) / norm;
#if defined(HNSW_SIMD_AVX2)
            size_t i = 0;
            __m256 inv = _mm256_set1_ps(inv_norm);
            for (; i + 7 < dim_; i += 8) {
                __m256 v = _mm256_loadu_ps(input + i);
                v = _mm256_mul_ps(v, inv);
                _mm256_storeu_ps(output + i, v);
            }
            for (; i < dim_; ++i) {
                output[i] = input[i] * inv_norm;
            }
#elif defined(HNSW_SIMD_NEON)
            size_t i = 0;
            float32x4_t inv = vdupq_n_f32(inv_norm);
            for (; i + 3 < dim_; i += 4) {
                float32x4_t v = vld1q_f32(input + i);
                v = vmulq_f32(v, inv);
                vst1q_f32(output + i, v);
            }
            for (; i < dim_; ++i) {
                output[i] = input[i] * inv_norm;
            }
#else
            for (size_t i = 0; i < dim_; ++i) {
                output[i] = input[i] * inv_norm;
            }
#endif
        } else {
            // Handle zero/near-zero vectors - just copy
            std::memcpy(output, input, dim_ * sizeof(T));
        }
    }

public:
    explicit SpaceQuantizedCosine(size_t dim,
                                  QuantModeIP qmode,
                                  OptBinModeIP bin_mode = OptBinModeIP::STANDARD,
                                  const std::vector<std::vector<T>>* sample_embeddings = nullptr,
                                  size_t buffer_capacity = 1000)
        : dim_(dim),
          inner_space_(dim, qmode, bin_mode, 
                      sample_embeddings ? &normalize_samples_for_training(*sample_embeddings) : nullptr,
                      buffer_capacity,
                      false) {  // Don't use IP's internal normalization tracking
        
        HNSWDEBUG << "  [SpaceQuantizedCosine: normalizes vectors BEFORE quantization]";
        HNSWDEBUG << "  [Quantized vectors represent normalized originals → cosine similarity]";
    }
    
    // Normalize samples for training (returns new vector)
    std::vector<std::vector<T>> normalize_samples_for_training(const std::vector<std::vector<T>>& samples) {
        std::vector<std::vector<T>> normalized(samples.size());
        for (size_t i = 0; i < samples.size(); ++i) {
            normalized[i].resize(dim_);
            normalize_vector(samples[i].data(), normalized[i].data());
        }
        return normalized;
    }

    // Interface compliance
    size_t get_bytes_per_vector() override {
       return  bytes_per_vector;                
    }
    size_t get_data_size() override { 
        return inner_space_.get_data_size();  // No extra storage needed!
    }
    
    DISTFUNC_TYPE get_dist_func() override { 
        return inner_space_.get_dist_func();  // Use IP distance directly
    }
    
    void* get_dist_func_param() override { 
        return inner_space_.get_dist_func_param();  // Forward to IP space
    }

    // Quantize: normalize THEN quantize
    void quantize(const T* emb, uint8_t* out) const override {
        std::vector<T> normalized(dim_);
        normalize_vector(emb, normalized.data());
        
        // Quantize the normalized vector (e.g., BIN1, INT8, etc.)
        // The quantized representation encodes the normalized vector
        inner_space_.quantize(normalized.data(), out);
    }

    // Centroid operations (forward to inner space after normalization)
    void add_to_centroid(const T* emb) {
        std::vector<T> normalized(dim_);
        normalize_vector(emb, normalized.data());
        inner_space_.add_to_centroid(normalized.data());
    }
    
    void flush_centroid_buffer() {
        inner_space_.flush_centroid_buffer();
    }
    
    bool save_centroid(const std::string& filepath) const {
        return inner_space_.save_centroid(filepath);
    }
    
    bool load_centroid(const std::string& filepath) {
        return inner_space_.load_centroid(filepath);
    }
    
    std::vector<T> get_centroid() {
        return inner_space_.get_centroid();
    }
    
    size_t get_centroid_count() const {
        return inner_space_.get_centroid_count();
    }
    
    size_t get_buffer_count() const {
        return inner_space_.get_buffer_count();
    }
    
    // Access to underlying IP space (for advanced use)
    SpaceQuantizedIP<T>& get_inner_space() {
        return inner_space_;
    }
    
    const SpaceQuantizedIP<T>& get_inner_space() const {
        return inner_space_;
    }
    
    // Static helper to normalize a vector in-place
    static void normalize_inplace(T* vec, size_t dim) {
        T norm_sq = 0;
        for (size_t i = 0; i < dim; ++i) {
            norm_sq += vec[i] * vec[i];
        }
        T norm = std::sqrt(norm_sq);
        
        if (norm > cEPS) {
            T inv_norm = T(1) / norm;
            for (size_t i = 0; i < dim; ++i) {
                vec[i] *= inv_norm;
            }
        }
    }
    
    // Static helper to normalize a batch of vectors
    static void normalize_batch(std::vector<std::vector<T>>& vectors) {
        for (auto& vec : vectors) {
            normalize_inplace(vec.data(), vec.size());
        }
    }

    // Fit/train the quantization parameters from sample embeddings
    void fit(const std::vector<std::vector<T>>& sample_embeddings) override {
        if (sample_embeddings.empty()) {
            HNSWDEBUG << "  [SpaceQuantizedCosine::fit] Warning: empty sample set provided";
            return;
        }
        
        HNSWDEBUG << "  [SpaceQuantizedCosine::fit] Training on " << sample_embeddings.size() << " samples";
        
        // Normalize samples first
        auto normalized = normalize_samples_for_training(sample_embeddings);
        
        // Delegate to inner IP space for actual training
        inner_space_.fit(normalized);
        
        HNSWDEBUG << "  [SpaceQuantizedCosine::fit] Training complete";
    }

    // Persistance

    bool save_quantization_params(const std::string& filepath) const {
        // Delegate to inner IP space
        return inner_space_.save_quantization_params(filepath);
    }
    bool save_quantization_params(std::ofstream &out) const {
        // Delegate to inner IP space
        return inner_space_.save_quantization_params(out);
    }

    bool load_quantization_params(const std::string& filepath) {
        // Delegate to inner IP space
        return inner_space_.load_quantization_params(filepath);
    }

    bool load_quantization_params(std::ifstream &in) {
        // Delegate to inner IP space
        return inner_space_.load_quantization_params(in);
    }


    // Getters (delegate to inner space)
    const std::vector<T>& get_mean() const { return inner_space_.get_mean(); }
    const std::vector<T>& get_scale() const { return inner_space_.get_scale(); }
    const std::vector<T>& get_offset() const { return inner_space_.get_offset(); }
    bool is_using_rotation() const { return inner_space_.is_using_rotation(); }
    bool is_using_rabitq() const { return inner_space_.is_using_rabitq(); }
    size_t get_residual_dims() const { return inner_space_.get_residual_dims(); }
};

// Convenience type aliases
using SpaceQuantizedCosineF = SpaceQuantizedCosine<float>;
using SpaceQuantizedCosineD = SpaceQuantizedCosine<double>;

} // namespace hnswlib

/*
Usage example for Cosine Similarity with non-normalized vectors:

// Create space - normalizes vectors before quantization
auto space = new hnswlib::SpaceQuantizedCosine<float>(
    dim, 
    hnswlib::QuantModeIP::BIN1,       // Binary quantization
    hnswlib::OptBinModeIP::RABITQ,    // With RaBitQ refinement
    &training_samples,                 // Can be non-normalized!
    1000
);

// Create HNSW index
auto index = new hnswlib::HierarchicalNSW<float>(space, max_elements);

// Add points - input vectors can be non-normalized
for (size_t i = 0; i < num_vectors; ++i) {
    // Space will normalize BEFORE quantization
    space->add_to_centroid(vectors[i].data());
    
    std::vector<uint8_t> code(space->get_data_size());
    space->quantize(vectors[i].data(), code.data());  // Normalizes internally
    index->addPoint(code.data(), i);
}

// Query - also works with non-normalized vectors
std::vector<uint8_t> query_code(space->get_data_size());
space->quantize(query_vector.data(), query_code.data());  // Normalizes internally
auto results = index->searchKnn(query_code.data(), k);

// The distance returned approximates: 1 - cosine_similarity
// where cosine_similarity = (a·b) / (||a|| * ||b||)

How it works:
1. Input: non-normalized vectors a, b
2. Normalize: a' = a/||a||, b' = b/||b||  (both have norm=1)
3. Quantize: Q(a'), Q(b')  (e.g., binary, int8, etc.)
4. Distance: 1 - approximate_dot_product(Q(a'), Q(b'))
5. Since ||a'|| = ||b'|| = 1:
   approximate_dot_product(Q(a'), Q(b')) ≈ a'·b' = (a·b)/(||a||·||b||) = cosine_sim(a,b)

Key advantages:
- No extra storage needed (no norms to store)
- Works with any quantization mode (BIN1, INT4, INT8, RaBitQ)
- Input vectors don't need to be pre-normalized
- Quantized representation is compact and efficient
- Distance computation is just the IP distance (no extra calculations)
*/
