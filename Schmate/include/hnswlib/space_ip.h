// Modified version of hnswlib/space_ip.h 
// Drop-in replacement for the original space_ip.h

// This version:
// - ARM NEON and SVE instructions
// - Normalization algorithms: IP + Norms = Cosine Space

#pragma once
#include "hnswlib.h"

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif
#ifdef __ARM_FEATURE_SVE
#include <arm_sve.h>
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

static inline float InnerProductDistance(const void* pVect1, const void* pVect2, const void* qty_ptr) {
    return 1.0f - InnerProduct(pVect1, pVect2, qty_ptr);
}

// ============================================================================
// ARM NEON implementations
// ============================================================================

#ifdef __ARM_NEON

// Works on both NEON and SVE via bridge
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

static inline float InnerProductDistanceNEON(const void* pVect1, const void* pVect2, const void* qty_ptr) {
    return 1.0f - InnerProductNEON(pVect1, pVect2, qty_ptr);
}

static inline float InnerProductNEON16Ext(const void* pVect1v, const void* pVect2v, const void* qty_ptr) {
    return InnerProductNEON(pVect1v, pVect2v, qty_ptr);
}

static inline float InnerProductNEON16ExtResiduals(const void* pVect1v, const void* pVect2v, const void* qty_ptr) {
    return InnerProductNEON(pVect1v, pVect2v, qty_ptr);
}

static inline float InnerProductDistanceNEON16Ext(const void* pVect1, const void* pVect2, const void* qty_ptr) {
    return 1.0f - InnerProductNEON16Ext(pVect1, pVect2, qty_ptr);
}

#endif // __ARM_NEON


// ============================================================================
// SVE Inner Product Implementation
// ============================================================================

#ifdef __ARM_FEATURE_SVE

static float InnerProductSVE(const void* pVect1v, const void* pVect2v, const void* qty_ptr) {
    float* pVect1 = (float*)pVect1v;
    float* pVect2 = (float*)pVect2v;
    size_t qty = *((size_t*)qty_ptr);
    
    uint64_t vl = svcntw();
    
    svfloat32_t sum_vec = svdup_n_f32(0.0f);
    svbool_t pg_full = svptrue_b32();
    
    uint64_t i = 0;
    
    // Process full vectors
    while (i + vl <= qty) {
        svfloat32_t v1 = svld1_f32(pg_full, pVect1 + i);
        svfloat32_t v2 = svld1_f32(pg_full, pVect2 + i);
        sum_vec = svmla_f32_z(pg_full, sum_vec, v1, v2);
        i += vl;
    }
    
    // Handle remaining elements
    if (i < qty) {
        svbool_t pg_remain = svwhilelt_b32(i, qty);
        svfloat32_t v1 = svld1_f32(pg_remain, pVect1 + i);
        svfloat32_t v2 = svld1_f32(pg_remain, pVect2 + i);
        sum_vec = svmla_f32_z(pg_remain, sum_vec, v1, v2);
    }
    
    // Horizontal reduction
    float sum = svaddv_f32(svptrue_b32(), sum_vec);
    
    return sum;
}

// Optimized with 4-way unrolling
static float InnerProductSVE4Unroll(const void* pVect1v, const void* pVect2v, const void* qty_ptr) {
    float* pVect1 = (float*)pVect1v;
    float* pVect2 = (float*)pVect2v;
    size_t qty = *((size_t*)qty_ptr);
    
    uint64_t vl = svcntw();
    
    svfloat32_t sum0 = svdup_n_f32(0.0f);
    svfloat32_t sum1 = svdup_n_f32(0.0f);
    svfloat32_t sum2 = svdup_n_f32(0.0f);
    svfloat32_t sum3 = svdup_n_f32(0.0f);
    
    svbool_t pg = svptrue_b32();
    
    uint64_t i = 0;
    
    // Process 4 vectors at a time
    while (i + 4 * vl <= qty) {
        svfloat32_t v1_0 = svld1_f32(pg, pVect1 + i);
        svfloat32_t v2_0 = svld1_f32(pg, pVect2 + i);
        sum0 = svmla_f32_z(pg, sum0, v1_0, v2_0);
        
        svfloat32_t v1_1 = svld1_f32(pg, pVect1 + i + vl);
        svfloat32_t v2_1 = svld1_f32(pg, pVect2 + i + vl);
        sum1 = svmla_f32_z(pg, sum1, v1_1, v2_1);
        
        svfloat32_t v1_2 = svld1_f32(pg, pVect1 + i + 2 * vl);
        svfloat32_t v2_2 = svld1_f32(pg, pVect2 + i + 2 * vl);
        sum2 = svmla_f32_z(pg, sum2, v1_2, v2_2);
        
        svfloat32_t v1_3 = svld1_f32(pg, pVect1 + i + 3 * vl);
        svfloat32_t v2_3 = svld1_f32(pg, pVect2 + i + 3 * vl);
        sum3 = svmla_f32_z(pg, sum3, v1_3, v2_3);
        
        i += 4 * vl;
    }
    
    // Combine accumulators
    sum0 = svadd_f32_z(pg, sum0, sum1);
    sum2 = svadd_f32_z(pg, sum2, sum3);
    sum0 = svadd_f32_z(pg, sum0, sum2);
    
    // Process remaining full vectors
    while (i + vl <= qty) {
        svfloat32_t v1 = svld1_f32(pg, pVect1 + i);
        svfloat32_t v2 = svld1_f32(pg, pVect2 + i);
        sum0 = svmla_f32_z(pg, sum0, v1, v2);
        i += vl;
    }
    
    // Handle remaining elements
    if (i < qty) {
        svbool_t pg_remain = svwhilelt_b32(i, qty);
        svfloat32_t v1 = svld1_f32(pg_remain, pVect1 + i);
        svfloat32_t v2 = svld1_f32(pg_remain, pVect2 + i);
        sum0 = svmla_f32_z(pg_remain, sum0, v1, v2);
    }
    
    // Horizontal reduction
    float sum = svaddv_f32(svptrue_b32(), sum0);
    
    return sum;
}

static float InnerProductDistanceSVE(const void* pVect1, const void* pVect2, const void* qty_ptr) {
    return 1.0f - InnerProductSVE4Unroll(pVect1, pVect2, qty_ptr);
}

static float InnerProductSVE16Ext(const void* pVect1, const void* pVect2, const void* qty_ptr) {
    return InnerProductSVE4Unroll(pVect1, pVect2, qty_ptr);
}

#endif // __ARM_FEATURE_SVE


// ============================================================================
// AVX implementations 
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

#if defined(USE_AVX)

static float
InnerProductSIMD16ExtAVX(const void *pVect1v, const void *pVect2v, const void *qty_ptr) {
    float PORTABLE_ALIGN32 TmpRes[8];
    float *pVect1 = (float *) pVect1v;
    float *pVect2 = (float *) pVect2v;
    size_t qty = *((size_t *) qty_ptr);

    size_t qty16 = qty / 16;


    const float *pEnd1 = pVect1 + 16 * qty16;

    __m256 sum256 = _mm256_set1_ps(0);

    while (pVect1 < pEnd1) {
        //_mm_prefetch((char*)(pVect2 + 16), _MM_HINT_T0);

        __m256 v1 = _mm256_loadu_ps(pVect1);
        pVect1 += 8;
        __m256 v2 = _mm256_loadu_ps(pVect2);
        pVect2 += 8;
        sum256 = _mm256_add_ps(sum256, _mm256_mul_ps(v1, v2));

        v1 = _mm256_loadu_ps(pVect1);
        pVect1 += 8;
        v2 = _mm256_loadu_ps(pVect2);
        pVect2 += 8;
        sum256 = _mm256_add_ps(sum256, _mm256_mul_ps(v1, v2));
    }

    _mm256_store_ps(TmpRes, sum256);
    float sum = TmpRes[0] + TmpRes[1] + TmpRes[2] + TmpRes[3] + TmpRes[4] + TmpRes[5] + TmpRes[6] + TmpRes[7];

    return sum;
}

static float
InnerProductDistanceSIMD16ExtAVX(const void *pVect1v, const void *pVect2v, const void *qty_ptr) {
    return 1.0f - InnerProductSIMD16ExtAVX(pVect1v, pVect2v, qty_ptr);
}

#endif

#if defined(USE_SSE)

static float
InnerProductSIMD16ExtSSE(const void *pVect1v, const void *pVect2v, const void *qty_ptr) {
    float PORTABLE_ALIGN32 TmpRes[8];
    float *pVect1 = (float *) pVect1v;
    float *pVect2 = (float *) pVect2v;
    size_t qty = *((size_t *) qty_ptr);

    size_t qty16 = qty / 16;

    const float *pEnd1 = pVect1 + 16 * qty16;

    __m128 v1, v2;
    __m128 sum_prod = _mm_set1_ps(0);

    while (pVect1 < pEnd1) {
        v1 = _mm_loadu_ps(pVect1);
        pVect1 += 4;
        v2 = _mm_loadu_ps(pVect2);
        pVect2 += 4;
        sum_prod = _mm_add_ps(sum_prod, _mm_mul_ps(v1, v2));

        v1 = _mm_loadu_ps(pVect1);
        pVect1 += 4;
        v2 = _mm_loadu_ps(pVect2);
        pVect2 += 4;
        sum_prod = _mm_add_ps(sum_prod, _mm_mul_ps(v1, v2));

        v1 = _mm_loadu_ps(pVect1);
        pVect1 += 4;
        v2 = _mm_loadu_ps(pVect2);
        pVect2 += 4;
        sum_prod = _mm_add_ps(sum_prod, _mm_mul_ps(v1, v2));

        v1 = _mm_loadu_ps(pVect1);
        pVect1 += 4;
        v2 = _mm_loadu_ps(pVect2);
        pVect2 += 4;
        sum_prod = _mm_add_ps(sum_prod, _mm_mul_ps(v1, v2));
    }
    _mm_store_ps(TmpRes, sum_prod);
    float sum = TmpRes[0] + TmpRes[1] + TmpRes[2] + TmpRes[3];

    return sum;
}

static float
InnerProductDistanceSIMD16ExtSSE(const void *pVect1v, const void *pVect2v, const void *qty_ptr) {
    return 1.0f - InnerProductSIMD16ExtSSE(pVect1v, pVect2v, qty_ptr);
}

#endif

#if defined(USE_SSE) || defined(USE_AVX) || defined(USE_AVX512)
static DISTFUNC<float> InnerProductSIMD16Ext = InnerProductSIMD16ExtSSE;
static DISTFUNC<float> InnerProductSIMD4Ext = InnerProductSIMD4ExtSSE;
static DISTFUNC<float> InnerProductDistanceSIMD16Ext = InnerProductDistanceSIMD16ExtSSE;
static DISTFUNC<float> InnerProductDistanceSIMD4Ext = InnerProductDistanceSIMD4ExtSSE;

static float
InnerProductDistanceSIMD16ExtResiduals(const void *pVect1v, const void *pVect2v, const void *qty_ptr) {
    size_t qty = *((size_t *) qty_ptr);
    size_t qty16 = qty >> 4 << 4;
    float res = InnerProductSIMD16Ext(pVect1v, pVect2v, &qty16);
    float *pVect1 = (float *) pVect1v + qty16;
    float *pVect2 = (float *) pVect2v + qty16;

    size_t qty_left = qty - qty16;
    float res_tail = InnerProduct(pVect1, pVect2, &qty_left);
    return 1.0f - (res + res_tail);
}

static float
InnerProductDistanceSIMD4ExtResiduals(const void *pVect1v, const void *pVect2v, const void *qty_ptr) {
    size_t qty = *((size_t *) qty_ptr);
    size_t qty4 = qty >> 2 << 2;

    float res = InnerProductSIMD4Ext(pVect1v, pVect2v, &qty4);
    size_t qty_left = qty - qty4;

    float *pVect1 = (float *) pVect1v + qty4;
    float *pVect2 = (float *) pVect2v + qty4;
    float res_tail = InnerProduct(pVect1, pVect2, &qty_left);

    return 1.0f - (res + res_tail);
}
#endif


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

        // Priority: SVE > AVX512 > AVX > NEON > Scalar
#if defined(__ARM_FEATURE_SVE)
        if ( has_sve_runtime()) fstdistfunc_ = InnerProductDistanceSVE;
        else fstdistfunc_ = InnerProductDistanceNEON16Ext; // Use NEON
#elif defined(USE_AVX) || defined(USE_SSE) || defined(USE_AVX512)
    #if defined(USE_AVX512)
        if (AVX512Capable()) {
            // OK
            InnerProductSIMD16Ext = InnerProductSIMD16ExtAVX512;
            InnerProductDistanceSIMD16Ext = InnerProductDistanceSIMD16ExtAVX512;
        } else if (AVXCapable()) {
            InnerProductSIMD16Ext = InnerProductSIMD16ExtAVX;
            InnerProductDistanceSIMD16Ext = InnerProductDistanceSIMD16ExtAVX;
        }
    #elif defined(USE_AVX)
        if (AVXCapable()) {
            InnerProductSIMD16Ext = InnerProductSIMD16ExtAVX;
            InnerProductDistanceSIMD16Ext = InnerProductDistanceSIMD16ExtAVX;
        } 
    #endif
    #if defined(USE_AVX)
        if (AVXCapable()) {
            InnerProductSIMD4Ext = InnerProductSIMD4ExtAVX;
            InnerProductDistanceSIMD4Ext = InnerProductDistanceSIMD4ExtAVX;
        }
    #endif
        if (dim % 16 == 0)
            fstdistfunc_ = InnerProductDistanceSIMD16Ext;
        else if (dim % 4 == 0)
            fstdistfunc_ = InnerProductDistanceSIMD4Ext;
        else if (dim > 16) 
            fstdistfunc_ = InnerProductDistanceSIMD16ExtResiduals;
        else if (dim > 4)
            fstdistfunc_ = InnerProductDistanceSIMD4ExtResiduals;
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
static inline void normalize_batch_neon(float* data, size_t dim, size_t num_vectors) {
    for (size_t v = 0; v < num_vectors; v++) {
        normalize_vector_neon(data + v * dim, dim);
    }
}

#endif // __ARM_NEON

// ============================================================================
// SVE Normalization
// ============================================================================

#ifdef __ARM_FEATURE_SVE

static void normalize_vector_sve(float* data, size_t dim) {
    uint64_t vl = svcntw();
    
    // Compute squared sum
    svfloat32_t sum_vec = svdup_n_f32(0.0f);
    svbool_t pg_full = svptrue_b32();
    
    uint64_t i = 0;
    while (i + vl <= dim) {
        svfloat32_t v = svld1_f32(pg_full, data + i);
        sum_vec = svmla_f32_z(pg_full, sum_vec, v, v);
        i += vl;
    }
    
    if (i < dim) {
        svbool_t pg_remain = svwhilelt_b32(i, dim);
        svfloat32_t v = svld1_f32(pg_remain, data + i);
        sum_vec = svmla_f32_z(pg_remain, sum_vec, v, v);
    }
    
    float sum = svaddv_f32(svptrue_b32(), sum_vec);
    
    // Compute norm and inverse
    float norm = std::sqrt(sum + 1e-30f);
    float inv_norm = 1.0f / norm;
    svfloat32_t inv_norm_vec = svdup_n_f32(inv_norm);
    
    // Normalize
    i = 0;
    while (i + vl <= dim) {
        svfloat32_t v = svld1_f32(pg_full, data + i);
        v = svmul_f32_z(pg_full, v, inv_norm_vec);
        svst1_f32(pg_full, data + i, v);
        i += vl;
    }
    
    if (i < dim) {
        svbool_t pg_remain = svwhilelt_b32(i, dim);
        svfloat32_t v = svld1_f32(pg_remain, data + i);
        v = svmul_f32_z(pg_remain, v, inv_norm_vec);
        svst1_f32(pg_remain, data + i, v);
    }
}

// Batch normalization
static void normalize_batch_sve(float* data, size_t dim, size_t num_vectors) {
    for (size_t v = 0; v < num_vectors; v++) {
        normalize_vector_sve(data + v * dim, dim);
    }
}

#endif // __ARM_FEATURE_SVE


//////

// ============================================================================
// AVX2 implementation
// ============================================================================

#ifdef __AVX2__

static void normalize_vector_avx2(float* data, size_t dim) {
    __m256 sum_vec = _mm256_setzero_ps();
    
    size_t i = 0;
    // Compute squared sum
    for (; i + 8 <= dim; i += 8) {
        __m256 v = _mm256_loadu_ps(data + i);
        sum_vec = _mm256_fmadd_ps(v, v, sum_vec);
    }
    
    // Horizontal sum
    __m128 sum_high = _mm256_extractf128_ps(sum_vec, 1);
    __m128 sum_low = _mm256_castps256_ps128(sum_vec);
    __m128 sum128 = _mm_add_ps(sum_low, sum_high);
    sum128 = _mm_hadd_ps(sum128, sum128);
    sum128 = _mm_hadd_ps(sum128, sum128);
    float sum = _mm_cvtss_f32(sum128);
    
    // Handle remaining elements
    for (; i < dim; i++) {
        sum += data[i] * data[i];
    }
    
    // Compute norm and inverse
    float norm = std::sqrt(sum + 1e-30f);
    float inv_norm = 1.0f / norm;
    __m256 norm_vec = _mm256_set1_ps(inv_norm);
    
    // Normalize
    i = 0;
    for (; i + 8 <= dim; i += 8) {
        __m256 v = _mm256_loadu_ps(data + i);
        v = _mm256_mul_ps(v, norm_vec);
        _mm256_storeu_ps(data + i, v);
    }
    
    // Handle remaining elements
    for (; i < dim; i++) {
        data[i] *= inv_norm;
    }
}

// Batch normalization
static void normalize_batch_avx2(float* data, size_t dim, size_t num_vectors) {
    for (size_t v = 0; v < num_vectors; v++) {
        normalize_vector_avx2(data + v * dim, dim);
    }
}


#endif // __AVX2__

// Scalar fallback normalization
static void normalize_vector_scalar(float* data, size_t dim) {
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

// Batch normalization
static inline void normalize_batch_scalar(float* data, size_t dim, size_t num_vectors) {
    for (size_t v = 0; v < num_vectors; v++) {
        normalize_vector_scalar(data + v * dim, dim);
    }
}


// ============================================================================
// Unified normalize_l2 function (auto-selects best implementation)
// ============================================================================

inline void normalize_l2(float* data, size_t dim) {
#if defined(__ARM_FEATURE_SVE)
    static use_sve = has_sve_runtime(); 
    if (use_sve) normalize_vector_sve(data, dim);
    else normalize_vector_neon(data, dim);
#elif defined(__AVX2__)
    normalize_vector_avx2(data, dim);
#elif defined(__ARM_NEON)
    normalize_vector_neon(data, dim);
#else
    normalize_vector_scalar(data, dim);
#endif
}

inline void normalize_l2_batch(std::vector<std::vector<float>>& embeddings) {
    for (auto& emb : embeddings) {
        normalize_l2(emb.data(), emb.size());
    }
}

inline void normalize_l2_batch(float* data, size_t dim, size_t num_vectors) {
    for (size_t v = 0; v < num_vectors; v++) {
        normalize_l2(data + v * dim, dim);
    }
}

// ============================================================================
// Function pointer based dynamic dispatch (advanced usage)
// ============================================================================

typedef void (*normalize_func_t)(float*, size_t);

inline normalize_func_t get_normalize_function() {
#if defined(__ARM_FEATURE_SVE)
    if (has_sve_runtime()) return normalize_vector_sve;
    return normalize_vector_neon(data, dim);
#elif defined(__AVX2__)
    return normalize_vector_avx2;
#elif defined(__ARM_NEON)
    return normalize_vector_neon;
#else
    return normalize_vector_scalar;
#endif
}

//////

}  // namespace hnswlib
