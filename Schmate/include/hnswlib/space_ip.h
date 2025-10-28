// Modified version of hnswlib/space_ip.h with ARM NEON support
// Drop-in replacement for the original space_ip.h

#pragma once
#include "hnswlib.h"

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

namespace hnswlib {

// ============================================================================
// Scalar implementations (fallback)
// ============================================================================

static float InnerProduct(const void* pVect1, const void* pVect2, const void* qty_ptr) {
    size_t qty = *((size_t*)qty_ptr);
    float res = 0;
    for (unsigned i = 0; i < qty; i++) {
        res += ((float*)pVect1)[i] * ((float*)pVect2)[i];
    }
    return res;
}

static float InnerProductDistance(const void* pVect1, const void* pVect2, const void* qty_ptr) {
    return 1.0f - InnerProduct(pVect1, pVect2, qty_ptr);
}

// ============================================================================
// ARM NEON implementations
// ============================================================================

#ifdef __ARM_NEON

static float InnerProductNEON(const void* pVect1v, const void* pVect2v, const void* qty_ptr) {
    float* pVect1 = (float*)pVect1v;
    float* pVect2 = (float*)pVect2v;
    size_t qty = *((size_t*)qty_ptr);
    
    float32x4_t sum_vec = vdupq_n_f32(0.0f);
    
    size_t i = 0;
    // Process 16 floats (4 NEON vectors) at a time for better pipelining
    for (; i + 16 <= qty; i += 16) {
        float32x4_t v1_0 = vld1q_f32(pVect1 + i);
        float32x4_t v2_0 = vld1q_f32(pVect2 + i);
        sum_vec = vmlaq_f32(sum_vec, v1_0, v2_0);
        
        float32x4_t v1_1 = vld1q_f32(pVect1 + i + 4);
        float32x4_t v2_1 = vld1q_f32(pVect2 + i + 4);
        sum_vec = vmlaq_f32(sum_vec, v1_1, v2_1);
        
        float32x4_t v1_2 = vld1q_f32(pVect1 + i + 8);
        float32x4_t v2_2 = vld1q_f32(pVect2 + i + 8);
        sum_vec = vmlaq_f32(sum_vec, v1_2, v2_2);
        
        float32x4_t v1_3 = vld1q_f32(pVect1 + i + 12);
        float32x4_t v2_3 = vld1q_f32(pVect2 + i + 12);
        sum_vec = vmlaq_f32(sum_vec, v1_3, v2_3);
    }
    
    // Process remaining 4-element blocks
    for (; i + 4 <= qty; i += 4) {
        float32x4_t v1 = vld1q_f32(pVect1 + i);
        float32x4_t v2 = vld1q_f32(pVect2 + i);
        sum_vec = vmlaq_f32(sum_vec, v1, v2);
    }
    
    // Horizontal sum of NEON vector
    float32x2_t sum_2 = vadd_f32(vget_low_f32(sum_vec), vget_high_f32(sum_vec));
    float sum = vget_lane_f32(vpadd_f32(sum_2, sum_2), 0);
    
    // Handle remaining elements
    for (; i < qty; i++) {
        sum += pVect1[i] * pVect2[i];
    }
    
    return sum;
}

static float InnerProductDistanceNEON(const void* pVect1, const void* pVect2, const void* qty_ptr) {
    return 1.0f - InnerProductNEON(pVect1, pVect2, qty_ptr);
}

static float InnerProductNEON16Ext(const void* pVect1v, const void* pVect2v, const void* qty_ptr) {
    return InnerProductNEON(pVect1v, pVect2v, qty_ptr);
}

static float InnerProductNEON16ExtResiduals(const void* pVect1v, const void* pVect2v, const void* qty_ptr) {
    return InnerProductNEON(pVect1v, pVect2v, qty_ptr);
}

static float InnerProductDistanceNEON16Ext(const void* pVect1, const void* pVect2, const void* qty_ptr) {
    return 1.0f - InnerProductNEON16Ext(pVect1, pVect2, qty_ptr);
}

#endif // __ARM_NEON

// ============================================================================
// AVX implementations (keep existing)
// ============================================================================

#if defined(USE_AVX)

static float InnerProductSIMD4Ext(const void* pVect1v, const void* pVect2v, const void* qty_ptr) {
    float* pVect1 = (float*)pVect1v;
    float* pVect2 = (float*)pVect2v;
    size_t qty = *((size_t*)qty_ptr);

    float PORTABLE_ALIGN32 TmpRes[8];
    size_t qty16 = qty / 16;

    const float* pEnd1 = pVect1 + 16 * qty16;

    __m256 sum = _mm256_set1_ps(0);

    while (pVect1 < pEnd1) {
        __m256 v1 = _mm256_loadu_ps(pVect1);
        pVect1 += 8;
        __m256 v2 = _mm256_loadu_ps(pVect2);
        pVect2 += 8;
        sum = _mm256_add_ps(sum, _mm256_mul_ps(v1, v2));

        v1 = _mm256_loadu_ps(pVect1);
        pVect1 += 8;
        v2 = _mm256_loadu_ps(pVect2);
        pVect2 += 8;
        sum = _mm256_add_ps(sum, _mm256_mul_ps(v1, v2));
    }

    _mm256_store_ps(TmpRes, sum);
    float res = TmpRes[0] + TmpRes[1] + TmpRes[2] + TmpRes[3] + TmpRes[4] + TmpRes[5] + TmpRes[6] + TmpRes[7];

    return res;
}

static float InnerProductDistanceSIMD4Ext(const void* pVect1, const void* pVect2, const void* qty_ptr) {
    return 1.0f - InnerProductSIMD4Ext(pVect1, pVect2, qty_ptr);
}

#endif // USE_AVX

#if defined(USE_AVX512)

static float InnerProductSIMD16ExtAVX512(const void* pVect1v, const void* pVect2v, const void* qty_ptr) {
    float* pVect1 = (float*)pVect1v;
    float* pVect2 = (float*)pVect2v;
    size_t qty = *((size_t*)qty_ptr);

    float PORTABLE_ALIGN64 TmpRes[16];
    size_t qty16 = qty / 16;

    const float* pEnd1 = pVect1 + 16 * qty16;

    __m512 sum512 = _mm512_set1_ps(0);

    while (pVect1 < pEnd1) {
        __m512 v1 = _mm512_loadu_ps(pVect1);
        pVect1 += 16;
        __m512 v2 = _mm512_loadu_ps(pVect2);
        pVect2 += 16;
        sum512 = _mm512_add_ps(sum512, _mm512_mul_ps(v1, v2));
    }

    _mm512_store_ps(TmpRes, sum512);
    float sum = TmpRes[0] + TmpRes[1] + TmpRes[2] + TmpRes[3] + TmpRes[4] + TmpRes[5] + TmpRes[6] + TmpRes[7] +
                TmpRes[8] + TmpRes[9] + TmpRes[10] + TmpRes[11] + TmpRes[12] + TmpRes[13] + TmpRes[14] + TmpRes[15];

    return sum;
}

static float InnerProductDistanceSIMD16ExtAVX512(const void* pVect1, const void* pVect2, const void* qty_ptr) {
    return 1.0f - InnerProductSIMD16ExtAVX512(pVect1, pVect2, qty_ptr);
}

#endif // USE_AVX512

// ============================================================================
// InnerProductSpace class with automatic SIMD selection
// ============================================================================

class InnerProductSpace : public SpaceInterface<float> {
    DISTFUNC<float> fstdistfunc_;
    size_t data_size_;
    size_t dim_;

public:
    InnerProductSpace(size_t dim) {
        dim_ = dim;
        data_size_ = dim * sizeof(float);
        
        // Automatically select best available implementation
#if defined(USE_AVX512)
        fstdistfunc_ = InnerProductDistanceSIMD16ExtAVX512;
#elif defined(USE_AVX)
        fstdistfunc_ = InnerProductDistanceSIMD4Ext;
#elif defined(__ARM_NEON)
        fstdistfunc_ = InnerProductDistanceNEON16Ext;
#else
        fstdistfunc_ = InnerProductDistance;
#endif
    }

    size_t get_data_size() {
        return data_size_;
    }

    DISTFUNC<float> get_dist_func() {
        return fstdistfunc_;
    }

    void* get_dist_func_param() {
        return &dim_;
    }

    ~InnerProductSpace() {}
};

// ============================================================================
// Normalization utilities with NEON support
// ============================================================================

#ifdef __ARM_NEON

static void normalize_vector_neon(float* data, size_t dim) {
    float32x4_t sum_vec = vdupq_n_f32(0.0f);
    
    size_t i = 0;
    // Compute squared sum
    for (; i + 4 <= dim; i += 4) {
        float32x4_t v = vld1q_f32(data + i);
        sum_vec = vmlaq_f32(sum_vec, v, v);
    }
    
    // Horizontal sum
    float32x2_t sum_2 = vadd_f32(vget_low_f32(sum_vec), vget_high_f32(sum_vec));
    float sum = vget_lane_f32(vpadd_f32(sum_2, sum_2), 0);
    
    // Handle remaining elements
    for (; i < dim; i++) {
        sum += data[i] * data[i];
    }
    
    // Compute norm and inverse
    float norm = std::sqrt(sum + 1e-30f);
    float inv_norm = 1.0f / norm;
    float32x4_t norm_vec = vdupq_n_f32(inv_norm);
    
    // Normalize
    i = 0;
    for (; i + 4 <= dim; i += 4) {
        float32x4_t v = vld1q_f32(data + i);
        v = vmulq_f32(v, norm_vec);
        vst1q_f32(data + i, v);
    }
    
    // Handle remaining elements
    for (; i < dim; i++) {
        data[i] *= inv_norm;
    }
}

// Batch normalization
static void normalize_batch_neon(float* data, size_t dim, size_t num_vectors) {
    for (size_t v = 0; v < num_vectors; v++) {
        normalize_vector_neon(data + v * dim, dim);
    }
}

#endif // __ARM_NEON

// Scalar fallback normalization
static void normalize_vector(float* data, size_t dim) {
    float sum = 0.0f;
    for (size_t i = 0; i < dim; i++) {
        sum += data[i] * data[i];
    }
    float norm = std::sqrt(sum + 1e-30f);
    float inv_norm = 1.0f / norm;
    for (size_t i = 0; i < dim; i++) {
        data[i] *= inv_norm;
    }
}

}  // namespace hnswlib
