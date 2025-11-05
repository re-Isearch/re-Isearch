// Modified version of hnswlib/space_l2.h with ARM NEON support
// Drop-in replacement for the original space_l2.h

#pragma once
#include "hnswlib.h"

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

namespace hnswlib {

// ============================================================================
// Scalar implementations (fallback)
// ============================================================================

static float L2Sqr(const void* pVect1v, const void* pVect2v, const void* qty_ptr) {
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

static float L2SqrNEON(const void* pVect1v, const void* pVect2v, const void* qty_ptr) {
    float* pVect1 = (float*)pVect1v;
    float* pVect2 = (float*)pVect2v;
    size_t qty = *((size_t*)qty_ptr);
    
    float32x4_t sum_vec = vdupq_n_f32(0.0f);
    
    size_t i = 0;
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

// ============================================================================
// AVX implementations (keep existing)
// ============================================================================

#if defined(USE_AVX)
// Include existing AVX implementations from original file
// L2SqrSIMD16Ext, L2SqrSIMD16ExtResiduals, etc.
#endif

#if defined(USE_AVX512)
// Include existing AVX512 implementations from original file
// L2SqrSIMD16ExtAVX512, etc.
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
        dim_ = dim;
        data_size_ = dim * sizeof(float);
        
        // Automatically select best available implementation
#if defined(USE_AVX512)
        if (dim % 16 == 0) {
            fstdistfunc_ = L2SqrSIMD16ExtAVX512;
        } else {
            fstdistfunc_ = L2SqrSIMD16ExtResiduals;
        }
#elif defined(USE_AVX)
        if (dim % 16 == 0) {
            fstdistfunc_ = L2SqrSIMD16Ext;
        } else {
            fstdistfunc_ = L2SqrSIMD16ExtResiduals;
        }
#elif defined(__ARM_NEON)
        fstdistfunc_ = L2SqrNEON16Ext;
#else
        fstdistfunc_ = L2Sqr;
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

    ~L2Space() {}
};

}  // namespace hnswlib
