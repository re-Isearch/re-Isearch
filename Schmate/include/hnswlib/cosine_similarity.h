// Cosine Simularity using SIMD 
#pragma once
#include <cstddef>
#include <cmath>
#include <algorithm>

/* Apple M1 Benchmark (higher is better):
dim=64 scalar=59.4135ns simd=9.31394ns speedup=6.37899x
dim=256 scalar=269.422ns simd=52.1515ns speedup=5.16614x
dim=384 scalar=432.189ns simd=79.1694ns speedup=5.45904x <-- SBert 384D
dim=768 scalar=920.869ns simd=192.183ns speedup=4.79162x <-- Sbert 768D
dim=1024 scalar=1238.6ns simd=272.8ns speedup=4.54032x
dim=4096 scalar=5130.37ns simd=1243.1ns speedup=4.12708x

Similar speedups on VX2/AVX-512 class machines.
*/

#if defined(__x86_64__) || defined(_M_X64)
  #include <immintrin.h>
#endif

#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__)
  #include <arm_neon.h>
#endif

#if defined(__ARM_FEATURE_SVE) || defined(__ARM_FEATURE_SVE_BIT__)
  #include <arm_sve.h>
#endif

namespace hnswlib {


// Small epsilon to avoid div-by-zero
static constexpr float cEPS = 1e-8f;

// ---------- Scalar fallback ----------
inline float cosine_scalar(const float* a, const float* b, size_t dim) {
    float dot = 0.0f, norm_a = 0.0f, norm_b = 0.0f;
    for (size_t i = 0; i < dim; ++i) {
        float va = a[i];
        float vb = b[i];
        dot += va * vb;
        norm_a += va * va;
        norm_b += vb * vb;
    }
    return dot / (std::sqrt(norm_a * norm_b) + cEPS);
}

// ---------- AVX / AVX2 (256-bit) ----------
#if defined(__AVX__) || defined(__AVX2__)
inline float cosine_avx(const float* a, const float* b, size_t dim) {
    size_t i = 0;
    const size_t step = 8; // 8 floats per __m256
    __m256 vdot = _mm256_setzero_ps();
    __m256 vna  = _mm256_setzero_ps();
    __m256 vnb  = _mm256_setzero_ps();

    for (; i + step <= dim; i += step) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
        #if defined(__FMA__) || defined(__AVX2__)
        vdot = _mm256_fmadd_ps(va, vb, vdot);
        #else
        vdot = _mm256_add_ps(vdot, _mm256_mul_ps(va, vb));
        #endif
        vna  = _mm256_add_ps(vna, _mm256_mul_ps(va, va));
        vnb  = _mm256_add_ps(vnb, _mm256_mul_ps(vb, vb));
    }

    // horizontal reduce __m256 -> scalar
    auto hsum256 = [](const __m256 x) -> float {
        __m128 hi = _mm256_extractf128_ps(x, 1);
        __m128 lo = _mm256_castps256_ps128(x);
        __m128 sum = _mm_add_ps(lo, hi);
        sum = _mm_hadd_ps(sum, sum);
        sum = _mm_hadd_ps(sum, sum);
        return _mm_cvtss_f32(sum);
    };

    float dot = hsum256(vdot);
    float na  = hsum256(vna);
    float nb  = hsum256(vnb);

    // Remainder
    for (; i < dim; ++i) {
        float va = a[i];
        float vb = b[i];
        dot += va * vb;
        na  += va * va;
        nb  += vb * vb;
    }

    return dot / (std::sqrt(na * nb) + cEPS);
}
#endif

// ---------- AVX-512 (512-bit) ----------
#if defined(__AVX512F__)
inline float cosine_avx512(const float* a, const float* b, size_t dim) {
    size_t i = 0;
    const size_t step = 16; // 16 floats per __m512
    __m512 vdot = _mm512_setzero_ps();
    __m512 vna  = _mm512_setzero_ps();
    __m512 vnb  = _mm512_setzero_ps();

    for (; i + step <= dim; i += step) {
        __m512 va = _mm512_loadu_ps(a + i);
        __m512 vb = _mm512_loadu_ps(b + i);
        #if defined(__FMA__)
        vdot = _mm512_fmadd_ps(va, vb, vdot);
        #else
        vdot = _mm512_add_ps(vdot, _mm512_mul_ps(va, vb));
        #endif
        vna  = _mm512_add_ps(vna, _mm512_mul_ps(va, va));
        vnb  = _mm512_add_ps(vnb, _mm512_mul_ps(vb, vb));
    }

    // reduce __m512 to scalar
    #if defined(__INTEL_COMPILER) || defined(__clang__) || defined(__GNUC__)
    // Many compilers provide _mm512_reduce_add_ps
    float dot = _mm512_reduce_add_ps(vdot);
    float na  = _mm512_reduce_add_ps(vna);
    float nb  = _mm512_reduce_add_ps(vnb);
    #else
    // Fallback reduce by splitting
    __m256 lo, hi;
    lo = _mm256_castps256_ps128(_mm512_castps512_ps256(vdot)); // not portable; but GCC/Clang will have reduce
    // Simpler portable fallback: store then sum (rare path because compilers usually provide reduce)
    alignas(64) float tmpdot[16], tmpna[16], tmpnb[16];
    _mm512_storeu_ps(tmpdot, vdot);
    _mm512_storeu_ps(tmpna, vna);
    _mm512_storeu_ps(tmpnb, vnb);
    float dot = 0.f, na = 0.f, nb = 0.f;
    for (int k = 0; k < 16; ++k) { dot += tmpdot[k]; na += tmpna[k]; nb += tmpnb[k]; }
    #endif

    // Remainder
    for (; i < dim; ++i) {
        float va = a[i];
        float vb = b[i];
        dot += va * vb;
        na  += va * va;
        nb  += vb * vb;
    }

    return dot / (std::sqrt(na * nb) + cEPS);
}
#endif

// ---------- NEON (ARM AArch64) ----------
#if defined(__aarch64__) && (defined(__ARM_NEON) || defined(__ARM_NEON__))
inline float cosine_neon(const float* a, const float* b, size_t dim) {
    size_t i = 0;
    const size_t step = 4;
    float32x4_t vdot = vdupq_n_f32(0.0f);
    float32x4_t vna  = vdupq_n_f32(0.0f);
    float32x4_t vnb  = vdupq_n_f32(0.0f);

    for (; i + step <= dim; i += step) {
        float32x4_t va = vld1q_f32(a + i);
        float32x4_t vb = vld1q_f32(b + i);
        // FMA via vmlaq_f32: vdot += va * vb
        vdot = vmlaq_f32(vdot, va, vb);
        vna  = vmlaq_f32(vna, va, va);
        vnb  = vmlaq_f32(vnb, vb, vb);
    }

    // Reduce 4-lane vectors to scalars: vaddvq_f32 is available in AArch64
    float dot = vaddvq_f32(vdot);
    float na  = vaddvq_f32(vna);
    float nb  = vaddvq_f32(vnb);

    for (; i < dim; ++i) {
        float va = a[i];
        float vb = b[i];
        dot += va * vb;
        na  += va * va;
        nb  += vb * vb;
    }

    return dot / (std::sqrt(na * nb) + cEPS);
}
#endif

// ---------- SVE (Scalable Vector Extension) ----------
#if defined(__ARM_FEATURE_SVE)
inline float cosine_sve(const float* a, const float* b, size_t dim) {
    size_t i = 0;
    svbool_t pg;
    svfloat32_t vdot = svdup_f32(0.0f);
    svfloat32_t vna  = svdup_f32(0.0f);
    svfloat32_t vnb  = svdup_f32(0.0f);

    // svwhilelt_b32 generates a predicate for the active lanes
    while (i < dim) {
        pg = svwhilelt_b32((uint64_t)i, (uint64_t)dim);
        svfloat32_t va = svld1(pg, a + i);
        svfloat32_t vb = svld1(pg, b + i);
        // Elementwise fused multiply-add if available, else multiply + add:
        vdot = svmla_f32(vdot, va, vb);
        vna  = svmla_f32(vna, va, va);
        vnb  = svmla_f32(vnb, vb, vb);
        i += svcntw(); // advance by vector length (in 32-bit words)
    }

    // reduce across lanes
    float dot = svaddv_f32(svptrue_b32(), vdot);
    float na  = svaddv_f32(svptrue_b32(), vna);
    float nb  = svaddv_f32(svptrue_b32(), vnb);

    return dot / (std::sqrt(na * nb) + cEPS);
}
#endif

// ---------- Dispatcher ----------
// Select the best implementation available at compile-time, prefer widest.
// If multiple x86 features are compiled, optionally check runtime support to pick AVX512 if supported.
inline float cosine_similarity(const float* a, const float* b, size_t dim) {
    // x86 runtime detection if compiled with multiple options
    #if defined(__x86_64__)
        #if defined(__AVX512F__)
            // If compiled with AVX-512, use it if runtime supports it (GCC/Clang builtin)
            #if defined(__GNUC__) || defined(__clang__)
                if (__builtin_cpu_supports("avx512f")) {
                    return cosine_avx512(a, b, dim);
                }
                // fall through to AVX/AVX2 if available
            #else
                return cosine_avx512(a, b, dim);
            #endif
        #endif

        #if defined(__AVX__) || defined(__AVX2__)
            #if defined(__GNUC__) || defined(__clang__)
                if (__builtin_cpu_supports("avx2") || __builtin_cpu_supports("avx")) {
                    return cosine_avx(a, b, dim);
                }
            #else
                return cosine_avx(a, b, dim);
            #endif
        #endif

        // fallback scalar
        return cosine_scalar(a, b, dim);
    #elif defined(__aarch64__)
        // ARM: prefer SVE if compiled with it, then NEON, else scalar
        #if defined(__ARM_FEATURE_SVE)
            return cosine_sve(a, b, dim);
        #elif defined(__ARM_NEON) || defined(__ARM_NEON__)
            return cosine_neon(a, b, dim);
        #else
            return cosine_scalar(a, b, dim);
        #endif
    #else
        // Unknown target -> scalar
        return cosine_scalar(a, b, dim);
    #endif
}

} // namespace hnswlib

