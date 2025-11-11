#define DELAY_ALLOC  1

#include "unified_hnsw.hpp"
#include <hnswlib/cosine_similarity.h>
#include <hnswlib/l2_distance.h>
#include "pearson_corr.hpp"

#ifdef __ARM_FEATURE_SVE
# include "arm_sve_suport.hpp"
#endif

#define UNIFIED_INDEX_ 0 /* Set to 1 when we activate the code */


namespace hnswlib {

// ============================================================================
// Utility Functions
// ============================================================================
SimdKind detect_simd() {
#ifdef __AVX512F__
    if (AVX512Capable()) return SimdKind::AVX512;
#endif
#if defined(__AVX2__)
    if (AVXCapable()) return SimdKind::AVX2;
#endif
#if defined(__ARM_FEATURE_SVE)
    if (has_sve_runtime()) return SimdKind::SVE;
#endif
#if defined(__ARM_NEON)
    return SimdKind::NEON;
#endif
    return SimdKind::NONE;
}


#if 0 /* OFFLOADED */
float pearson_corr(const std::vector<float>& a, const std::vector<float>& b) {
    assert(a.size() == b.size());
    float mean_a = std::accumulate(a.begin(), a.end(), 0.f) / a.size();
    float mean_b = std::accumulate(b.begin(), b.end(), 0.f) / b.size();
    float num = 0.f, da2 = 0.f, db2 = 0.f;
    for (size_t i = 0; i < a.size(); i++) {
        float da = a[i] - mean_a, db = b[i] - mean_b;
        num += da * db;
        da2 += da * da;
        db2 += db * db;
    }
    return num / std::sqrt(da2 * db2 + 1e-8f);
}
#endif

// ============================================================================
// Binarization Functions
// ============================================================================

void binarize_scalar(const float* emb, const float* thr, size_t dim, uint8_t* out) {
    size_t nbytes = (dim + 7) / 8;
    std::memset(out, 0, nbytes);
    for (size_t d = 0; d < dim; d++)
        if (emb[d] > thr[d]) 
            out[d >> 3] |= 1u << (d & 7);
}

#ifdef __AVX2__
void binarize_avx2(const float* emb, const float* thr, size_t dim, uint8_t* out) {
    size_t nbytes = (dim + 7) / 8;
    std::memset(out, 0, nbytes);
    size_t d = 0;
    for (; d + 8 <= dim; d += 8) {
        __m256 v = _mm256_loadu_ps(emb + d);
        __m256 t = _mm256_loadu_ps(thr + d);
        __m256 cmp = _mm256_cmp_ps(v, t, _CMP_GT_OS);
        out[d >> 3] = static_cast<uint8_t>(_mm256_movemask_ps(cmp) & 0xFFu);
    }
    for (; d < dim; d++)
        if (emb[d] > thr[d]) 
            out[d >> 3] |= 1u << (d & 7);
}
#endif

#ifdef __AVX512F__
void binarize_avx512(const float* emb, const float* thr, size_t dim, uint8_t* out) {
    size_t nbytes = (dim + 7) / 8;
    std::memset(out, 0, nbytes);
    size_t d = 0;
    for (; d + 16 <= dim; d += 16) {
        __m512 v = _mm512_loadu_ps(emb + d);
        __m512 t = _mm512_loadu_ps(thr + d);
        __mmask16 m = _mm512_cmp_ps_mask(v, t, _CMP_GT_OS);
        out[d >> 3] = uint8_t(m & 0xFFu);
        out[(d >> 3) + 1] = uint8_t((m >> 8) & 0xFFu);
    }
#ifdef __AVX2__
    for (; d + 8 <= dim; d += 8) {
        __m256 v = _mm256_loadu_ps(emb + d);
        __m256 t = _mm256_loadu_ps(thr + d);
        __m256 cmp = _mm256_cmp_ps(v, t, _CMP_GT_OS);
        out[d >> 3] = uint8_t(_mm256_movemask_ps(cmp) & 0xFFu);
    }
#endif
    for (; d < dim; d++)
        if (emb[d] > thr[d]) 
            out[d >> 3] |= 1u << (d & 7);
}
#endif

#ifdef __ARM_NEON
void binarize_neon(const float* emb, const float* thr, size_t dim, uint8_t* out) {
    size_t nbytes = (dim + 7) / 8;
    std::memset(out, 0, nbytes);
    size_t d = 0;
    for (; d + 8 <= dim; d += 8) {
        float32x4_t v0 = vld1q_f32(emb + d), t0 = vld1q_f32(thr + d);
        uint32x4_t m0 = vcgtq_f32(v0, t0);
        float32x4_t v1 = vld1q_f32(emb + d + 4), t1 = vld1q_f32(thr + d + 4);
        uint32x4_t m1 = vcgtq_f32(v1, t1);
        uint8_t byte = 0;
        byte |= (vgetq_lane_u32(m0, 0) ? 1u << 0 : 0);
        byte |= (vgetq_lane_u32(m0, 1) ? 1u << 1 : 0);
        byte |= (vgetq_lane_u32(m0, 2) ? 1u << 2 : 0);
        byte |= (vgetq_lane_u32(m0, 3) ? 1u << 3 : 0);
        byte |= (vgetq_lane_u32(m1, 0) ? 1u << 4 : 0);
        byte |= (vgetq_lane_u32(m1, 1) ? 1u << 5 : 0);
        byte |= (vgetq_lane_u32(m1, 2) ? 1u << 6 : 0);
        byte |= (vgetq_lane_u32(m1, 3) ? 1u << 7 : 0);
        out[d >> 3] = byte;
    }
    for (; d < dim; d++)
        if (emb[d] > thr[d]) 
            out[d >> 3] |= 1u << (d & 7);
}
#endif

// ============================================================================
// BinaryQuantizer Implementation
// ============================================================================

BinaryQuantizer::BinaryQuantizer(size_t dim, BinMode mode, bool normalize_before_quantization) 
    : dim_(dim), thresholds_(dim, 0.0f), simd_(detect_simd()), mode_(mode),
      normalize_before_quantization_(normalize_before_quantization) {}

void BinaryQuantizer::fit(const std::vector<std::vector<float>>& sample_embeddings) {
    size_t n = sample_embeddings.size();
    if (n == 0) return;

    std::vector<std::vector<float>> embeddings = sample_embeddings;
    if (normalize_before_quantization_) {
        normalize_l2_batch(embeddings);
    }

    if (mode_ == BinMode::STANDARD) {
        for (size_t d = 0; d < dim_; d++) {
            std::vector<float> col(n);
            for (size_t i = 0; i < n; i++) 
                col[i] = embeddings[i][d];
            std::nth_element(col.begin(), col.begin() + n / 2, col.end());
            thresholds_[d] = col[n / 2];
        }
    } else if (mode_ == BinMode::BETTER) {
        int nCandidates = 50;
        for (size_t d = 0; d < dim_; d++) {
            std::vector<float> col(n);
            for (size_t i = 0; i < n; i++) 
                col[i] = embeddings[i][d];
            std::sort(col.begin(), col.end());
            
            float best_thr = col[0];
            float best_corr = -1.f;
            
            for (int k = 0; k < nCandidates; k++) {
                float q = 0.1f + 0.8f * float(k) / (nCandidates - 1);
                size_t id_q = std::min<size_t>(size_t(q * (n - 1)), n - 1);
                float thr = col[id_q];
                
                std::vector<float> bits(n);
                for (size_t i = 0; i < n; i++) 
                    bits[i] = col[i] > thr ? 1.f : 0.f;
                
                float corr = std::fabs(simd_pearson::pearson_corr(bits, col));
                if (corr > best_corr) {
                    best_corr = corr;
                    best_thr = thr;
                }
            }
            thresholds_[d] = best_thr;
        }
    }
}

void BinaryQuantizer::quantize(const float* emb, uint8_t* out) const {
    if (normalize_before_quantization_) {
        std::vector<float> normalized(emb, emb + dim_);
        normalize_l2(normalized.data(), dim_);
        quantize_internal(normalized.data(), out);
    } else {
        quantize_internal(emb, out);
    }
}

void BinaryQuantizer::quantize_internal(const float* emb, uint8_t* out) const {
    switch (simd_) {
#ifdef __AVX512F__
        case SimdKind::AVX512:
            binarize_avx512(emb, thresholds_.data(), dim_, out);
            return;
#endif
#ifdef __AVX2__
        case SimdKind::AVX2:
            binarize_avx2(emb, thresholds_.data(), dim_, out);
            return;
#endif
#ifdef __ARM_FEATURE_SVE
        case SimdKind::SVE:
            binarize_sve_optimized(emb, thresholds_.data(), dim_, out);
            return;
#endif
#ifdef __ARM_NEON
        case SimdKind::NEON:
            binarize_neon(emb, thresholds_.data(), dim_, out);
            return;
#endif
        default:
            binarize_scalar(emb, thresholds_.data(), dim_, out);
            return;
    }
}







void BinaryQuantizer::set_thresholds(const std::vector<float>& thresholds) {
    if (thresholds.size() == dim_) {
        thresholds_ = thresholds;
    }
}

// ============================================================================
// TernaryQuantizer Implementation
// ============================================================================

TernaryQuantizer::TernaryQuantizer(size_t dim, bool normalize_before_quantization)
    : dim_(dim), thresholds_pos_(dim, 0.0f), thresholds_neg_(dim, 0.0f),
      normalize_before_quantization_(normalize_before_quantization) {}

void TernaryQuantizer::fit(const std::vector<std::vector<float>>& sample_embeddings) {
    size_t n = sample_embeddings.size();
    if (n == 0) return;

    std::vector<std::vector<float>> embeddings = sample_embeddings;
    if (normalize_before_quantization_) {
        normalize_l2_batch(embeddings);
    }

    for (size_t d = 0; d < dim_; d++) {
        std::vector<float> col(n);
        for (size_t i = 0; i < n; i++)
            col[i] = embeddings[i][d];
        std::sort(col.begin(), col.end());
        
        size_t idx_neg = n / 3;
        size_t idx_pos = (2 * n) / 3;
        thresholds_neg_[d] = col[idx_neg];
        thresholds_pos_[d] = col[idx_pos];
    }
}

void TernaryQuantizer::quantize(const float* emb, uint8_t* out) const {
    std::vector<float> data(emb, emb + dim_);
    if (normalize_before_quantization_) {
        normalize_l2(data.data(), dim_);
    }
    
    size_t nbytes = (dim_ * 2 + 7) / 8;
    std::memset(out, 0, nbytes);
    
    for (size_t d = 0; d < dim_; d++) {
        uint8_t val = (data[d] > thresholds_pos_[d]) ? 2 : 
                     ((data[d] < thresholds_neg_[d]) ? 0 : 1);
        size_t bit_pos = d * 2;
        size_t byte_idx = bit_pos / 8;
        size_t bit_offset = bit_pos % 8;
        out[byte_idx] |= (val << bit_offset);
        if (bit_offset == 7) {
            out[byte_idx + 1] |= (val >> 1);
        }
    }
}

void TernaryQuantizer::set_thresholds(const std::vector<float>& pos, const std::vector<float>& neg) {
    if (pos.size() == dim_ && neg.size() == dim_) {
        thresholds_pos_ = pos;
        thresholds_neg_ = neg;
    }
}

// ============================================================================
// HammingDistance Implementation
// ============================================================================

uint32_t HammingDistance::compute(const uint8_t* a, const uint8_t* b, size_t nbytes) {
    uint32_t dist = 0;
    for (size_t i = 0; i < nbytes; i++) {
        dist += __builtin_popcount(a[i] ^ b[i]);
    }
    return dist;
}

#ifdef __ARM_NEON
uint32_t HammingDistance::compute_neon(const uint8_t* a, const uint8_t* b, size_t nbytes) {
    uint32_t dist = 0;
    size_t i = 0;
    
    for (; i + 16 <= nbytes; i += 16) {
        uint8x16_t va = vld1q_u8(a + i);
        uint8x16_t vb = vld1q_u8(b + i);
        uint8x16_t vxor = veorq_u8(va, vb);
        uint8x16_t vcnt = vcntq_u8(vxor);
        dist += vaddvq_u8(vcnt);
    }
    
    for (; i < nbytes; i++) {
        dist += __builtin_popcount(a[i] ^ b[i]);
    }
    
    return dist;
}
#endif

#ifdef __AVX2__
uint32_t HammingDistance::compute_avx2(const uint8_t* a, const uint8_t* b, size_t nbytes) {
    uint32_t dist = 0;
    size_t i = 0;
    
    for (; i + 32 <= nbytes; i += 32) {
        __m256i va = _mm256_loadu_si256((__m256i*)(a + i));
        __m256i vb = _mm256_loadu_si256((__m256i*)(b + i));
        __m256i vxor = _mm256_xor_si256(va, vb);
        
        __m256i low_mask = _mm256_set1_epi8(0x0f);
        __m256i lo = _mm256_and_si256(vxor, low_mask);
        __m256i hi = _mm256_and_si256(_mm256_srli_epi16(vxor, 4), low_mask);
        __m256i popcnt_lut = _mm256_setr_epi8(
            0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
            0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4);
        __m256i cnt_lo = _mm256_shuffle_epi8(popcnt_lut, lo);
        __m256i cnt_hi = _mm256_shuffle_epi8(popcnt_lut, hi);
        __m256i cnt = _mm256_add_epi8(cnt_lo, cnt_hi);
        
        __m256i sum = _mm256_sad_epu8(cnt, _mm256_setzero_si256());
        dist += _mm256_extract_epi64(sum, 0) + _mm256_extract_epi64(sum, 1) +
                _mm256_extract_epi64(sum, 2) + _mm256_extract_epi64(sum, 3);
    }
    
    for (; i < nbytes; i++) {
        dist += __builtin_popcount(a[i] ^ b[i]);
    }
    
    return dist;
}
#endif

uint32_t HammingDistance::compute_optimized(const uint8_t* a, const uint8_t* b, size_t nbytes) {
#ifdef __AVX2__
    return compute_avx2(a, b, nbytes);
#elif defined(__ARM_FEATURE_SVE)
    return hamming_distance_sve_16bit(a, b, nbytes);
#elif defined(__ARM_NEON)
    return compute_neon(a, b, nbytes);
#else
    return compute(a, b, nbytes);
#endif
}


// ============================================================================
// TernaryDistance Implementation
// ============================================================================

uint32_t TernaryDistance::compute(const uint8_t* a, const uint8_t* b, size_t dim) {
    uint32_t dist = 0;
    
    for (size_t d = 0; d < dim; d++) {
        size_t bit_pos = d * 2;
        size_t byte_idx = bit_pos / 8;
        size_t bit_offset = bit_pos % 8;
        
        uint8_t val_a = (a[byte_idx] >> bit_offset) & 0x03;
        if (bit_offset == 7) {
            val_a |= (a[byte_idx + 1] & 0x01) << 1;
        }
        
        uint8_t val_b = (b[byte_idx] >> bit_offset) & 0x03;
        if (bit_offset == 7) {
            val_b |= (b[byte_idx + 1] & 0x01) << 1;
        }
        
        if (val_a != val_b) dist++;
    }
    
    return dist;
}

// ============================================================================
// BinarySpace Implementation
// ============================================================================

BinarySpace::BinarySpace(size_t dim) : dim_(dim), nbytes_((dim + 7) / 8) {
    fstdistfunc_ = [](const void* pVect1, const void* pVect2, const void* qty_ptr) -> size_t {
        size_t nbytes = *((size_t*)qty_ptr);
        const uint8_t* a = (const uint8_t*)pVect1;
        const uint8_t* b = (const uint8_t*)pVect2;
        return HammingDistance::compute_optimized(a, b, nbytes);
    };
    
    dist_func_param_ = &nbytes_;
}

size_t BinarySpace::get_data_size() {
    return nbytes_;
}

DISTFUNC<size_t> BinarySpace::get_dist_func() {
    return fstdistfunc_;
}

void* BinarySpace::get_dist_func_param() {
    return dist_func_param_;
}

// ============================================================================
// TernarySpace Implementation
// ============================================================================

TernarySpace::TernarySpace(size_t dim) : dim_(dim), nbytes_((dim * 2 + 7) / 8) {
    fstdistfunc_ = [](const void* pVect1, const void* pVect2, const void* qty_ptr) -> size_t {
        size_t dim = *((size_t*)qty_ptr);
        const uint8_t* a = (const uint8_t*)pVect1;
        const uint8_t* b = (const uint8_t*)pVect2;
        return TernaryDistance::compute(a, b, dim);
    };
    
    dist_func_param_ = &dim_;
}

size_t TernarySpace::get_data_size() {
    return nbytes_;
}

DISTFUNC<size_t> TernarySpace::get_dist_func() {
    return fstdistfunc_;
}

void* TernarySpace::get_dist_func_param() {
    return dist_func_param_;
}

// ============================================================================
// UnifiedIndex Implementation - Helper Functions
// ============================================================================

void UnifiedIndex::create_float_space() {
    if (dim_ == 0) {
       throw std::runtime_error("Zero (0) dimension float vector space specified!!!!");
       return; // This is evil
    }
    switch (metric_) {
        case Metric::L1:
           float_space_ = std::make_unique<hnswlib::L1Space>(dim_);
           break;
        case Metric::L2:
            float_space_ = std::make_unique<L2Space>(dim_);
            break;
        case Metric::IP:
            float_space_ = std::make_unique<InnerProductSpace>(dim_);
            break;
        case Metric::Cosine:
            float_space_ = std::make_unique<InnerProductSpace>(dim_);
            normalize_ = true;
            break;
    }
}

void UnifiedIndex::create_quantized_space() {
    if (dim_ == 0) {
       throw std::runtime_error("Zero (0) dimension quant vector space specified!!!!");
       return; // This is evil
    } 
    if (quantization_ == QuantizationType::BINARY) {
        quant_space_ = std::make_unique<BinarySpace>(dim_);
        binary_quantizer_ = std::make_unique<BinaryQuantizer>(dim_, bin_mode_, normalize_);
    } else if (quantization_ == QuantizationType::TERNARY) {
        quant_space_ = std::make_unique<TernarySpace>(dim_);
        ternary_quantizer_ = std::make_unique<TernaryQuantizer>(dim_, normalize_);
    }
}

void UnifiedIndex::create_index() {
    if (is_quantized()) {
        if (!quant_space_) create_quantized_space();
        quant_index_ = std::make_unique<HierarchicalNSW<size_t>>(
            quant_space_.get(), max_elements_, M_, ef_construction_);
    } else {
        if (!float_space_) create_float_space();
        float_index_ = std::make_unique<HierarchicalNSW<float>>(
            float_space_.get(), max_elements_, M_, ef_construction_);
    }
}


/*

// These are now part of our extended HNSWlib
float UnifiedIndex::cosine_similarity(const float* a, const float* b, size_t dim) {
    float dot = 0.0f, norm_a = 0.0f, norm_b = 0.0f;
    for (size_t i = 0; i < dim; i++) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }
    return dot / (std::sqrt(norm_a * norm_b) + 1e-8f);
}

float UnifiedIndex::l2_distance(const float* a, const float* b, size_t dim) {
    float dist = 0.0f;
    for (size_t i = 0; i < dim; i++) {
        float diff = a[i] - b[i];
        dist += diff * diff;
    }
    return std::sqrt(dist);
}
*/



// ============================================================================
// UnifiedIndex Implementation - Constructor
// ============================================================================


#if DELAY_ALLOC == 1
UnifiedIndex::UnifiedIndex(const UnifiedIndexMeta& meta) : meta_(meta) {
  normalize_ =  (metric_ == Metric::Cosine);
}
#endif

UnifiedIndex::UnifiedIndex(size_t dim, size_t max_elements, Metric metric,
    QuantizationType quantization, BinMode bin_mode, bool enable_rescoring,
    size_t M, size_t ef_construction, size_t flush_threshold) {
    
    dim_ = dim;
    max_elements_ = max_elements;
    metric_ = metric;
    quantization_ = quantization;
    bin_mode_ = bin_mode;
    enable_rescoring_ = enable_rescoring;
    M_ = M;
    ef_construction_ = ef_construction;
    ef_ = 10;

    flush_threshold_ = flush_threshold;
 
    normalize_ = (metric == Metric::Cosine);

#if DELAY_ALLOC == 0
    if (quantization == QuantizationType::NONE) {
        create_float_space();
        float_index_ = std::make_unique<HierarchicalNSW<float>>(
            float_space_.get(), max_elements, M, ef_construction);
    } else {
        create_quantized_space();
        quant_index_ = std::make_unique<HierarchicalNSW<size_t>>(
            quant_space_.get(), max_elements, M, ef_construction);
    }
#endif
}

// ============================================================================
// UnifiedIndex Implementation - Public Methods
// ============================================================================

void UnifiedIndex::fit_quantizer(const std::vector<std::vector<float>>& sample_embeddings) {
    if (quantization_ == QuantizationType::BINARY && binary_quantizer_) {
        binary_quantizer_->fit(sample_embeddings);
        quantizer_fitted_ = true;
    } else if (quantization_ == QuantizationType::TERNARY && ternary_quantizer_) {
        ternary_quantizer_->fit(sample_embeddings);
        quantizer_fitted_ = true;
    }
}


// NOTE: We now normalize here. When folded in we must
// remove the normalization for encode_text.
// TODO!!!!
void UnifiedIndex::addPoint(const float* data, labeltype label) {
    if (normalize_) {
        std::vector<float> normalized(data, data + dim_);
        normalize_l2(normalized.data(), dim_);
        addPoint_internal(normalized.data(), label);
    } else {
        addPoint_internal(data, label);
    }
}

void UnifiedIndex::addPoint_internal(const float* data, labeltype label) {
#if DELAY_ALLOC == 1
    // Make sure we have an index, create if needed
    if (quantization_ == QuantizationType::NONE) {
        if (!float_space_) create_float_space();
        if (!float_index_) {
            float_index_ = std::make_unique<HierarchicalNSW<float>>(
                float_space_.get(), max_elements_, M_, ef_construction_);
        }
    } else {
        if (!quant_space_) create_quantized_space();
        if (!quant_index_) {
            quant_index_ = std::make_unique<HierarchicalNSW<size_t>>(
                quant_space_.get(), max_elements_, M_, ef_construction_);
        }
    }           
#endif 
    if (quantization_ == QuantizationType::NONE) {
        float_index_->addPoint(data, label);
    } else if (quantization_ == QuantizationType::BINARY) {
        std::vector<uint8_t> binary(binary_quantizer_->get_nbytes());
        binary_quantizer_->quantize(data, binary.data());
        quant_index_->addPoint(binary.data(), label);
    } else if (quantization_ == QuantizationType::TERNARY) {
        std::vector<uint8_t> ternary(ternary_quantizer_->get_nbytes());
        ternary_quantizer_->quantize(data, ternary.data());
        quant_index_->addPoint(ternary.data(), label);
    }

    if (enable_rescoring_) {
        original_vectors_[label] = std::vector<float>(data, data + dim_);
    }
}

std::priority_queue<std::pair<float, labeltype>> UnifiedIndex::searchKnn(
    const float* query, size_t k, bool use_rescoring) {
    
    if (normalize_) {
        std::vector<float> normalized(query, query + dim_);
        normalize_l2(normalized.data(), dim_);
        return searchKnn_internal(normalized.data(), k, use_rescoring);
    } else {
        return searchKnn_internal(query, k, use_rescoring);
    }
}

std::priority_queue<std::pair<float, labeltype>> UnifiedIndex::searchKnn_internal(
    const float* query, size_t k, bool use_rescoring) {
    
    if (quantization_ == QuantizationType::NONE) {
        return float_index_->searchKnn(query, k);
    }
    
    std::vector<uint8_t> quantized;
    if (quantization_ == QuantizationType::BINARY) {
        quantized.resize(binary_quantizer_->get_nbytes());
        binary_quantizer_->quantize(query, quantized.data());
    } else if (quantization_ == QuantizationType::TERNARY) {
        quantized.resize(ternary_quantizer_->get_nbytes());
        ternary_quantizer_->quantize(query, quantized.data());
    }
    
    if (!use_rescoring || !enable_rescoring_) {
        auto results = quant_index_->searchKnn(quantized.data(), k);
        std::priority_queue<std::pair<float, labeltype>> converted;
        while (!results.empty()) {
            auto [dist, label] = results.top();
            results.pop();
            converted.emplace(static_cast<float>(dist), label);
        }
        return converted;
    }
    
    size_t rescore_factor = std::max(size_t(3), k * 3);
    size_t num_candidates = std::min(rescore_factor, quant_index_->getCurrentElementCount());
    
    auto binary_results = quant_index_->searchKnn(quantized.data(), num_candidates);
    
    std::vector<std::pair<float, labeltype>> rescored;
    while (!binary_results.empty()) {
        auto [hamming_dist, label] = binary_results.top();
        binary_results.pop();
        
        if (original_vectors_.find(label) != original_vectors_.end()) {
            float dist;
            if (metric_ == Metric::Cosine || metric_ == Metric::IP) {
                float sim = hnswlib::cosine_similarity(query, original_vectors_[label].data(), dim_);
                dist = -sim;
            } else {
                dist = l2_distance(query, original_vectors_[label].data(), dim_);
            }
            rescored.emplace_back(dist, label);
        }
    }
    
    std::sort(rescored.begin(), rescored.end());
    
    std::priority_queue<std::pair<float, labeltype>> result;
    for (size_t i = 0; i < std::min(k, rescored.size()); i++) {
        result.push(rescored[i]);
    }
    
    return result;
}

std::vector<std::pair<float, labeltype>> UnifiedIndex::searchWithStopCondition(
    const float* query, float epsilon, size_t min_cand, size_t max_cand) {
    
    std::vector<float> query_normalized;
    const float* query_ptr = query;
    
    if (normalize_) {
        query_normalized.assign(query, query + dim_);
        normalize_l2(query_normalized.data(), dim_);
        query_ptr = query_normalized.data();
    }
    
    std::vector<std::pair<float, labeltype>> results;
    
    if (quantization_ == QuantizationType::NONE) {
        size_t original_ef = float_index_->ef_;
        float_index_->setEf(max_cand);
        
        auto candidates = float_index_->searchKnn(query_ptr, max_cand);
        float_index_->setEf(original_ef);
        
        if (candidates.empty()) return results;
        
        float best_dist = std::numeric_limits<float>::max();
        std::vector<std::pair<float, labeltype>> all_candidates;
        
        while (!candidates.empty()) {
            auto [dist, label] = candidates.top();
            candidates.pop();
            all_candidates.emplace_back(dist, label);
            best_dist = std::min(best_dist, dist);
        }
        
        for (const auto& [dist, label] : all_candidates) {
            if (dist <= best_dist + epsilon) {
                results.emplace_back(dist, label);
            }
        }
        
        if (results.size() < min_cand) {
            std::sort(all_candidates.begin(), all_candidates.end());
            results.clear();
            for (size_t i = 0; i < std::min(min_cand, all_candidates.size()); i++) {
                results.push_back(all_candidates[i]);
            }
        }
        
    } else {
        std::vector<uint8_t> quantized;
        if (quantization_ == QuantizationType::BINARY) {
            quantized.resize(binary_quantizer_->get_nbytes());
            binary_quantizer_->quantize(query_ptr, quantized.data());
        } else if (quantization_ == QuantizationType::TERNARY) {
            quantized.resize(ternary_quantizer_->get_nbytes());
            ternary_quantizer_->quantize(query_ptr, quantized.data());
        }
        
        size_t original_ef = quant_index_->ef_;
        quant_index_->setEf(max_cand);
        
        auto candidates = quant_index_->searchKnn(quantized.data(), max_cand);
        quant_index_->setEf(original_ef);
        
        if (candidates.empty()) return results;
        
        float best_dist = std::numeric_limits<float>::max();
        std::vector<std::pair<float, labeltype>> all_candidates;
        
        while (!candidates.empty()) {
            auto [dist, label] = candidates.top();
            candidates.pop();
            float fdist = static_cast<float>(dist);
            all_candidates.emplace_back(fdist, label);
            best_dist = std::min(best_dist, fdist);
        }
        
        if (enable_rescoring_) {
            std::vector<std::pair<float, labeltype>> rescored;
            for (const auto& [dist, label] : all_candidates) {
                if (original_vectors_.find(label) != original_vectors_.end()) {
                    float true_dist;
                    if (metric_ == Metric::Cosine || metric_ == Metric::IP) {
                        float sim = hnswlib::cosine_similarity(query_ptr, original_vectors_[label].data(), dim_);
                        true_dist = -sim;
                    } else {
                        true_dist = l2_distance(query_ptr, original_vectors_[label].data(), dim_);
                    }
                    rescored.emplace_back(true_dist, label);
                }
            }
            
            if (!rescored.empty()) {
                std::sort(rescored.begin(), rescored.end());
                best_dist = rescored[0].first;
                
                for (const auto& [dist, label] : rescored) {
                    if (dist <= best_dist + epsilon) {
                        results.emplace_back(dist, label);
                    }
                }
                
                if (results.size() < min_cand) {
                    results.clear();
                    for (size_t i = 0; i < std::min(min_cand, rescored.size()); i++) {
                        results.push_back(rescored[i]);
                    }
                }
                
                return results;
            }
        }
        
        for (const auto& [dist, label] : all_candidates) {
            if (dist <= best_dist + epsilon) {
                results.emplace_back(dist, label);
            }
        }
        
        if (results.size() < min_cand) {
            std::sort(all_candidates.begin(), all_candidates.end());
            results.clear();
            for (size_t i = 0; i < std::min(min_cand, all_candidates.size()); i++) {
                results.push_back(all_candidates[i]);
            }
        }
    }
    
    return results;
}

void UnifiedIndex::setEf(size_t ef) {
    ef_ = ef;
    if (float_index_) float_index_->setEf(ef);
    if (quant_index_) quant_index_->setEf(ef);
}

size_t UnifiedIndex::getCurrentElementCount() const {
    if (float_index_) return float_index_->getCurrentElementCount();
    if (quant_index_) return quant_index_->getCurrentElementCount();
    return 0;
}

void UnifiedIndex::saveIndex(const std::string& path) {
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs.is_open()) {
        HNSWERR << "Can't save index: '" << path << "' cannot be opened for writing";
        return; // Can't continue
    }
    meta_.save(ofs);
    
    if (quantization_ == QuantizationType::BINARY && binary_quantizer_) {
        const auto& thresholds = binary_quantizer_->get_thresholds();
        ofs.write(reinterpret_cast<const char*>(thresholds.data()), dim_ * sizeof(float));
    } else if (quantization_ == QuantizationType::TERNARY && ternary_quantizer_) {
        const auto& thr_pos = ternary_quantizer_->get_thresholds_pos();
        const auto& thr_neg = ternary_quantizer_->get_thresholds_neg();
        ofs.write(reinterpret_cast<const char*>(thr_pos.data()), dim_ * sizeof(float));
        ofs.write(reinterpret_cast<const char*>(thr_neg.data()), dim_ * sizeof(float));
    }
    
    if (enable_rescoring_) {
        size_t num_vectors = original_vectors_.size();
        ofs.write(reinterpret_cast<const char*>(&num_vectors), sizeof(size_t));
        
        for (const auto& [label, vec] : original_vectors_) {
            ofs.write(reinterpret_cast<const char*>(&label), sizeof(labeltype));
            ofs.write(reinterpret_cast<const char*>(vec.data()), dim_ * sizeof(float));
        }
    }
    
    if (float_index_) {
        float_index_->saveIndex(ofs);
    } else if (quant_index_) {
        quant_index_->saveIndex(ofs);
    }
    
    ofs.close();
}

bool UnifiedIndex::loadIndex(const std::string& path, bool searchOnly) {
    bool changed = false;
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open()) {
        HNSWERR << "Cannot load index: '" << path << "' cannot be opened for reading";
	return false;
    }
    
    UnifiedIndexMeta loaded_meta;
    if (!loaded_meta.load(ifs)) return false;

    if (loaded_meta.dim_ != dim_) {
#if DELAY_ALLOC == 1
	// dim_ == 0 means we wanted to load something
	if (dim_ != 0) HNSWERR << "Dimension mismatch. Expected " << std::to_string(dim_) <<
                " but got " << std::to_string(loaded_meta.dim_) << std::endl ;
	changed = true;
#else
	throw std::runtime_error("Dimension mismatch. Expected " + std::to_string(dim_) +
                " but got " + std::to_string(loaded_meta.dim_));
	return false;
#endif
    }

    // By specifying max_elements_ = 0 we get a allocator
    size_t  max_elements = max_elements_;
    meta_ = loaded_meta;

    // We want the max of the configured and stored.
    if (max_elements > max_elements_) {
       max_elements_ = max_elements;
       changed = true;
    }
#if 0
    // Check that the index capacity is sufficient
    auto [element_count, max_from_file] = peek_index_elements(ifs);
    if (element_count > max_elements_) {
       if (searchOnly) max_elements_ = elements_count;
       else            max_elements_ = max_from_file;
       changed = true;
    }
#endif
    
#if DELAY_ALLOC == 1
    // Make sure we have an index, create if needed
    if (quantization_ == QuantizationType::NONE) {
        if (changed || !float_space_) create_float_space();
        if (changed || !float_index_) { 
            float_index_ = std::make_unique<HierarchicalNSW<float>>(
                float_space_.get(), max_elements_, M_, ef_construction_);
        }
    } else {
        if (changed || !quant_space_) create_quantized_space();
        if (changed || !quant_index_) {
            quant_index_ = std::make_unique<HierarchicalNSW<size_t>>(
                quant_space_.get(), max_elements_, M_, ef_construction_);
        }
    }           
#endif  
    
    if (quantization_ == QuantizationType::BINARY) {
        std::vector<float> thresholds(dim_);
        ifs.read(reinterpret_cast<char*>(thresholds.data()), dim_ * sizeof(float));
        binary_quantizer_->set_thresholds(thresholds);
    } else if (quantization_ == QuantizationType::TERNARY) {
        std::vector<float> thr_pos(dim_);
        std::vector<float> thr_neg(dim_);
        ifs.read(reinterpret_cast<char*>(thr_pos.data()), dim_ * sizeof(float));
        ifs.read(reinterpret_cast<char*>(thr_neg.data()), dim_ * sizeof(float));
	//
	if (!ifs.good()) return false;
	//
        ternary_quantizer_->set_thresholds(thr_pos, thr_neg);
    }
    
    if (loaded_meta.enable_rescoring_) {
        size_t num_vectors;
        ifs.read(reinterpret_cast<char*>(&num_vectors), sizeof(size_t));
        
        if (enable_rescoring_) {
            original_vectors_.clear();
            for (size_t i = 0; i < num_vectors; i++) {
                labeltype label;
                ifs.read(reinterpret_cast<char*>(&label), sizeof(labeltype));
                std::vector<float> vec(dim_);
                ifs.read(reinterpret_cast<char*>(vec.data()), dim_ * sizeof(float));
		//
		if (!ifs.good()) return false;
		//
                original_vectors_[label] = std::move(vec);
            }
        } else {
            for (size_t i = 0; i < num_vectors; i++) {
                ifs.seekg(sizeof(labeltype) + dim_ * sizeof(float), std::ios::cur);
            }
        }
    }
    
    if (quantization_ == QuantizationType::NONE) {
        if (!float_space_) {
            create_float_space();
        }
        if (!float_index_) {
            float_index_ = std::make_unique<HierarchicalNSW<float>>(
                float_space_.get(), meta_.max_elements_, meta_.M_, meta_.ef_construction_);
        }
        float_index_->loadIndex(ifs, float_space_.get(), meta_.max_elements_);
        float_index_->setEf(meta_.ef_);
    } else {
        if (!quant_space_) {
            create_quantized_space();
        }
        if (!quant_index_) {
            quant_index_ = std::make_unique<HierarchicalNSW<size_t>>(
                quant_space_.get(), meta_.max_elements_, meta_.M_, meta_.ef_construction_);
        }
        quant_index_->loadIndex(ifs, quant_space_.get(), meta_.max_elements_);
        quant_index_->setEf(meta_.ef_);
    }
    
    ifs.close();
    return true;
}

// This removes all elements leaving it empty.
void UnifiedIndex::clear() {
   // We re-use the space
   if (is_quantized()) {
         original_vectors_.clear();
         if (quant_space_) {
             quant_index_ = std::make_unique<hnswlib::HierarchicalNSW<size_t>>(
                quant_space_.get(), max_elements_, M_, ef_construction_);
         }
   } else {
         if (float_space_) {
             float_index_ = std::make_unique<hnswlib::HierarchicalNSW<float>>( 
                float_space_.get(), max_elements_, M_, ef_construction_);
         }
   } 
}   

// Peek at the index file to get element count
std::pair<size_t, size_t> peek_index_elements(std::istream& ifs) {
    // Save current position
    std::streampos original_pos = ifs.tellg();
      
    // HNSWlib saves these values at the start of the file (in order):
    size_t offsetLevel0;
    size_t max_elements;
    size_t cur_element_count;
    size_t size_data_per_element;
/*
    size_t label_offset;
    size_t offsetData;
    size_t max_level;
    size_t enterpoint_node;
    size_t maxM;
    size_t maxM0;
    size_t M;
    size_t mult;
    size_t ef_construction;
*/  
    
    // Read the header
    readBinaryPOD(ifs, offsetLevel0);
    readBinaryPOD(ifs, max_elements);
    readBinaryPOD(ifs, cur_element_count);
    readBinaryPOD(ifs, size_data_per_element); // Read this for debugging

    /* std::cout << "MAX elements = " << max_elements << " count=" << cur_element_count <<
        " data_per_element=" << size_data_per_element << std::endl; */

    // Restore position
    ifs.seekg(original_pos);

    return {cur_element_count,max_elements};
}

// Look at a stored index file and fetch its
// <curent_element_count, max_elements>
// We use this to read an index BEFORE we create a Unified Index to make
// use that we allocate suitable sizes
std::pair<size_t, size_t> peek_index_elements(const std::string path) {
    std::ifstream input(path, std::ios::binary);
    if (input.is_open()) {
      input.seekg(UnifiedIndexMeta::size());
      auto result = peek_index_elements(input);
      input.close();
      return result;
    }   
    return {};
}


/*
Dimension of Common SBERT Models:

all-MiniLM-L6-v2            384    Most popular, fast, good quality
all-mpnet-base-v2           768    Higher quality, slower
all-MiniLM-L12-v2           384    Balance of speed/quality
paraphrase-MiniLM-L6-v2     384    Paraphrase detection
paraphrase-mpnet-base-v2    768    Paraphrase detection
multi-qa-MiniLM-L6-cos-v1   384    Question answering
multi-qa-mpnet-base-cos-v1  768    Question answering

For 384D SBERT vectors:

Float32: 1,536 bytes                   (384 × 4) + 160 = 1,696 bytes 

Binary:  48 bytes (32x compression)    (384 ÷ 8) + 160 = 208 bytes (-88% memory)
Binary + rescoring --> 1,584 bytes (48 + 160 + 1,536) = 1744 bytes (+2% memory)

Ternary: 96 bytes (16x compression)    (384 ÷ 4) + 160 = 256 bytes (-85% memory)
Ternary + rescoring --> 1,584 bytes (96 + 160 + 1,536) = 1792 bytes (+6% memory)

*/

size_t UnifiedIndex::bytes_per_vector() const
{
    size_t vector_bytes = dim_ * sizeof(float);
    if (is_quantized()) {
        size_t quant_bytes;
        if (is_quantized()) { 
            quant_bytes = ((dim_ + 63) / 64) * 8;  // 48 bytes for 384D
        } else {
            quant_bytes = (dim_ + 3) / 4;           // 96 bytes for 384D
        }
        
        size_t graph_overhead = M_ * 10;  // ~160 bytes for M=16
        size_t total = quant_bytes + graph_overhead;
        
        if (enable_rescoring_) {
            total += vector_bytes;  // Add original 1536 bytes for 384D
        }
        
        return total;
    } else {
        // Float metrics
        size_t graph_overhead = M_ * 10;  // ~160 bytes for M=16
        return vector_bytes + graph_overhead;
    }
}



} // namespace hnswlib
