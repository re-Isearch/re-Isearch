#pragma once
#include "quantized.h"

#include <mutex>
#include <random>

#include "pearson_corr.h"

/*
Inner Product Quantized Space for HNSW

Key differences from L2:
- Distance = 1 - dot_product (to maintain "smaller is closer" convention)
- Binary quantization uses sign bits (positive vs negative after mean-centering)
- INT4/INT8 quantization preserves dot product structure
- RaBitQ stores residuals for dot product refinement

Features:
- Distance Computation: Returns 1 - dot_product to maintain the "smaller is closer"
  convention that HNSW expects
- Binary Quantization: Uses sign bits based on mean-centering:
  bit=1 for values > mean
  bit=0 for values ≤ mean
  Counts matching bits (not mismatching like Hamming)
- Mean-Centering: All quantization is performed on mean-centered data, which is crucial
  for inner product to work properly
- Quantization Parameters: Uses offset and scale_rcp (reciprocal) instead of minval and
  scale for more efficient inner product computation
- RaBitQ Adaptation: Stores residuals that capture dot product errors rather than L2 distance errors
- Normalization Support: Optional flag to handle pre-normalized vectors (common in IP scenarios, eg. Cosine)
*/

#if defined(__AVX512F__)
  #include <immintrin.h>
  #define HNSW_SIMD_AVX512
#elif defined(__AVX2__)
  #include <immintrin.h>
  #define HNSW_SIMD_AVX2
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
  #include <arm_neon.h>
  #define HNSW_SIMD_NEON
#elif defined(__ARM_FEATURE_SVE)
  #include <arm_sve.h>
  #define HNSW_SIMD_SVE
#endif

namespace hnswlib {

#if 1

typedef QuantMode  QuantModeIP;
typedef OptBinMode OptBinModeIP;

#else

enum class QuantModeIP {
    BIN1=0, INT158=1, INT4=2, INT8=3, INT16=4};

enum class OptBinModeIP  { PASS=0, STANDARD, BETTER, CENTROID, ROTATIONAL, RABITQ, RABITQ_EXTENDED };

// Conversion functions
inline std::optional<QuantModeIP> toQuantModeIP(StorageType st) noexcept {
    switch (st) {
        case StorageType::BIN1:
            return QuantModeIP::BIN1;
        case StorageType::INT2:
        case StorageType::INT3:
            return QuantModeIP::INT158;
        case StorageType::INT4:
        case StorageType::INT5:
            return QuantModeIP::INT4;
        case StorageType::INT6:
        case StorageType::INT8:
            return QuantModeIP::INT8;
        default:
            return std::nullopt;
    }
}

inline StorageType toStorageTypeIP(QuantModeIP mode) noexcept {
    switch (mode) {
        case QuantModeIP::BIN1:  return StorageType::BIN1;
        case QuantModeIP::INT158:return StorageType::INT2;
        case QuantModeIP::INT4:  return StorageType::INT4;
        case QuantModeIP::INT8:  return StorageType::INT8;
        case QuantModeIP::INT16: return StorageType::INT16;
    }
}
#endif


#if defined(__AVX2__)
#include <immintrin.h>

inline float ip_int8_avx2(const uint8_t* a, const uint8_t* b, size_t dim)
{
    __m256i acc = _mm256_setzero_si256();
    size_t i = 0;

    for (; i + 31 < dim; i += 32) {
        __m256i va = _mm256_loadu_si256((const __m256i*)(a + i));
        __m256i vb = _mm256_loadu_si256((const __m256i*)(b + i));

        __m256i a0 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(va, 0));
        __m256i b0 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(vb, 0));
        acc = _mm256_add_epi32(acc, _mm256_madd_epi16(a0, b0));

        __m256i a1 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(va, 1));
        __m256i b1 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(vb, 1));
        acc = _mm256_add_epi32(acc, _mm256_madd_epi16(a1, b1));
    }

    alignas(32) int tmp[8];
    _mm256_store_si256((__m256i*)tmp, acc);
    int sum = tmp[0]+tmp[1]+tmp[2]+tmp[3]+tmp[4]+tmp[5]+tmp[6]+tmp[7];

    for (; i < dim; ++i)
        sum += int(int8_t(a[i])) * int(int8_t(b[i]));

    return float(-sum);
}

inline float ip_int4_avx2(const uint8_t* a, const uint8_t* b, size_t dim)
{
    size_t bytes = (dim + 1) >> 1;
    __m256i acc = _mm256_setzero_si256();
    const __m256i mask = _mm256_set1_epi8(0x0F);

    size_t i = 0;
    for (; i + 31 < bytes; i += 32) {
        __m256i va = _mm256_loadu_si256((const __m256i*)(a + i));
        __m256i vb = _mm256_loadu_si256((const __m256i*)(b + i));

        __m256i a0 = _mm256_and_si256(va, mask);
        __m256i b0 = _mm256_and_si256(vb, mask);
        a0 = _mm256_sub_epi8(a0, _mm256_cmpgt_epi8(a0, _mm256_set1_epi8(7)));
        b0 = _mm256_sub_epi8(b0, _mm256_cmpgt_epi8(b0, _mm256_set1_epi8(7)));

        acc = _mm256_add_epi32(acc,
            _mm256_madd_epi16(
                _mm256_cvtepi8_epi16(_mm256_extracti128_si256(a0,0)),
                _mm256_cvtepi8_epi16(_mm256_extracti128_si256(b0,0))
            ));

        __m256i a1 = _mm256_and_si256(_mm256_srli_epi16(va,4), mask);
        __m256i b1 = _mm256_and_si256(_mm256_srli_epi16(vb,4), mask);
        a1 = _mm256_sub_epi8(a1, _mm256_cmpgt_epi8(a1, _mm256_set1_epi8(7)));
        b1 = _mm256_sub_epi8(b1, _mm256_cmpgt_epi8(b1, _mm256_set1_epi8(7)));

        acc = _mm256_add_epi32(acc,
            _mm256_madd_epi16(
                _mm256_cvtepi8_epi16(_mm256_extracti128_si256(a1,0)),
                _mm256_cvtepi8_epi16(_mm256_extracti128_si256(b1,0))
            ));
    }

    alignas(32) int tmp[8];
    _mm256_store_si256((__m256i*)tmp, acc);
    int sum = tmp[0]+tmp[1]+tmp[2]+tmp[3]+tmp[4]+tmp[5]+tmp[6]+tmp[7];

    for (; i < bytes; ++i) {
        uint8_t ba = a[i], bb = b[i];
        int a0 = ba & 0x0F; if (a0 >= 8) a0 -= 16;
        int b0 = bb & 0x0F; if (b0 >= 8) b0 -= 16;
        sum += a0 * b0;
        if (2*i + 1 < dim) {
            int a1 = ba >> 4; if (a1 >= 8) a1 -= 16;
            int b1 = bb >> 4; if (b1 >= 8) b1 -= 16;
            sum += a1 * b1;
        }
    }

    return float(-sum);
}

#endif

#if defined(__ARM_NEON)
#include <arm_neon.h>

inline float ip_int8_neon(const uint8_t* a,
                          const uint8_t* b,
                          size_t dim)
{
    int32x4_t acc = vdupq_n_s32(0);
    size_t i = 0;

    for (; i + 15 < dim; i += 16) {
        int8x16_t va = vld1q_s8((const int8_t*)(a + i));
        int8x16_t vb = vld1q_s8((const int8_t*)(b + i));
        int16x8_t a0 = vmovl_s8(vget_low_s8(va));
        int16x8_t b0 = vmovl_s8(vget_low_s8(vb));
        int16x8_t a1 = vmovl_s8(vget_high_s8(va));
        int16x8_t b1 = vmovl_s8(vget_high_s8(vb));

        acc = vaddq_s32(acc, vmull_s16(vget_low_s16(a0), vget_low_s16(b0)));
        acc = vaddq_s32(acc, vmull_s16(vget_high_s16(a0), vget_high_s16(b0)));
        acc = vaddq_s32(acc, vmull_s16(vget_low_s16(a1), vget_low_s16(b1)));
        acc = vaddq_s32(acc, vmull_s16(vget_high_s16(a1), vget_high_s16(b1)));
    }

    int sum = vaddvq_s32(acc);
    for (; i < dim; ++i)
        sum += int(int8_t(a[i])) * int(int8_t(b[i]));

    return float(-sum);
}
#endif



// =============================================================================
template<typename T=float>
class SpaceQuantizedIP : public SpaceInterface<float> {
public:
    using DISTFUNC_TYPE = DISTFUNC<float>;

    size_t dim_;
    StorageType storage_type_;
    QuantModeIP qmode_;
    OptBinModeIP bin_mode_;
    size_t bytes_per_vector =0;

    // Mean vector for centering (crucial for IP)
    std::vector<T> mean_;
    
    // Quantization parameters
    std::vector<T> scale, offset, scale_rcp;  // offset replaces minval for IP
    
    // Ternary quantization state
    std::vector<T> ternary_threshold_low_;
    std::vector<T> ternary_threshold_high_;
    std::vector<T> ternary_scale_;
    
    // Rotational quantization state
    std::vector<T> rotation_matrix_;
    bool use_rotation_;
    
    // RaBitQ state
    std::vector<std::vector<T>> residuals_;
    size_t residual_dims_;
    bool use_rabitq_;
    std::mutex residual_mutex_;
    
    // Centroid-based quantization state
    std::vector<T> centroid_;
    std::vector<T> sum_;
    size_t count_;
    std::vector<T> buffer_;
    size_t buffer_capacity_;
    size_t buffer_count_;
    mutable std::mutex centroid_mutex_;

    // Normalization factors (optional, for normalized IP)
    std::vector<T> norms_;
    bool use_normalization_;

#if 0 /* FIXUP */

    explicit SpaceQuantizedIP(size_t dim, StorageType storage_type,
                              OptBinModeIP bin_mode = OptBinModeIP::STANDARD,
                              const std::vector<std::vector<T>>* sample_embeddings = nullptr,
                              size_t buffer_capacity = 1000,
                              bool normalize = false)
        : dim_(dim), storage_type_(storage_type), bin_mode_(bin_mode), buffer_capacity_(buffer_capacity),
          use_normalization_(normalize) {

        qmode_ = toQuantMode(storage_type_);
#else
    explicit SpaceQuantizedIP(size_t dim, QuantModeIP qmode,
                              OptBinModeIP bin_mode = OptBinModeIP::STANDARD,
                              const std::vector<std::vector<T>>* sample_embeddings = nullptr,
                              size_t buffer_capacity = 1000,
                              bool normalize = false)
        : dim_(dim), qmode_(qmode), bin_mode_(bin_mode), buffer_capacity_(buffer_capacity),
          use_normalization_(normalize) {

       storage_type_ = toStorageType(qmode_); 
#endif

        assert(dim>0);

	count_         = 0;
	buffer_count_  = 0;
        use_rotation_  = (bin_mode_ == OptBinModeIP::ROTATIONAL);
        use_rabitq_    = (bin_mode_ == OptBinModeIP::RABITQ || bin_mode_ == OptBinModeIP::RABITQ_EXTENDED);
        residual_dims_ = (use_rabitq_ ?
                        (bin_mode_ == OptBinModeIP::RABITQ_EXTENDED ?
                         std::min(size_t(64), dim_ / 4) :
                         std::min(size_t(16), dim_ / 8))
                        : 0);
        
        // Initialize mean
        mean_.assign(dim_, T(0));
        
        if (use_normalization_) {
            norms_.reserve(10000); // Pre-allocate for efficiency
        }
        
#if 0
        if (use_rabitq_) {
            if (bin_mode_ == OptBinModeIP::RABITQ_EXTENDED) {
                HNSWDEBUG << "  [RaBitQ-Extended IP: keeping " << residual_dims_ << " residual dims]";
            } else {
                HNSWDEBUG << "  [RaBitQ IP: keeping " << residual_dims_ << " residual dims]";
            }
        }
#endif
        
        if (use_rotation_) {
            rotation_matrix_.resize(dim_ * dim_);
            if (sample_embeddings) {
                compute_rotation_matrix(*sample_embeddings);
            } else {
                // Identity matrix
                for (size_t i = 0; i < dim_; ++i) {
                    for (size_t j = 0; j < dim_; ++j) {
                        rotation_matrix_[i * dim_ + j] = (i == j) ? T(1) : T(0);
                    }
                }
            }
        }
        
        if (bin_mode == OptBinModeIP::CENTROID || bin_mode == OptBinModeIP::RABITQ) {
            if (centroid_.empty()) {
                centroid_.assign(dim_, T(0));
                sum_.assign(dim_, T(0));
            }
        }

        if (bin_mode == OptBinModeIP::PASS) {
            // Passthrough mode
#if 1 /* FIXUP  */    
            size_t bits_per_component = hnswlib::IntStorage::bits_per_element(storage_type_);
            size_t vector_bits = dim * bits_per_component;
            bytes_per_vector= (vector_bits + 7) / 8;
#endif
#if 1                       
        } else if (qmode == QuantModeIP::INT16) {
            bytes_per_vector=  sizeof(float) + dim * sizeof(int16_t);
#endif
        } else if (qmode==QuantModeIP::BIN1) {
            bytes_per_vector=(dim_+7)/8;
            
            if (sample_embeddings) {
                compute_mean(*sample_embeddings);
                
                if (bin_mode == OptBinModeIP::CENTROID || bin_mode == OptBinModeIP::RABITQ) {
                    train_centroid(*sample_embeddings);
                }
            }
        } else if (qmode==QuantModeIP::INT158) {
            bytes_per_vector = (dim_ * 2 + 7) / 8;
            ternary_threshold_low_.assign(dim_, T(0));
            ternary_threshold_high_.assign(dim_, T(0));
            ternary_scale_.assign(dim_, T(1));
            
            if (sample_embeddings) {
                compute_mean(*sample_embeddings);
                if (bin_mode == OptBinModeIP::CENTROID) {
                    train_centroid(*sample_embeddings);
                    compute_ternary_thresholds_from_centroid();
                } else {
                    compute_ternary_thresholds(*sample_embeddings);
                }
            }
        } else {
            // INT4 or INT8
            bytes_per_vector=(qmode==QuantModeIP::INT8)?dim_:(dim_+1)/2;
            scale.assign(dim_,1); offset.assign(dim_,0); scale_rcp.assign(dim_,1);
            
            if (sample_embeddings) {
                compute_mean(*sample_embeddings);
                if (bin_mode == OptBinModeIP::CENTROID) {
                    train_centroid(*sample_embeddings);
                    compute_scale_offset_from_centroid(*sample_embeddings, 
                                                      (qmode==QuantModeIP::INT8)?255.0:15.0);
                } else {
                    compute_scale_offset(*sample_embeddings,
                                        (qmode==QuantModeIP::INT8)?255.0:15.0);
                }
            }
        }
    }

    size_t get_bytes_per_vector() override {
       return  bytes_per_vector;                
    }
    size_t get_data_size() override { 
        size_t base_size = bytes_per_vector;
        if (use_rabitq_) {
            base_size += residual_dims_ * sizeof(T);
        }
        return base_size;
    }
    
    DISTFUNC_TYPE get_dist_func() override { return &SpaceQuantizedIP::fstdist_; }
    void* get_dist_func_param() override { return this; }

    // -------------------------------------------------------------------------
    void quantize(const T* emb, uint8_t* out) const override {
        if (bin_mode_ == OptBinMode::PASS || qmode_ == QuantMode::NONE || 
                qmode_ == QuantMode::FP16 || qmode_ == QuantMode::BF16) {
#if 1 /* FIXUP */
           IntStorage::quantize(storage_type_, emb, out, dim_);
#else
           auto st = toStorageType(qmode_);
           IntStorage::quantize(st, emb, out, dim_);
#endif
           return;
        }

        // Apply rotation if enabled
        std::vector<T> rotated;
        const T* input = emb;
        
        if (use_rotation_) {
            rotated.resize(dim_);
            apply_rotation(emb, rotated.data());
            input = rotated.data();
        }
        
        // Quantize main vector
        switch(qmode_){
            case QuantModeIP::BIN1: quantize_bin(input,out); break;
            case QuantModeIP::INT158: quantize_ternary_simd(input,out); break;
            case QuantModeIP::INT8: quantize_int8_simd(input,out); break;
            case QuantModeIP::INT4: quantize_int4_simd(input,out); break;

	    case QuantModeIP::INT16:
		HNSWFATAL << "Don't yet support INT16 Quant on IP\n";
		break;
            case QuantModeIP::FP16: IntStorage::quantize(StorageType::FP16, input, out, dim_); break;
            case QuantModeIP::BF16: IntStorage::quantize(StorageType::BF16, input, out, dim_); break;
	    case QuantModeIP::NONE: IntStorage::quantize(StorageType::FLOAT32, input, out, dim_); break;
        }
        
        // For RaBitQ, add residual
        if (use_rabitq_ && qmode_ == QuantModeIP::BIN1) {
            T* residual_ptr = reinterpret_cast<T*>(out + bytes_per_vector);
            compute_residual(input, out, residual_ptr);
        }
    }
    
    // Store normalization factor for a vector (call before quantization)
    void store_norm(const T* emb) {
        if (!use_normalization_) return;
        
        T norm_sq = 0;
        for (size_t i = 0; i < dim_; ++i) {
            norm_sq += emb[i] * emb[i];
        }
        norms_.push_back(std::sqrt(norm_sq));
    }
    
    // =================== MEAN COMPUTATION ===================================
    void compute_mean(const std::vector<std::vector<T>>& samples) {
        const size_t n = samples.size();
        if (n == 0) return;
        
        std::fill(mean_.begin(), mean_.end(), T(0));
        
        for (size_t i = 0; i < n; ++i) {
            for (size_t d = 0; d < dim_; ++d) {
                mean_[d] += samples[i][d];
            }
        }
        
        for (size_t d = 0; d < dim_; ++d) {
            mean_[d] /= T(n);
        }
        
        // HNSWDEBUG << "  [Computed mean for IP quantization]";
    }
    
    // =================== RESIDUAL COMPUTATION ================================
    void compute_residual(const T* original, const uint8_t* binary_code, T* residual) const {
        // Reconstruct approximate dot product contribution per dimension
        std::vector<std::pair<T, size_t>> errors;
        errors.reserve(dim_);
        
        for (size_t i = 0; i < dim_; ++i) {
            size_t byte_idx = i / 8;
            size_t bit_idx = i % 8;
            bool bit = (binary_code[byte_idx] >> bit_idx) & 1;
            
            // For IP: bit=1 means positive, bit=0 means negative after mean-centering
            T centered = original[i] - mean_[i];
            T reconstructed = bit ? std::abs(centered) : -std::abs(centered);
            T error = centered - reconstructed;
            
            errors.push_back({std::abs(error), i});
        }
        
        // Sort by error magnitude
        std::partial_sort(errors.begin(), 
                         errors.begin() + residual_dims_,
                         errors.end(),
                         [](const auto& a, const auto& b) { return a.first > b.first; });
        
        // Store residuals for top dimensions
        for (size_t i = 0; i < residual_dims_; ++i) {
            size_t dim_idx = errors[i].second;
            
            size_t byte_idx = dim_idx / 8;
            size_t bit_idx = dim_idx % 8;
            bool bit = (binary_code[byte_idx] >> bit_idx) & 1;
            
            T centered = original[dim_idx] - mean_[dim_idx];
            T reconstructed = bit ? std::abs(centered) : -std::abs(centered);
            residual[i] = centered - reconstructed;
        }
    }
    
    void compute_rotation_matrix(const std::vector<std::vector<T>>& samples) {
        const size_t n = samples.size();
        if (n == 0) return;
        
        // HNSWDEBUG << "  Computing rotation matrix via PCA...";
        
        // Random orthogonal matrix using Gram-Schmidt
        std::mt19937 rng(42);
        std::normal_distribution<T> dist(0, 1);
        
        for (size_t i = 0; i < dim_; ++i) {
            std::vector<T> vec(dim_);
            for (size_t j = 0; j < dim_; ++j) {
                vec[j] = dist(rng);
            }
            
            // Gram-Schmidt orthogonalization
            for (size_t k = 0; k < i; ++k) {
                T dot = 0;
                for (size_t j = 0; j < dim_; ++j) {
                    dot += vec[j] * rotation_matrix_[k * dim_ + j];
                }
                for (size_t j = 0; j < dim_; ++j) {
                    vec[j] -= dot * rotation_matrix_[k * dim_ + j];
                }
            }
            
            // Normalize
            T norm = 0;
            for (size_t j = 0; j < dim_; ++j) {
                norm += vec[j] * vec[j];
            }
            norm = std::sqrt(norm);
            
            if (norm > 1e-6) {
                for (size_t j = 0; j < dim_; ++j) {
                    rotation_matrix_[i * dim_ + j] = vec[j] / norm;
                }
            } else {
                for (size_t j = 0; j < dim_; ++j) {
                    rotation_matrix_[i * dim_ + j] = (i == j) ? T(1) : T(0);
                }
            }
        }
        
        // HNSWDEBUG << " done";
    }
    
    void apply_rotation(const T* input, T* output) const {
#if defined(HNSW_SIMD_AVX2)
        for (size_t i = 0; i < dim_; ++i) {
            size_t j = 0;
            __m256 sum = _mm256_setzero_ps();
            
            for (; j + 7 < dim_; j += 8) {
                __m256 row = _mm256_loadu_ps(&rotation_matrix_[i * dim_ + j]);
                __m256 inp = _mm256_loadu_ps(&input[j]);
                sum = _mm256_fmadd_ps(row, inp, sum);
            }
            
            alignas(32) float tmp[8];
            _mm256_store_ps(tmp, sum);
            output[i] = tmp[0] + tmp[1] + tmp[2] + tmp[3] + tmp[4] + tmp[5] + tmp[6] + tmp[7];
            
            for (; j < dim_; ++j) {
                output[i] += rotation_matrix_[i * dim_ + j] * input[j];
            }
        }
#elif defined(HNSW_SIMD_NEON)
        for (size_t i = 0; i < dim_; ++i) {
            size_t j = 0;
            float32x4_t sum = vdupq_n_f32(0.0f);
            
            for (; j + 3 < dim_; j += 4) {
                float32x4_t row = vld1q_f32(&rotation_matrix_[i * dim_ + j]);
                float32x4_t inp = vld1q_f32(&input[j]);
                sum = vmlaq_f32(sum, row, inp);
            }
            
            float tmp[4];
            vst1q_f32(tmp, sum);
            output[i] = tmp[0] + tmp[1] + tmp[2] + tmp[3];
            
            for (; j < dim_; ++j) {
                output[i] += rotation_matrix_[i * dim_ + j] * input[j];
            }
        }
#else
        for (size_t i = 0; i < dim_; ++i) {
            output[i] = 0;
            for (size_t j = 0; j < dim_; ++j) {
                output[i] += rotation_matrix_[i * dim_ + j] * input[j];
            }
        }
#endif
    }
    
    void train_centroid(const std::vector<std::vector<T>>& samples) {
        std::lock_guard<std::mutex> lock(centroid_mutex_);
        
        const size_t n = samples.size();
        if (n == 0) return;
        
        std::fill(sum_.begin(), sum_.end(), T(0));
        count_ = 0;
        
        for (size_t i = 0; i < n; ++i) {
            for (size_t d = 0; d < dim_; ++d) {
                sum_[d] += samples[i][d];
            }
        }
        
        count_ = n;
        update_centroid();
    }
    
    void add_to_centroid(const T* emb) {
        if (bin_mode_ != OptBinModeIP::CENTROID) return;
        
        std::lock_guard<std::mutex> lock(centroid_mutex_);
        
        for (size_t d = 0; d < dim_; ++d) {
            buffer_.push_back(emb[d]);
        }
        buffer_count_++;
        
        if (count_ == 0 && buffer_count_ == 1) {
            for (size_t d = 0; d < dim_; ++d) {
                sum_[d] = emb[d];
                centroid_[d] = emb[d];
            }
            count_ = 1;
        }
        
        if (buffer_count_ >= buffer_capacity_) {
            std::vector<T> buffer_copy = buffer_;
            size_t buffer_count_copy = buffer_count_;
            
            for (size_t i = 0; i < buffer_count_; ++i) {
                const T* vec = buffer_.data() + i * dim_;
                for (size_t d = 0; d < dim_; ++d) {
                    sum_[d] += vec[d];
                }
            }
            count_ += buffer_count_;
            update_centroid();
            
            update_quantization_params(buffer_copy, buffer_count_copy);
            
            buffer_.clear();
            buffer_count_ = 0;
        }
    }
    
    void flush_centroid_buffer() {
        if (bin_mode_ != OptBinModeIP::CENTROID) return;
        
        std::lock_guard<std::mutex> lock(centroid_mutex_);
        if (buffer_count_ > 0) {
            std::vector<T> buffer_copy = buffer_;
            size_t buffer_count_copy = buffer_count_;
            
            for (size_t i = 0; i < buffer_count_; ++i) {
                const T* vec = buffer_.data() + i * dim_;
                for (size_t d = 0; d < dim_; ++d) {
                    sum_[d] += vec[d];
                }
            }
            count_ += buffer_count_;
            update_centroid();
            
            update_quantization_params(buffer_copy, buffer_count_copy);
            
            buffer_.clear();
            buffer_count_ = 0;
        }
    }
    
    bool save_centroid(const std::string& filepath) const {
        std::lock_guard<std::mutex> lock(centroid_mutex_);
        
        std::ofstream out(filepath, std::ios::binary);
        if (!out.is_open()) return false;
        
        out.write(reinterpret_cast<const char*>(&dim_), sizeof(dim_));
        out.write(reinterpret_cast<const char*>(&count_), sizeof(count_));
        out.write(reinterpret_cast<const char*>(&buffer_capacity_), sizeof(buffer_capacity_));
        
        out.write(reinterpret_cast<const char*>(centroid_.data()), dim_ * sizeof(T));
        out.write(reinterpret_cast<const char*>(sum_.data()), dim_ * sizeof(T));
        out.write(reinterpret_cast<const char*>(mean_.data()), dim_ * sizeof(T));
        
        out.write(reinterpret_cast<const char*>(&buffer_count_), sizeof(buffer_count_));
        if (buffer_count_ > 0) {
            out.write(reinterpret_cast<const char*>(buffer_.data()), 
                      buffer_count_ * dim_ * sizeof(T));
        }
        
        return out.good();
    }
    
    bool load_centroid(const std::string& filepath) {
        std::lock_guard<std::mutex> lock(centroid_mutex_);
        
        std::ifstream in(filepath, std::ios::binary);
        if (!in.is_open()) return false;
        
        size_t file_dim;
        in.read(reinterpret_cast<char*>(&file_dim), sizeof(file_dim));
        if (file_dim != dim_) return false;
        
        in.read(reinterpret_cast<char*>(&count_), sizeof(count_));
        in.read(reinterpret_cast<char*>(&buffer_capacity_), sizeof(buffer_capacity_));
        
        centroid_.resize(dim_);
        in.read(reinterpret_cast<char*>(centroid_.data()), dim_ * sizeof(T));
        
        sum_.resize(dim_);
        in.read(reinterpret_cast<char*>(sum_.data()), dim_ * sizeof(T));
        
        mean_.resize(dim_);
        in.read(reinterpret_cast<char*>(mean_.data()), dim_ * sizeof(T));
        
        in.read(reinterpret_cast<char*>(&buffer_count_), sizeof(buffer_count_));
        if (buffer_count_ > 0) {
            buffer_.resize(buffer_count_ * dim_);
            in.read(reinterpret_cast<char*>(buffer_.data()), 
                    buffer_count_ * dim_ * sizeof(T));
        }
        
        return in.good();
    }

  void fit(const std::vector<std::vector<T>>& sample_embeddings) override {
    if (sample_embeddings.empty()) {
        // HNSWDEBUG << "  [SpaceQuantizedIP::fit] Warning: empty sample set provided";
        return;
    }
    
    const size_t n = sample_embeddings.size();
    // HNSWDEBUG << "  [SpaceQuantizedIP::fit] Training on " << n << " samples, dim=" << dim_;
    
    // Compute mean
    compute_mean(sample_embeddings);
    
    // Initialize rotation matrix if needed
    if (use_rotation_) {
        compute_rotation_matrix(sample_embeddings);
    }
    
    // Initialize centroid if using CENTROID or RaBitQ modes
    if (bin_mode_ == OptBinModeIP::CENTROID || bin_mode_ == OptBinModeIP::RABITQ) {
        train_centroid(sample_embeddings);
    }
    
    // Compute quantization parameters based on mode
    if (qmode_ == QuantModeIP::BIN1) {
        // Binary: mean is already computed
        // HNSWDEBUG << "  [BIN1] Using mean-centered binary quantization";
    } else if (qmode_ == QuantModeIP::INT158) {
        // Ternary quantization
        if (bin_mode_ == OptBinModeIP::CENTROID) {
            compute_ternary_thresholds_from_centroid();
        } else {
            compute_ternary_thresholds(sample_embeddings);
        }
        // HNSWDEBUG << "  [INT158] Ternary thresholds computed";
    } else if (qmode_ == QuantModeIP::INT4 || qmode_ == QuantModeIP::INT8) {
        // INT4/INT8 quantization
        double levels = (qmode_ == QuantModeIP::INT8) ? 255.0 : 15.0;
        if (bin_mode_ == OptBinModeIP::CENTROID) {
            compute_scale_offset_from_centroid(sample_embeddings, levels);
        } else {
            compute_scale_offset(sample_embeddings, levels);
        }
        // HNSWDEBUG << "  [" << (qmode_ == QuantModeIP::INT8 ? "INT8" : "INT4") << "] Scale/offset computed";
    }
    
    // HNSWDEBUG << "  [SpaceQuantizedIP::fit] Training complete";
  }

private:
    // =================== DISTANCE (INNER PRODUCT) ===========================
    static float fstdist_(const void* p1, const void* p2, const void* param) {
        const auto* sp = reinterpret_cast<const SpaceQuantizedIP*>(param);
        float dist = sp->compute_dist(reinterpret_cast<const uint8_t*>(p1),
                                       reinterpret_cast<const uint8_t*>(p2));
        if (std::isnan(dist) || std::isinf(dist)) {
            HNSWERR << "Invalid distance computed by fstdist_ (SpaceQuantizedIP): " << dist;
            return 1e9;
        }
        return dist;
    }


    inline float finalize_dist(float dot) const {
      if (use_normalization_)
        return 1.0f - dot / dim_; // Normalize and convert to distance
      else
        return -dot;
    }

    // IP distance for pre-quantised vectors
    float compute_dist_pass(int bits, const uint8_t* a, const uint8_t* b, size_t dim) const
    {
        double acc = 0.0;

        // Fast path INT8
        if (bits == 8) {
            for (size_t i = 0; i < dim; ++i) {
                acc += int(int8_t(a[i])) * int(int8_t(b[i]));
            }
            return finalize_dist(acc);
        }

        // Fast path INT4
        if (bits == 4) {
            size_t bytes = (dim + 1) >> 1;
            for (size_t i = 0; i < bytes; ++i) {
                uint8_t ba = a[i];
                uint8_t bb = b[i];

                int a0 = ba & 0x0F; if (a0 >= 8) a0 -= 16;
                int b0 = bb & 0x0F; if (b0 >= 8) b0 -= 16;
                acc += a0 * b0;

                if (2*i + 1 < dim) {
                    int a1 = ba >> 4; if (a1 >= 8) a1 -= 16;
                    int b1 = bb >> 4; if (b1 >= 8) b1 -= 16;
                    acc += a1 * b1;
                }
            }
            return finalize_dist(acc);
        }

        // Generic bit-stream (INT1/2/3/5/6)
        const uint32_t mask = (1u << bits) - 1;
        const uint32_t sign = 1u << (bits - 1);

        size_t bitpos = 0, bytepos = 0;

        for (size_t d = 0; d < dim; ++d) {
            uint32_t va = a[bytepos] >> bitpos;
            uint32_t vb = b[bytepos] >> bitpos;

            if (bitpos + bits > 8) {
                va |= uint32_t(a[bytepos + 1]) << (8 - bitpos);
                vb |= uint32_t(b[bytepos + 1]) << (8 - bitpos);
            }

            va &= mask;
            vb &= mask;

            if (va & sign) va -= (1u << bits);
            if (vb & sign) vb -= (1u << bits);

            acc += int(va) * int(vb);

            bitpos += bits;
            bytepos += bitpos >> 3;
            bitpos &= 7;
        }

        return finalize_dist(acc);
    }


    float compute_dist(const uint8_t* a, const uint8_t* b) const {
        float dot_product = 0;

        if (bin_mode_ == OptBinMode::PASS) {
	  int bits = hnswlib::IntStorage::bits_per_element(storage_type_);
	  return compute_dist_pass(bits, a, b, dim_); 
        }
        
        if (qmode_==QuantModeIP::BIN1) {
            // Binary inner product: count matching bits
            // XOR gives differences, so we want bits that are the same
            uint32_t matches = 0;
#if 0 
            for(size_t i=0;i<bytes_per_vector;++i) {
                // Count bits that match (both 0 or both 1)
                uint8_t xor_val = a[i] ^ b[i];
                matches += 8 - __builtin_popcount(xor_val); // Count matching bits in byte
            }
#else
            for (size_t i = 0; i < bytes_per_vector; ++i) {
              uint8_t xor_val = a[i] ^ b[i];

              // Mask unused bits in last byte
              if (i == bytes_per_vector - 1 && (dim_ & 7)) {
                  uint8_t valid_mask = (1u << (dim_ & 7)) - 1;
                  xor_val &= valid_mask;
                  matches += (dim_ & 7) - __builtin_popcount(xor_val);
              } else {
                  matches += 8 - __builtin_popcount(xor_val);
              }
          }

#endif
            // Normalize to [-1, 1] range then convert to distance
            dot_product = (2.0f * matches / float(dim_)) - 1.0f;
            
            // RaBitQ refinement
            if (use_rabitq_) {
                const T* res_a = reinterpret_cast<const T*>(a + bytes_per_vector);
                const T* res_b = reinterpret_cast<const T*>(b + bytes_per_vector);
                
                float residual_dot = 0;
                
#if defined(HNSW_SIMD_AVX2)
                size_t i = 0;
                __m256 sum = _mm256_setzero_ps();
                
                for (; i + 7 < residual_dims_; i += 8) {
                    __m256 va = _mm256_loadu_ps(res_a + i);
                    __m256 vb = _mm256_loadu_ps(res_b + i);
                    sum = _mm256_fmadd_ps(va, vb, sum);
                }
                
                alignas(32) float tmp[8];
                _mm256_store_ps(tmp, sum);
                residual_dot = tmp[0] + tmp[1] + tmp[2] + tmp[3] + tmp[4] + tmp[5] + tmp[6] + tmp[7];
                
                for (; i < residual_dims_; ++i) {
                    residual_dot += res_a[i] * res_b[i];
                }
#elif defined(HNSW_SIMD_NEON)
                size_t i = 0;
                float32x4_t sum = vdupq_n_f32(0.0f);
                
                for (; i + 3 < residual_dims_; i += 4) {
                    float32x4_t va = vld1q_f32(res_a + i);
                    float32x4_t vb = vld1q_f32(res_b + i);
                    sum = vmlaq_f32(sum, va, vb);
                }
                
                float tmp[4];
                vst1q_f32(tmp, sum);
                residual_dot = tmp[0] + tmp[1] + tmp[2] + tmp[3];
                
                for (; i < residual_dims_; ++i) {
                    residual_dot += res_a[i] * res_b[i];
                }
#else
                for (size_t i = 0; i < residual_dims_; ++i) {
                    residual_dot += res_a[i] * res_b[i];
                }
#endif
                
                float residual_weight = (bin_mode_ == OptBinModeIP::RABITQ_EXTENDED) ? 0.5f : 0.3f;
                dot_product = (1.0f - residual_weight) * dot_product + 
                             residual_weight * residual_dot / residual_dims_;
            }
            
            // Convert to distance: 1 - dot_product (since we want smaller = closer)
            return 1.0f - dot_product;
            
        } else if (qmode_==QuantModeIP::INT158) {
            // Ternary inner product
            double acc = 0;
            size_t bit_idx = 0;
            for (size_t d = 0; d < dim_; ++d) {
                size_t byte_idx = bit_idx / 8;
                size_t bit_off = bit_idx % 8;
                
                uint8_t bits_a, bits_b;
                if (bit_off <= 6) {
                    bits_a = (a[byte_idx] >> bit_off) & 0x03;
                    bits_b = (b[byte_idx] >> bit_off) & 0x03;
                } else {
                    bits_a = ((a[byte_idx] >> 7) & 0x01) | 
                            ((a[byte_idx + 1] & 0x01) << 1);
                    bits_b = ((b[byte_idx] >> 7) & 0x01) | 
                            ((b[byte_idx + 1] & 0x01) << 1);
                }
                
                // Decode: 00=-1, 01=0, 10=+1
                int val_a = (bits_a == 0) ? -1 : ((bits_a == 1) ? 0 : 1);
                int val_b = (bits_b == 0) ? -1 : ((bits_b == 1) ? 0 : 1);
                
                acc += val_a * val_b * double(ternary_scale_[d]);
                
                bit_idx += 2;
            }
            dot_product = float(acc);
	    return finalize_dist(dot_product);
        } else if (qmode_==QuantModeIP::INT8) {
            // INT8 inner product
            double acc = 0;
            for(size_t i = 0; i < dim_; ++i) {
                // Dequantize: value = (quantized - offset) * scale
                T val_a = (T(a[i]) - offset[i]) * scale[i];
                T val_b = (T(b[i]) - offset[i]) * scale[i];
                acc += val_a * val_b;
            }
            dot_product = float(acc);
            return 1.0f - (dot_product / dim_); // Normalize and convert to distance
            
        } else { // INT4
            double acc = 0;
            for(size_t d=0; d<dim_; d+=2){
                uint8_t ba=a[d>>1], bb=b[d>>1];
                uint8_t a0=ba&0x0F, a1=ba>>4, b0=bb&0x0F, b1=bb>>4;
                
                T val_a0 = (T(a0) - offset[d]) * scale[d];
                T val_b0 = (T(b0) - offset[d]) * scale[d];
                acc += val_a0 * val_b0;
                
                if(d+1<dim_){
                    T val_a1 = (T(a1) - offset[d+1]) * scale[d+1];
                    T val_b1 = (T(b1) - offset[d+1]) * scale[d+1];
                    acc += val_a1 * val_b1;
                }
            }
            dot_product = float(acc);
	    return finalize_dist(dot_product);
        }
    }

    // =================== THRESHOLDS & SCALING ===============================
    void compute_ternary_thresholds(const std::vector<std::vector<T>>& samples) {
        const size_t n = samples.size();
        std::vector<T> col(n);
        
        for (size_t d = 0; d < dim_; ++d) {
            // Mean-center first
            for (size_t i = 0; i < n; ++i) {
                col[i] = samples[i][d] - mean_[d];
            }
            std::sort(col.begin(), col.end());
            
            // Use 33rd and 67th percentiles
            size_t idx_low = n / 3;
            size_t idx_high = (2 * n) / 3;
            
            ternary_threshold_low_[d] = col[idx_low] + mean_[d];
            ternary_threshold_high_[d] = col[idx_high] + mean_[d];
            
            T range = col[n-1] - col[0];
            ternary_scale_[d] = (range > 0) ? T(1) / (range * range) : T(1);
        }
    }
    
    void compute_ternary_thresholds_from_centroid() {
        for (size_t d = 0; d < dim_; ++d) {
            T c = centroid_[d];
            T offset = std::abs(c - mean_[d]) * T(0.5);
            if (offset == 0) offset = T(0.1);
            
            ternary_threshold_low_[d] = c - offset;
            ternary_threshold_high_[d] = c + offset;
            ternary_scale_[d] = T(1) / (offset * offset);
        }
    }
    
    void compute_scale_offset_from_centroid(const std::vector<std::vector<T>>& samples, double levels) {
        const size_t n = samples.size();
        for (size_t d = 0; d < dim_; ++d) {
            // Mean-center
            std::vector<T> centered(n);
            for (size_t i = 0; i < n; ++i) {
                centered[i] = samples[i][d] - mean_[d];
            }
            
            T lo = *std::min_element(centered.begin(), centered.end());
            T hi = *std::max_element(centered.begin(), centered.end());
            
            // Center quantization around centroid
            T c = centroid_[d] - mean_[d];
            T range = std::max(std::abs(hi - c), std::abs(lo - c)) * T(2);
            
            scale[d] = range / levels;
            if (scale[d] == 0) scale[d] = 1;
            scale_rcp[d] = T(1) / scale[d];
            
            // Offset to center quantization
            offset[d] = levels / T(2);
        }
    }
    
    void compute_scale_offset(const std::vector<std::vector<T>>& samples, double levels) {
        const size_t n = samples.size();
        for (size_t d = 0; d < dim_; ++d) {
            // Mean-center
            T lo = samples[0][d] - mean_[d];
            T hi = lo;
            for (size_t i = 1; i < n; ++i) {
                T v = samples[i][d] - mean_[d];
                if (v < lo) lo = v;
                if (v > hi) hi = v;
            }
            
            scale[d] = (hi - lo) / levels;
            if (scale[d] == 0) scale[d] = 1;
            scale_rcp[d] = T(1) / scale[d];
            
            // Offset to center quantization at 0
            offset[d] = levels / T(2);
        }
        
        if (dim_ > 0) {
            // HNSWDEBUG << "  [Scale debug IP: dim0 range=[" << -offset[0]*scale[0] << "," << (levels-offset[0])*scale[0] << "], scale=" << scale[0] << "]";
        }
    }

    // =================== QUANTIZATION ======================================
    void quantize_ternary_simd(const T* emb, uint8_t* out) const {
        std::memset(out, 0, bytes_per_vector);
        
        size_t bit_idx = 0;
        for (size_t d = 0; d < dim_; ++d) {
            // Mean-center then apply ternary thresholds
            T centered = emb[d] - mean_[d];
            uint8_t code;
            if (centered < (ternary_threshold_low_[d] - mean_[d])) {
                code = 0b00; // -1
            } else if (centered > (ternary_threshold_high_[d] - mean_[d])) {
                code = 0b10; // +1
            } else {
                code = 0b01; // 0
            }
            
            size_t byte_idx = bit_idx / 8;
            size_t bit_off = bit_idx % 8;
            out[byte_idx] |= (code << bit_off);
            
            bit_idx += 2;
        }
    }
    
    void quantize_bin(const T* emb, uint8_t* out) const {
        std::memset(out, 0, bytes_per_vector);
        
        // For binary IP: use sign of (value - mean)
        // Positive values get bit=1, negative get bit=0
        
#if defined(HNSW_SIMD_AVX512)
        size_t i = 0;
        __m512 vzero = _mm512_setzero_ps();
        for (; i + 15 < dim_; i += 16) {
            __m512 v = _mm512_loadu_ps(emb + i);
            __m512 m = _mm512_loadu_ps(mean_.data() + i);
            __m512 centered = _mm512_sub_ps(v, m);
            __mmask16 cmp = _mm512_cmp_ps_mask(centered, vzero, _CMP_GT_OQ);
            uint16_t mask = cmp;
            for (int b = 0; b < 16; ++b) {
                if (mask & (1 << b)) {
                    size_t idx = i + b;
                    out[idx >> 3] |= uint8_t(1u << (idx & 7));
                }
            }
        }
        for (; i < dim_; ++i) {
            if (emb[i] > mean_[i]) {
                out[i >> 3] |= uint8_t(1u << (i & 7));
            }
        }
#elif defined(HNSW_SIMD_AVX2)
        size_t i = 0;
        __m256 vzero = _mm256_setzero_ps();
        for (; i + 7 < dim_; i += 8) {
            __m256 v = _mm256_loadu_ps(emb + i);
            __m256 m = _mm256_loadu_ps(mean_.data() + i);
            __m256 centered = _mm256_sub_ps(v, m);
            __m256 cmp = _mm256_cmp_ps(centered, vzero, _CMP_GT_OQ);
            int mask = _mm256_movemask_ps(cmp);
            for (int b = 0; b < 8; ++b) {
                if (mask & (1 << b)) {
                    size_t idx = i + b;
                    out[idx >> 3] |= uint8_t(1u << (idx & 7));
                }
            }
        }
        for (; i < dim_; ++i) {
            if (emb[i] > mean_[i]) {
                out[i >> 3] |= uint8_t(1u << (i & 7));
            }
        }
#elif defined(HNSW_SIMD_NEON)
        size_t i = 0;
        float32x4_t vzero = vdupq_n_f32(0.0f);
        for (; i + 3 < dim_; i += 4) {
            float32x4_t v = vld1q_f32(emb + i);
            float32x4_t m = vld1q_f32(mean_.data() + i);
            float32x4_t centered = vsubq_f32(v, m);
            uint32x4_t cmp = vcgtq_f32(centered, vzero);
            uint32_t mask[4];
            vst1q_u32(mask, cmp);
            for (int b = 0; b < 4; ++b) {
                if (mask[b]) {
                    size_t idx = i + b;
                    out[idx >> 3] |= uint8_t(1u << (idx & 7));
                }
            }
        }
        for (; i < dim_; ++i) {
            if (emb[i] > mean_[i]) {
                out[i >> 3] |= uint8_t(1u << (i & 7));
            }
        }
#else
        for (size_t i = 0; i < dim_; ++i) {
            if (emb[i] > mean_[i]) {
                out[i >> 3] |= uint8_t(1u << (i & 7));
            }
        }
#endif
    }

    void quantize_int8_simd(const T* emb, uint8_t* out) const {
#if defined(HNSW_SIMD_AVX2)
        size_t i = 0;
        __m256 vzero = _mm256_setzero_ps();
        __m256 v255 = _mm256_set1_ps(255.0f);
        for (; i + 7 < dim_; i += 8) {
            __m256 v = _mm256_loadu_ps(emb + i);
            __m256 vmean = _mm256_loadu_ps(mean_.data() + i);
            __m256 voffset = _mm256_loadu_ps(offset.data() + i);
            __m256 vscale_rcp = _mm256_loadu_ps(scale_rcp.data() + i);
            
            __m256 centered = _mm256_sub_ps(v, vmean);
            __m256 scaled = _mm256_fmadd_ps(centered, vscale_rcp, voffset);
            scaled = _mm256_round_ps(scaled, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
            scaled = _mm256_max_ps(vzero, _mm256_min_ps(v255, scaled));
            
            alignas(32) float tmp[8];
            _mm256_store_ps(tmp, scaled);
            for (int j = 0; j < 8; ++j) {
                out[i + j] = uint8_t(tmp[j]);
            }
        }
        for (; i < dim_; ++i) {
            T centered = emb[i] - mean_[i];
            float v = centered * scale_rcp[i] + offset[i];
            int qi = std::clamp(int(std::nearbyint(v)), 0, 255);
            out[i] = uint8_t(qi);
        }
#elif defined(HNSW_SIMD_NEON)
        size_t i = 0;
        float32x4_t vzero = vdupq_n_f32(0.0f);
        float32x4_t v255 = vdupq_n_f32(255.0f);
        for (; i + 3 < dim_; i += 4) {
            float32x4_t v = vld1q_f32(emb + i);
            float32x4_t vmean = vld1q_f32(mean_.data() + i);
            float32x4_t voffset = vld1q_f32(offset.data() + i);
            float32x4_t vscale_rcp = vld1q_f32(scale_rcp.data() + i);
            
            float32x4_t centered = vsubq_f32(v, vmean);
            float32x4_t scaled = vmlaq_f32(voffset, centered, vscale_rcp);
            int32x4_t rounded = vcvtnq_s32_f32(scaled);
            scaled = vcvtq_f32_s32(rounded);
            scaled = vmaxq_f32(vzero, vminq_f32(v255, scaled));
            
            float tmp[4];
            vst1q_f32(tmp, scaled);
            for (int j = 0; j < 4; ++j) {
                out[i + j] = uint8_t(tmp[j]);
            }
        }
        for (; i < dim_; ++i) {
            T centered = emb[i] - mean_[i];
            float v = centered * scale_rcp[i] + offset[i];
            int qi = std::clamp(int(std::nearbyint(v)), 0, 255);
            out[i] = uint8_t(qi);
        }
#else
        for (size_t i = 0; i < dim_; ++i) {
            T centered = emb[i] - mean_[i];
            float v = centered * scale_rcp[i] + offset[i];
            int qi = std::clamp(int(std::nearbyint(v)), 0, 255);
            out[i] = uint8_t(qi);
        }
#endif
    }

    void quantize_int4_simd(const T* emb, uint8_t* out) const {
#if defined(HNSW_SIMD_AVX2)
        size_t idx = 0;
        size_t d = 0;
        __m256 vzero = _mm256_setzero_ps();
        __m256 v15 = _mm256_set1_ps(15.0f);
        
        for (; d + 15 < dim_; d += 16) {
            __m256 v0 = _mm256_loadu_ps(emb + d);
            __m256 v1 = _mm256_loadu_ps(emb + d + 8);
            __m256 vmean0 = _mm256_loadu_ps(mean_.data() + d);
            __m256 vmean1 = _mm256_loadu_ps(mean_.data() + d + 8);
            __m256 voffset0 = _mm256_loadu_ps(offset.data() + d);
            __m256 voffset1 = _mm256_loadu_ps(offset.data() + d + 8);
            __m256 vscale_rcp0 = _mm256_loadu_ps(scale_rcp.data() + d);
            __m256 vscale_rcp1 = _mm256_loadu_ps(scale_rcp.data() + d + 8);
            
            __m256 centered0 = _mm256_sub_ps(v0, vmean0);
            __m256 centered1 = _mm256_sub_ps(v1, vmean1);
            __m256 scaled0 = _mm256_fmadd_ps(centered0, vscale_rcp0, voffset0);
            __m256 scaled1 = _mm256_fmadd_ps(centered1, vscale_rcp1, voffset1);
            scaled0 = _mm256_round_ps(scaled0, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
            scaled1 = _mm256_round_ps(scaled1, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
            scaled0 = _mm256_max_ps(vzero, _mm256_min_ps(v15, scaled0));
            scaled1 = _mm256_max_ps(vzero, _mm256_min_ps(v15, scaled1));
            
            alignas(32) float tmp0[8], tmp1[8];
            _mm256_store_ps(tmp0, scaled0);
            _mm256_store_ps(tmp1, scaled1);
            
            for (int j = 0; j < 8; ++j) {
                uint8_t q0 = uint8_t(tmp0[j]);
                uint8_t q1 = uint8_t(tmp1[j]);
                out[idx++] = uint8_t((q1 << 4) | (q0 & 0x0F));
            }
        }
        
        for (; d < dim_; d += 2) {
            T centered0 = emb[d] - mean_[d];
            float v0 = centered0 * scale_rcp[d] + offset[d];
            int q0 = std::clamp(int(std::nearbyint(v0)), 0, 15);
            int q1 = 0;
            if (d + 1 < dim_) {
                T centered1 = emb[d + 1] - mean_[d + 1];
                float v1 = centered1 * scale_rcp[d + 1] + offset[d + 1];
                q1 = std::clamp(int(std::nearbyint(v1)), 0, 15);
            }
            out[idx++] = uint8_t((q1 << 4) | (q0 & 0x0F));
        }
#elif defined(HNSW_SIMD_NEON)
        size_t idx = 0;
        size_t d = 0;
        float32x4_t vzero = vdupq_n_f32(0.0f);
        float32x4_t v15 = vdupq_n_f32(15.0f);
        
        for (; d + 7 < dim_; d += 8) {
            float32x4_t v0 = vld1q_f32(emb + d);
            float32x4_t v1 = vld1q_f32(emb + d + 4);
            float32x4_t vmean0 = vld1q_f32(mean_.data() + d);
            float32x4_t vmean1 = vld1q_f32(mean_.data() + d + 4);
            float32x4_t voffset0 = vld1q_f32(offset.data() + d);
            float32x4_t voffset1 = vld1q_f32(offset.data() + d + 4);
            float32x4_t vscale_rcp0 = vld1q_f32(scale_rcp.data() + d);
            float32x4_t vscale_rcp1 = vld1q_f32(scale_rcp.data() + d + 4);
            
            float32x4_t centered0 = vsubq_f32(v0, vmean0);
            float32x4_t centered1 = vsubq_f32(v1, vmean1);
            float32x4_t scaled0 = vmlaq_f32(voffset0, centered0, vscale_rcp0);
            float32x4_t scaled1 = vmlaq_f32(voffset1, centered1, vscale_rcp1);
            int32x4_t rounded0 = vcvtnq_s32_f32(scaled0);
            int32x4_t rounded1 = vcvtnq_s32_f32(scaled1);
            scaled0 = vcvtq_f32_s32(rounded0);
            scaled1 = vcvtq_f32_s32(rounded1);
            scaled0 = vmaxq_f32(vzero, vminq_f32(v15, scaled0));
            scaled1 = vmaxq_f32(vzero, vminq_f32(v15, scaled1));
            
            float tmp0[4], tmp1[4];
            vst1q_f32(tmp0, scaled0);
            vst1q_f32(tmp1, scaled1);
            
            for (int j = 0; j < 4; ++j) {
                uint8_t q0 = uint8_t(tmp0[j]);
                uint8_t q1 = uint8_t(tmp1[j]);
                out[idx++] = uint8_t((q1 << 4) | (q0 & 0x0F));
            }
        }
        
        for (; d < dim_; d += 2) {
            T centered0 = emb[d] - mean_[d];
            float v0 = centered0 * scale_rcp[d] + offset[d];
            int q0 = std::clamp(int(std::nearbyint(v0)), 0, 15);
            int q1 = 0;
            if (d + 1 < dim_) {
                T centered1 = emb[d + 1] - mean_[d + 1];
                float v1 = centered1 * scale_rcp[d + 1] + offset[d + 1];
                q1 = std::clamp(int(std::nearbyint(v1)), 0, 15);
            }
            out[idx++] = uint8_t((q1 << 4) | (q0 & 0x0F));
        }
#else
        size_t idx = 0;
        for (size_t d = 0; d < dim_; d += 2) {
            T centered0 = emb[d] - mean_[d];
            float v0 = centered0 * scale_rcp[d] + offset[d];
            int q0 = std::clamp(int(std::nearbyint(v0)), 0, 15);
            int q1 = 0;
            if (d + 1 < dim_) {
                T centered1 = emb[d + 1] - mean_[d + 1];
                float v1 = centered1 * scale_rcp[d + 1] + offset[d + 1];
                q1 = std::clamp(int(std::nearbyint(v1)), 0, 15);
            }
            out[idx++] = uint8_t((q1 << 4) | (q0 & 0x0F));
        }
#endif
    }
    
    // =================== CENTROID HELPERS ==================================
    void update_centroid() {
        if (count_ == 0) return;
        for (size_t d = 0; d < dim_; ++d) {
            centroid_[d] = sum_[d] / T(count_);
        }
    }
    
    void update_quantization_params(const std::vector<T>& buffer_samples, size_t n_samples) {
        if (qmode_ == QuantModeIP::BIN1) {
            // Binary quantization uses mean, already updated
        } else if (qmode_ == QuantModeIP::INT158) {
            compute_ternary_thresholds_from_centroid();
        } else if (qmode_ == QuantModeIP::INT4 || qmode_ == QuantModeIP::INT8) {
            double levels = (qmode_ == QuantModeIP::INT8) ? 255.0 : 15.0;
            
            for (size_t d = 0; d < dim_; ++d) {
                T lo = buffer_samples[d] - mean_[d];
                T hi = lo;
                
                for (size_t i = 1; i < n_samples; ++i) {
                    T v = buffer_samples[i * dim_ + d] - mean_[d];
                    if (v < lo) lo = v;
                    if (v > hi) hi = v;
                }
                
                T c = centroid_[d] - mean_[d];
                T range = std::max(std::abs(hi - c), std::abs(lo - c)) * T(2);
                
                T alpha = T(0.3);
                T new_scale = range / levels;
                if (new_scale == 0) new_scale = 1;
                
                scale[d] = alpha * new_scale + (T(1) - alpha) * scale[d];
                scale_rcp[d] = T(1) / scale[d];
            }
        }
    }

// Persist Params
bool save_quantization_params(const std::string& filepath) const {
    std::ofstream out(filepath, std::ios::binary);
    return save_quantization_params(out);
}

bool save_quantization_params(std::ofstream &out) const override {
    if (!out.is_open()) return false;
    
    // Write header
    out.write(reinterpret_cast<const char*>(&dim_), sizeof(dim_));
    int qmode_int = static_cast<int>(qmode_);
    int bin_mode_int = static_cast<int>(bin_mode_);
    out.write(reinterpret_cast<const char*>(&qmode_int), sizeof(qmode_int));
    out.write(reinterpret_cast<const char*>(&bin_mode_int), sizeof(bin_mode_int));
    out.write(reinterpret_cast<const char*>(&bytes_per_vector), sizeof(bytes_per_vector));
    out.write(reinterpret_cast<const char*>(&use_rotation_), sizeof(use_rotation_));
    out.write(reinterpret_cast<const char*>(&use_rabitq_), sizeof(use_rabitq_));
    out.write(reinterpret_cast<const char*>(&residual_dims_), sizeof(residual_dims_));
    out.write(reinterpret_cast<const char*>(&use_normalization_), sizeof(use_normalization_));
    
    // Write mean (always needed for IP)
    out.write(reinterpret_cast<const char*>(mean_.data()), dim_ * sizeof(T));
    
    // Write quantization parameters based on mode
    if (qmode_ == QuantModeIP::INT158) {
        out.write(reinterpret_cast<const char*>(ternary_threshold_low_.data()), dim_ * sizeof(T));
        out.write(reinterpret_cast<const char*>(ternary_threshold_high_.data()), dim_ * sizeof(T));
        out.write(reinterpret_cast<const char*>(ternary_scale_.data()), dim_ * sizeof(T));
    } else if (qmode_ == QuantModeIP::INT4 || qmode_ == QuantModeIP::INT8) {
        out.write(reinterpret_cast<const char*>(scale.data()), dim_ * sizeof(T));
        out.write(reinterpret_cast<const char*>(offset.data()), dim_ * sizeof(T));
        out.write(reinterpret_cast<const char*>(scale_rcp.data()), dim_ * sizeof(T));
    }
    
    // Write rotation matrix if used
    if (use_rotation_) {
        out.write(reinterpret_cast<const char*>(rotation_matrix_.data()), 
                  dim_ * dim_ * sizeof(T));
    }
    
    // Write centroid if used
    if (bin_mode_ == OptBinModeIP::CENTROID || bin_mode_ == OptBinModeIP::RABITQ) {
        out.write(reinterpret_cast<const char*>(centroid_.data()), dim_ * sizeof(T));
        out.write(reinterpret_cast<const char*>(sum_.data()), dim_ * sizeof(T));
        out.write(reinterpret_cast<const char*>(&count_), sizeof(count_));
    }
    
    return out.good();
}

bool load_quantization_params(const std::string& filepath) {
    std::ifstream in(filepath, std::ios::binary);
    return load_quantization_params(in);
}

bool load_quantization_params(std::ifstream &in) override {
    if (!in.is_open()) return false;
    
    // Read and verify header
    size_t file_dim;
    int qmode_int, bin_mode_int;
    in.read(reinterpret_cast<char*>(&file_dim), sizeof(file_dim));
    if (file_dim != dim_) {
        HNSWERR << "Dimension mismatch: expected " << dim_ << ", got " << file_dim;
        return false;
    }
    
    in.read(reinterpret_cast<char*>(&qmode_int), sizeof(qmode_int));
    in.read(reinterpret_cast<char*>(&bin_mode_int), sizeof(bin_mode_int));
    
    if (static_cast<QuantModeIP>(qmode_int) != qmode_) {
        HNSWERR << "QuantModeIP mismatch";
        return false;
    }
    if (static_cast<OptBinModeIP>(bin_mode_int) != bin_mode_) {
        HNSWERR << "OptBinModeIP mismatch";
        return false;
    }
    
    in.read(reinterpret_cast<char*>(&bytes_per_vector), sizeof(bytes_per_vector));
    in.read(reinterpret_cast<char*>(&use_rotation_), sizeof(use_rotation_));
    in.read(reinterpret_cast<char*>(&use_rabitq_), sizeof(use_rabitq_));
    in.read(reinterpret_cast<char*>(&residual_dims_), sizeof(residual_dims_));
    in.read(reinterpret_cast<char*>(&use_normalization_), sizeof(use_normalization_));
    
    // Read mean
    mean_.resize(dim_);
    in.read(reinterpret_cast<char*>(mean_.data()), dim_ * sizeof(T));
    
    // Read quantization parameters based on mode
    if (qmode_ == QuantModeIP::INT158) {
        ternary_threshold_low_.resize(dim_);
        ternary_threshold_high_.resize(dim_);
        ternary_scale_.resize(dim_);
        in.read(reinterpret_cast<char*>(ternary_threshold_low_.data()), dim_ * sizeof(T));
        in.read(reinterpret_cast<char*>(ternary_threshold_high_.data()), dim_ * sizeof(T));
        in.read(reinterpret_cast<char*>(ternary_scale_.data()), dim_ * sizeof(T));
    } else if (qmode_ == QuantModeIP::INT4 || qmode_ == QuantModeIP::INT8) {
        scale.resize(dim_);
        offset.resize(dim_);
        scale_rcp.resize(dim_);
        in.read(reinterpret_cast<char*>(scale.data()), dim_ * sizeof(T));
        in.read(reinterpret_cast<char*>(offset.data()), dim_ * sizeof(T));
        in.read(reinterpret_cast<char*>(scale_rcp.data()), dim_ * sizeof(T));
    }
    
    // Read rotation matrix if used
    if (use_rotation_) {
        rotation_matrix_.resize(dim_ * dim_);
        in.read(reinterpret_cast<char*>(rotation_matrix_.data()), 
                dim_ * dim_ * sizeof(T));
    }
    
    // Read centroid if used
    if (bin_mode_ == OptBinModeIP::CENTROID || bin_mode_ == OptBinModeIP::RABITQ) {
        centroid_.resize(dim_);
        sum_.resize(dim_);
        in.read(reinterpret_cast<char*>(centroid_.data()), dim_ * sizeof(T));
        in.read(reinterpret_cast<char*>(sum_.data()), dim_ * sizeof(T));
        in.read(reinterpret_cast<char*>(&count_), sizeof(count_));
    }
    
    return in.good();
}

// Getters
const std::vector<T>& get_mean() const { return mean_; }
const std::vector<T>& get_scale() const { return scale; }
const std::vector<T>& get_offset() const { return offset; }
const std::vector<T>& get_ternary_threshold_low() const { return ternary_threshold_low_; }
const std::vector<T>& get_ternary_threshold_high() const { return ternary_threshold_high_; }
const std::vector<T>& get_rotation_matrix() const { return rotation_matrix_; }
bool is_using_rotation() const { return use_rotation_; }
bool is_using_rabitq() const { return use_rabitq_; }
size_t get_residual_dims() const { return residual_dims_; }


};

} // namespace hnswlib

/*
Usage example for Inner Product:

// Create space with binary quantization for IP
auto space = new hnswlib::SpaceQuantizedIP<float>(
    dim, 
    hnswlib::QuantModeIP::BIN1, 
    hnswlib::OptBinModeIP::CENTROID,
    &initial_samples,
    1000,  // buffer capacity
    false  // use_normalization (set true if vectors are normalized)
);

// Create HNSW index
auto index = new hnswlib::HierarchicalNSW<float>(space, max_elements);

// Add points
for (size_t i = 0; i < num_vectors; ++i) {
    space->add_to_centroid(vectors[i].data());
    
    std::vector<uint8_t> code(space->get_data_size());
    space->quantize(vectors[i].data(), code.data());
    index->addPoint(code.data(), i);
}

// Flush and save
space->flush_centroid_buffer();
space->save_centroid("centroid_ip.bin");
index->saveIndex("index_ip.bin");
*/
