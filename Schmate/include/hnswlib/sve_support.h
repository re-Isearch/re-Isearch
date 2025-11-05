// ============================================================================
// HNSWlib ARM SVE (Scalable Vector Extension) Support
// Add to space_l2.h and space_ip.h
// ============================================================================

#pragma once

#ifdef __ARM_FEATURE_SVE
#include <arm_sve.h>
#endif

namespace hnswlib {

// ============================================================================
// SVE L2 Distance Implementation
// ============================================================================

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
// Updated L2Space class with SVE support
// ============================================================================

class L2Space : public SpaceInterface<float> {
    DISTFUNC<float> fstdistfunc_;
    size_t data_size_;
    size_t dim_;

public:
    L2Space(size_t dim) {
        dim_ = dim;
        data_size_ = dim * sizeof(float);
        
        // Priority: SVE > AVX512 > AVX > NEON > Scalar
#if defined(__ARM_FEATURE_SVE)
        fstdistfunc_ = L2SqrSVE16Ext;
#elif defined(USE_AVX512)
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

// ============================================================================
// Updated InnerProductSpace class with SVE support
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
        fstdistfunc_ = InnerProductDistanceSVE;
#elif defined(USE_AVX512)
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

#endif // __ARM_FEATURE_SVE

} // namespace hnswlib

// ============================================================================
// Compilation Instructions
// ============================================================================

/*
To compile with SVE support:

For GCC:
  g++ -std=c++17 -O3 -march=armv8-a+sve your_code.cpp

For specific vector lengths:
  g++ -std=c++17 -O3 -march=armv8-a+sve -msve-vector-bits=256 your_code.cpp
  
For Clang:
  clang++ -std=c++17 -O3 -march=armv8-a+sve your_code.cpp

CMakeLists.txt addition:
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    check_cxx_compiler_flag("-march=armv8-a+sve" COMPILER_SUPPORTS_SVE)
    if(COMPILER_SUPPORTS_SVE)
      message(STATUS "ARM SVE support detected")
      add_compile_options(-march=armv8-a+sve)
    endif()
  endif()

To check SVE at runtime (Linux):
  cat /proc/cpuinfo | grep sve
  
Or programmatically:
  #include <sys/auxv.h>
  #include <asm/hwcap.h>
  
  bool has_sve = getauxval(AT_HWCAP) & HWCAP_SVE;
*/

// ============================================================================
// Performance Comparison
// ============================================================================

/*
Expected performance vs NEON (768-dimensional vectors):

L2 Distance:
- NEON (128-bit):  ~30-40 ns
- SVE (256-bit):   ~20-25 ns (1.5-1.6x faster)
- SVE (512-bit):   ~15-20 ns (2.0-2.5x faster)

Inner Product:
- NEON (128-bit):  ~25-35 ns
- SVE (256-bit):   ~18-22 ns (1.5-1.6x faster)
- SVE (512-bit):   ~12-18 ns (2.0-2.5x faster)

Hardware with SVE:
- Fujitsu A64FX: 512-bit SVE (2.2 GHz, HPC)
- AWS Graviton3: 256-bit SVE (2.6 GHz, cloud)
- AWS Graviton4: 128-bit SVE (optimized for cloud)
- NVIDIA Grace: 128-bit SVE (ARM Neoverse V2)
- Future ARM Neoverse: Variable-length SVE

Key advantage: Same code runs on all SVE implementations!
*/

// ============================================================================
// Testing Code
// ============================================================================

#ifdef INCLUDE_SVE_TEST

#include <iostream>

void test_sve_detection() {
    std::cout << "SVE Detection:\n";
    
#ifdef __ARM_FEATURE_SVE
    std::cout << "  ✓ SVE available at compile time\n";
    std::cout << "  Vector length (32-bit): " << svcntw() << " elements\n";
    std::cout << "  Vector length (bits): " << (svcntw() * 32) << " bits\n";
    
    // Test if it matches common sizes
    uint64_t vl = svcntw();
    if (vl == 4) std::cout << "  Type: 128-bit SVE\n";
    else if (vl == 8) std::cout << "  Type: 256-bit SVE\n";
    else if (vl == 16) std::cout << "  Type: 512-bit SVE\n";
    else if (vl == 32) std::cout << "  Type: 1024-bit SVE\n";
    else if (vl == 64) std::cout << "  Type: 2048-bit SVE\n";
    else std::cout << "  Type: Variable-length SVE\n";
#else
    std::cout << "  ✗ SVE not available\n";
#endif
}

#endif // INCLUDE_SVE_TEST
