// Modified version of hnswlib/space_l2.h with ARM NEON and SVE support
// Drop-in replacement for the original space_l2.h

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

static inline float L2Sqr(const void* pVect1v, const void* pVect2v, const void* qty_ptr) {
    float* pVect1 = (float*)pVect1v;
    float* pVect2 = (float*)pVect2v;
    size_t qty = *((size_t*)qty_ptr);
    float res = 0;
    for (size_t i = 0; i < qty; i++) {
        float t = pVect1[i] - pVect2[i];
        res += t * t;
    }
    return (res);
}

// ============================================================================
// ARM NEON implementations
// ============================================================================

#ifdef __ARM_NEON

// Works on both NEON and SVE via bridge
static float L2SqrNEON(const void* pVect1v, const void* pVect2v, const void* qty_ptr) {
    float* pVect1 = (float*)pVect1v;
    float* pVect2 = (float*)pVect2v;
    size_t qty = *((size_t*)qty_ptr);
    
    float32x4_t sum_vec = vdupq_n_f32(0.0f);
    
    size_t i = 0;
    // Process 16 floats at a time
    // On NEON: 4 vectors of 4 floats each
    // On SVE (bridge): 4 SVE vectors (width depends on implementation)
    for (; i + 16 <= qty; i += 16) {
        float32x4_t v1_0 = vld1q_f32(pVect1 + i);
        float32x4_t v2_0 = vld1q_f32(pVect2 + i);
        float32x4_t diff_0 = vsubq_f32(v1_0, v2_0);
        sum_vec = vmlaq_f32(sum_vec, diff_0, diff_0);
        
        float32x4_t v1_1 = vld1q_f32(pVect1 + i + 4);
        float32x4_t v2_1 = vld1q_f32(pVect2 + i + 4);
        float32x4_t diff_1 = vsubq_f32(v1_1, v2_1);
        sum_vec = vmlaq_f32(sum_vec, diff_1, diff_1);
        
        float32x4_t v1_2 = vld1q_f32(pVect1 + i + 8);
        float32x4_t v2_2 = vld1q_f32(pVect2 + i + 8);
        float32x4_t diff_2 = vsubq_f32(v1_2, v2_2);
        sum_vec = vmlaq_f32(sum_vec, diff_2, diff_2);
        
        float32x4_t v1_3 = vld1q_f32(pVect1 + i + 12);
        float32x4_t v2_3 = vld1q_f32(pVect2 + i + 12);
        float32x4_t diff_3 = vsubq_f32(v1_3, v2_3);
        sum_vec = vmlaq_f32(sum_vec, diff_3, diff_3);
    }
    
    for (; i + 4 <= qty; i += 4) {
        float32x4_t v1 = vld1q_f32(pVect1 + i);
        float32x4_t v2 = vld1q_f32(pVect2 + i);
        float32x4_t diff = vsubq_f32(v1, v2);
        sum_vec = vmlaq_f32(sum_vec, diff, diff);
    }
    
    float32x2_t sum_2 = vadd_f32(vget_low_f32(sum_vec), vget_high_f32(sum_vec));
    float sum = vget_lane_f32(vpadd_f32(sum_2, sum_2), 0);
    
    for (; i < qty; i++) {
        float diff = pVect1[i] - pVect2[i];
        sum += diff * diff;
    }
    
    return sum;
}

static float L2SqrNEON16Ext(const void* pVect1v, const void* pVect2v, const void* qty_ptr) {
    return L2SqrNEON(pVect1v, pVect2v, qty_ptr);
}

#endif

#ifdef __ARM_FEATURE_SVE

static float L2SqrSVE(const void* pVect1v, const void* pVect2v, const void* qty_ptr) {
    float* pVect1 = (float*)pVect1v;
    float* pVect2 = (float*)pVect2v;
    size_t qty = *((size_t*)qty_ptr);
    
    // Get vector length (number of float32 elements per vector)
    uint64_t vl = svcntw();
    
    // Initialize accumulator
    svfloat32_t sum_vec = svdup_n_f32(0.0f);
    
    // Create predicate for full vectors
    svbool_t pg_full = svptrue_b32();
    
    uint64_t i = 0;
    
    // Process full vectors
    while (i + vl <= qty) {
        svfloat32_t v1 = svld1_f32(pg_full, pVect1 + i);
        svfloat32_t v2 = svld1_f32(pg_full, pVect2 + i);
        svfloat32_t diff = svsub_f32_z(pg_full, v1, v2);
        sum_vec = svmla_f32_z(pg_full, sum_vec, diff, diff);
        i += vl;
    }
    
    // Handle remaining elements with predication
    if (i < qty) {
        svbool_t pg_remain = svwhilelt_b32(i, qty);
        svfloat32_t v1 = svld1_f32(pg_remain, pVect1 + i);
        svfloat32_t v2 = svld1_f32(pg_remain, pVect2 + i);
        svfloat32_t diff = svsub_f32_z(pg_remain, v1, v2);
        sum_vec = svmla_f32_z(pg_remain, sum_vec, diff, diff);
    }
    
    // Horizontal reduction
    float sum = svaddv_f32(svptrue_b32(), sum_vec);
    
    return sum;
}

// Optimized version processing multiple vectors per iteration
static float L2SqrSVE4Unroll(const void* pVect1v, const void* pVect2v, const void* qty_ptr) {
    float* pVect1 = (float*)pVect1v;
    float* pVect2 = (float*)pVect2v;
    size_t qty = *((size_t*)qty_ptr);
    
    uint64_t vl = svcntw();
    
    // Four accumulators for better pipelining
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
        svfloat32_t diff0 = svsub_f32_z(pg, v1_0, v2_0);
        sum0 = svmla_f32_z(pg, sum0, diff0, diff0);
        
        svfloat32_t v1_1 = svld1_f32(pg, pVect1 + i + vl);
        svfloat32_t v2_1 = svld1_f32(pg, pVect2 + i + vl);
        svfloat32_t diff1 = svsub_f32_z(pg, v1_1, v2_1);
        sum1 = svmla_f32_z(pg, sum1, diff1, diff1);
        
        svfloat32_t v1_2 = svld1_f32(pg, pVect1 + i + 2 * vl);
        svfloat32_t v2_2 = svld1_f32(pg, pVect2 + i + 2 * vl);
        svfloat32_t diff2 = svsub_f32_z(pg, v1_2, v2_2);
        sum2 = svmla_f32_z(pg, sum2, diff2, diff2);
        
        svfloat32_t v1_3 = svld1_f32(pg, pVect1 + i + 3 * vl);
        svfloat32_t v2_3 = svld1_f32(pg, pVect2 + i + 3 * vl);
        svfloat32_t diff3 = svsub_f32_z(pg, v1_3, v2_3);
        sum3 = svmla_f32_z(pg, sum3, diff3, diff3);
        
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
        svfloat32_t diff = svsub_f32_z(pg, v1, v2);
        sum0 = svmla_f32_z(pg, sum0, diff, diff);
        i += vl;
    }
    
    // Handle remaining elements
    if (i < qty) {
        svbool_t pg_remain = svwhilelt_b32(i, qty);
        svfloat32_t v1 = svld1_f32(pg_remain, pVect1 + i);
        svfloat32_t v2 = svld1_f32(pg_remain, pVect2 + i);
        svfloat32_t diff = svsub_f32_z(pg_remain, v1, v2);
        sum0 = svmla_f32_z(pg_remain, sum0, diff, diff);
    }
    
    // Horizontal reduction
    float sum = svaddv_f32(svptrue_b32(), sum0);
    
    return sum;
}

static float L2SqrSVE16Ext(const void* pVect1, const void* pVect2, const void* qty_ptr) {
    return L2SqrSVE4Unroll(pVect1, pVect2, qty_ptr);
}

#endif // __ARM_FEATURE_SVE


// ============================================================================
// AMD/Intel implementations 
// ============================================================================
#if defined(USE_AVX512)

// Favor using AVX512 if available.
static float
L2SqrSIMD16ExtAVX512(const void *pVect1v, const void *pVect2v, const void *qty_ptr) {
    float *pVect1 = (float *) pVect1v;
    float *pVect2 = (float *) pVect2v;
    size_t qty = *((size_t *) qty_ptr);
    float PORTABLE_ALIGN64 TmpRes[16];
    size_t qty16 = qty >> 4;

    const float *pEnd1 = pVect1 + (qty16 << 4);

    __m512 diff, v1, v2;
    __m512 sum = _mm512_set1_ps(0);

    while (pVect1 < pEnd1) {
        v1 = _mm512_loadu_ps(pVect1);
        pVect1 += 16;
        v2 = _mm512_loadu_ps(pVect2);
        pVect2 += 16;
        diff = _mm512_sub_ps(v1, v2);
        // sum = _mm512_fmadd_ps(diff, diff, sum);
        sum = _mm512_add_ps(sum, _mm512_mul_ps(diff, diff));
    }

    _mm512_store_ps(TmpRes, sum);
    float res = TmpRes[0] + TmpRes[1] + TmpRes[2] + TmpRes[3] + TmpRes[4] + TmpRes[5] + TmpRes[6] +
            TmpRes[7] + TmpRes[8] + TmpRes[9] + TmpRes[10] + TmpRes[11] + TmpRes[12] +
            TmpRes[13] + TmpRes[14] + TmpRes[15];

    return (res);
}
#endif

#if defined(USE_AVX)

// Favor using AVX if available.
static float
L2SqrSIMD16ExtAVX(const void *pVect1v, const void *pVect2v, const void *qty_ptr) {
    float *pVect1 = (float *) pVect1v;
    float *pVect2 = (float *) pVect2v;
    size_t qty = *((size_t *) qty_ptr);
    float PORTABLE_ALIGN32 TmpRes[8];
    size_t qty16 = qty >> 4;

    const float *pEnd1 = pVect1 + (qty16 << 4);

    __m256 diff, v1, v2;
    __m256 sum = _mm256_set1_ps(0);

    while (pVect1 < pEnd1) {
        v1 = _mm256_loadu_ps(pVect1);
        pVect1 += 8;
        v2 = _mm256_loadu_ps(pVect2);
        pVect2 += 8;
        diff = _mm256_sub_ps(v1, v2);
        sum = _mm256_add_ps(sum, _mm256_mul_ps(diff, diff));

        v1 = _mm256_loadu_ps(pVect1);
        pVect1 += 8;
        v2 = _mm256_loadu_ps(pVect2);
        pVect2 += 8;
        diff = _mm256_sub_ps(v1, v2);
        sum = _mm256_add_ps(sum, _mm256_mul_ps(diff, diff));
    }

    _mm256_store_ps(TmpRes, sum);
    return TmpRes[0] + TmpRes[1] + TmpRes[2] + TmpRes[3] + TmpRes[4] + TmpRes[5] + TmpRes[6] + TmpRes[7];
}

#endif

#if defined(USE_SSE)

static float
L2SqrSIMD16ExtSSE(const void *pVect1v, const void *pVect2v, const void *qty_ptr) {
    float *pVect1 = (float *) pVect1v;
    float *pVect2 = (float *) pVect2v;
    size_t qty = *((size_t *) qty_ptr);
    float PORTABLE_ALIGN32 TmpRes[8];
    size_t qty16 = qty >> 4;

    const float *pEnd1 = pVect1 + (qty16 << 4);

    __m128 diff, v1, v2;
    __m128 sum = _mm_set1_ps(0);

    while (pVect1 < pEnd1) {
        //_mm_prefetch((char*)(pVect2 + 16), _MM_HINT_T0);
        v1 = _mm_loadu_ps(pVect1);
        pVect1 += 4;
        v2 = _mm_loadu_ps(pVect2);
        pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        sum = _mm_add_ps(sum, _mm_mul_ps(diff, diff));

        v1 = _mm_loadu_ps(pVect1);
        pVect1 += 4;
        v2 = _mm_loadu_ps(pVect2);
        pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        sum = _mm_add_ps(sum, _mm_mul_ps(diff, diff));

        v1 = _mm_loadu_ps(pVect1);
        pVect1 += 4;
        v2 = _mm_loadu_ps(pVect2);
        pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        sum = _mm_add_ps(sum, _mm_mul_ps(diff, diff));

        v1 = _mm_loadu_ps(pVect1);
        pVect1 += 4;
        v2 = _mm_loadu_ps(pVect2);
        pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        sum = _mm_add_ps(sum, _mm_mul_ps(diff, diff));
    }

    _mm_store_ps(TmpRes, sum);
    return TmpRes[0] + TmpRes[1] + TmpRes[2] + TmpRes[3];
}
#endif

#if defined(USE_SSE) || defined(USE_AVX) || defined(USE_AVX512)
static DISTFUNC<float> L2SqrSIMD16Ext = L2SqrSIMD16ExtSSE;

static float
L2SqrSIMD16ExtResiduals(const void *pVect1v, const void *pVect2v, const void *qty_ptr) {
    size_t qty = *((size_t *) qty_ptr);
    size_t qty16 = qty >> 4 << 4;
    float res = L2SqrSIMD16Ext(pVect1v, pVect2v, &qty16);
    float *pVect1 = (float *) pVect1v + qty16;
    float *pVect2 = (float *) pVect2v + qty16;

    size_t qty_left = qty - qty16;
    float res_tail = L2Sqr(pVect1, pVect2, &qty_left);
    return (res + res_tail);
}
#endif


#if defined(USE_SSE)
static float
L2SqrSIMD4Ext(const void *pVect1v, const void *pVect2v, const void *qty_ptr) {
    float PORTABLE_ALIGN32 TmpRes[8];
    float *pVect1 = (float *) pVect1v;
    float *pVect2 = (float *) pVect2v;
    size_t qty = *((size_t *) qty_ptr);


    size_t qty4 = qty >> 2;

    const float *pEnd1 = pVect1 + (qty4 << 2);

    __m128 diff, v1, v2;
    __m128 sum = _mm_set1_ps(0);

    while (pVect1 < pEnd1) {
        v1 = _mm_loadu_ps(pVect1);
        pVect1 += 4;
        v2 = _mm_loadu_ps(pVect2);
        pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        sum = _mm_add_ps(sum, _mm_mul_ps(diff, diff));
    }
    _mm_store_ps(TmpRes, sum);
    return TmpRes[0] + TmpRes[1] + TmpRes[2] + TmpRes[3];
}

static float
L2SqrSIMD4ExtResiduals(const void *pVect1v, const void *pVect2v, const void *qty_ptr) {
    size_t qty = *((size_t *) qty_ptr);
    size_t qty4 = qty >> 2 << 2;

    float res = L2SqrSIMD4Ext(pVect1v, pVect2v, &qty4);
    size_t qty_left = qty - qty4;

    float *pVect1 = (float *) pVect1v + qty4;
    float *pVect2 = (float *) pVect2v + qty4;
    float res_tail = L2Sqr(pVect1, pVect2, &qty_left);

    return (res + res_tail);
}
#endif



// ============================================================================
// L2Space class with automatic SIMD selection
// ============================================================================

class L2Space : public SpaceInterface<float> {
    DISTFUNC<float> fstdistfunc_;
    size_t data_size_;
    size_t dim_;

 public:
    L2Space(size_t dim) {
//        fstdistfunc_ = L2Sqr;
        // Priority: SVE > AVX512 > AVX > NEON > Scalar
#if defined(__ARM_FEATURE_SVE)
        if (has_sve_runtime()) fstdistfunc_ = L2SqrSVE16Ext;
        else fstdistfunc_ = L2SqrNEON16Ext; // Use Neon instead
#elif defined(USE_SSE) || defined(USE_AVX) || defined(USE_AVX512)
    #if defined(USE_AVX512)
        if (AVX512Capable())
            L2SqrSIMD16Ext = L2SqrSIMD16ExtAVX512;
        else if (AVXCapable())
            L2SqrSIMD16Ext = L2SqrSIMD16ExtAVX;
    #elif defined(USE_AVX)
        if (AVXCapable())
            L2SqrSIMD16Ext = L2SqrSIMD16ExtAVX;
    #endif

        if (dim % 16 == 0)
            fstdistfunc_ = L2SqrSIMD16Ext;
        else if (dim % 4 == 0)
            fstdistfunc_ = L2SqrSIMD4Ext;
        else if (dim > 16)
            fstdistfunc_ = L2SqrSIMD16ExtResiduals;
        else if (dim > 4)
            fstdistfunc_ = L2SqrSIMD4ExtResiduals;
#elif defined(__ARM_NEON)
        fstdistfunc_ = L2SqrNEON16Ext;
#else
        fstdistfunc_ = L2Sqr;
#endif
        dim_ = dim;
        data_size_ = dim * sizeof(float);
    }

    size_t get_data_size() {
        return data_size_;
    }

    DISTFUNC<float> get_dist_func() {
        return fstdistfunc_;
    }

    void *get_dist_func_param() {
        return &dim_;
    }

    ~L2Space() {}
};

static int
L2SqrI4x(const void *__restrict pVect1, const void *__restrict pVect2, const void *__restrict qty_ptr) {
    size_t qty = *((size_t *) qty_ptr);
    int res = 0;
    unsigned char *a = (unsigned char *) pVect1;
    unsigned char *b = (unsigned char *) pVect2;

    qty = qty >> 2;
    for (size_t i = 0; i < qty; i++) {
        res += ((*a) - (*b)) * ((*a) - (*b));
        a++;
        b++;
        res += ((*a) - (*b)) * ((*a) - (*b));
        a++;
        b++;
        res += ((*a) - (*b)) * ((*a) - (*b));
        a++;
        b++;
        res += ((*a) - (*b)) * ((*a) - (*b));
        a++;
        b++;
    }
    return (res);
}

static int L2SqrI(const void* __restrict pVect1, const void* __restrict pVect2, const void* __restrict qty_ptr) {
    size_t qty = *((size_t*)qty_ptr);
    int res = 0;
    unsigned char* a = (unsigned char*)pVect1;
    unsigned char* b = (unsigned char*)pVect2;

    for (size_t i = 0; i < qty; i++) {
        res += ((*a) - (*b)) * ((*a) - (*b));
        a++;
        b++;
    }
    return (res);
}

class L2SpaceI : public SpaceInterface<int> {
    DISTFUNC<int> fstdistfunc_;
    size_t data_size_;
    size_t dim_;

 public:
    L2SpaceI(size_t dim) {
        if (dim % 4 == 0) {
            fstdistfunc_ = L2SqrI4x;
        } else {
            fstdistfunc_ = L2SqrI;
        }
        dim_ = dim;
        data_size_ = dim * sizeof(unsigned char);
    }

    size_t get_data_size() {
        return data_size_;
    }

    DISTFUNC<int> get_dist_func() {
        return fstdistfunc_;
    }

    void *get_dist_func_param() {
        return &dim_;
    }

    ~L2SpaceI() {}
};


}  // namespace hnswlib
