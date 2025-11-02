#pragma once
#include <cmath>
#include <cstddef>
#include <algorithm>

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#endif

#if defined(__aarch64__) || defined(__ARM_NEON)
#include <arm_neon.h>
#endif

#if defined(__ARM_FEATURE_SVE)
#include <arm_sve.h>
#endif

namespace hnswlib {

static constexpr float EPS = 1e-8f;

// ------------------------------------
// Scalar
// ------------------------------------
inline float pearson_scalar(const float* a, const float* b, size_t n) {
    float sum_a = 0.f, sum_b = 0.f;
    for (size_t i = 0; i < n; i++) { sum_a += a[i]; sum_b += b[i]; }
    float mean_a = sum_a / n;
    float mean_b = sum_b / n;

    float num = 0.f, da2 = 0.f, db2 = 0.f;
    for (size_t i = 0; i < n; i++) {
        float da = a[i] - mean_a;
        float db = b[i] - mean_b;
        num += da * db;
        da2 += da * da;
        db2 += db * db;
    }
    return num / (std::sqrt(da2 * db2) + EPS);
}

// ------------------------------------
// AVX2 (256-bit)
// ------------------------------------
#if defined(__AVX__) || defined(__AVX2__)
inline float pearson_avx2(const float* a, const float* b, size_t n) {
    const size_t step = 8;
    size_t i = 0;

    __m256 vsum_a = _mm256_setzero_ps();
    __m256 vsum_b = _mm256_setzero_ps();

    for (; i + step <= n; i += step) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
        vsum_a = _mm256_add_ps(vsum_a, va);
        vsum_b = _mm256_add_ps(vsum_b, vb);
    }

    auto hsum256 = [](const __m256 x) {
        __m128 hi = _mm256_extractf128_ps(x, 1);
        __m128 lo = _mm256_castps256_ps128(x);
        __m128 sum = _mm_add_ps(lo, hi);
        sum = _mm_hadd_ps(sum, sum);
        sum = _mm_hadd_ps(sum, sum);
        return _mm_cvtss_f32(sum);
    };

    float sum_a = hsum256(vsum_a);
    float sum_b = hsum256(vsum_b);

    for (; i < n; i++) { sum_a += a[i]; sum_b += b[i]; }

    float mean_a = sum_a / n;
    float mean_b = sum_b / n;

    // Second pass: numerator + variances
    i = 0;
    __m256 vmean_a = _mm256_set1_ps(mean_a);
    __m256 vmean_b = _mm256_set1_ps(mean_b);
    __m256 vnum = _mm256_setzero_ps();
    __m256 vda2 = _mm256_setzero_ps();
    __m256 vdb2 = _mm256_setzero_ps();

    for (; i + step <= n; i += step) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
        __m256 da = _mm256_sub_ps(va, vmean_a);
        __m256 db = _mm256_sub_ps(vb, vmean_b);
        vnum  = _mm256_fmadd_ps(da, db, vnum);
        vda2  = _mm256_fmadd_ps(da, da, vda2);
        vdb2  = _mm256_fmadd_ps(db, db, vdb2);
    }

    float num = hsum256(vnum);
    float da2 = hsum256(vda2);
    float db2 = hsum256(vdb2);

    for (; i < n; i++) {
        float da = a[i] - mean_a;
        float db = b[i] - mean_b;
        num += da * db;
        da2 += da * da;
        db2 += db * db;
    }

    return num / (std::sqrt(da2 * db2) + EPS);
}
#endif

// ------------------------------------
// AVX-512
// ------------------------------------
#if defined(__AVX512F__)
inline float pearson_avx512(const float* a, const float* b, size_t n) {
    const size_t step = 16;
    size_t i = 0;

    __m512 vsum_a = _mm512_setzero_ps();
    __m512 vsum_b = _mm512_setzero_ps();

    for (; i + step <= n; i += step) {
        __m512 va = _mm512_loadu_ps(a + i);
        __m512 vb = _mm512_loadu_ps(b + i);
        vsum_a = _mm512_add_ps(vsum_a, va);
        vsum_b = _mm512_add_ps(vsum_b, vb);
    }

    float sum_a = _mm512_reduce_add_ps(vsum_a);
    float sum_b = _mm512_reduce_add_ps(vsum_b);

    for (; i < n; i++) { sum_a += a[i]; sum_b += b[i]; }

    float mean_a = sum_a / n;
    float mean_b = sum_b / n;

    i = 0;
    __m512 vmean_a = _mm512_set1_ps(mean_a);
    __m512 vmean_b = _mm512_set1_ps(mean_b);
    __m512 vnum = _mm512_setzero_ps();
    __m512 vda2 = _mm512_setzero_ps();
    __m512 vdb2 = _mm512_setzero_ps();

    for (; i + step <= n; i += step) {
        __m512 va = _mm512_loadu_ps(a + i);
        __m512 vb = _mm512_loadu_ps(b + i);
        __m512 da = _mm512_sub_ps(va, vmean_a);
        __m512 db = _mm512_sub_ps(vb, vmean_b);
        vnum = _mm512_fmadd_ps(da, db, vnum);
        vda2 = _mm512_fmadd_ps(da, da, vda2);
        vdb2 = _mm512_fmadd_ps(db, db, vdb2);
    }

    float num = _mm512_reduce_add_ps(vnum);
    float da2 = _mm512_reduce_add_ps(vda2);
    float db2 = _mm512_reduce_add_ps(vdb2);

    for (; i < n; i++) {
        float da = a[i] - mean_a;
        float db = b[i] - mean_b;
        num += da * db;
        da2 += da * da;
        db2 += db * db;
    }

    return num / (std::sqrt(da2 * db2) + EPS);
}
#endif

// ------------------------------------
// NEON (AArch64)
// ------------------------------------
#if defined(__aarch64__)
inline float pearson_neon(const float* a, const float* b, size_t n) {
    const size_t step = 4;
    size_t i = 0;

    float32x4_t vsum_a = vdupq_n_f32(0);
    float32x4_t vsum_b = vdupq_n_f32(0);

    for (; i + step <= n; i += step) {
        vsum_a = vaddq_f32(vsum_a, vld1q_f32(a + i));
        vsum_b = vaddq_f32(vsum_b, vld1q_f32(b + i));
    }

    float sum_a = vaddvq_f32(vsum_a);
    float sum_b = vaddvq_f32(vsum_b);

    for (; i < n; i++) { sum_a += a[i]; sum_b += b[i]; }

    float mean_a = sum_a / n;
    float mean_b = sum_b / n;

    i = 0;
    const float32x4_t vmean_a = vdupq_n_f32(mean_a);
    const float32x4_t vmean_b = vdupq_n_f32(mean_b);
    float32x4_t vnum = vdupq_n_f32(0);
    float32x4_t vda2 = vdupq_n_f32(0);
    float32x4_t vdb2 = vdupq_n_f32(0);

    for (; i + step <= n; i += step) {
        float32x4_t va = vld1q_f32(a + i);
        float32x4_t vb = vld1q_f32(b + i);
        float32x4_t da = vsubq_f32(va, vmean_a);
        float32x4_t db = vsubq_f32(vb, vmean_b);
        vnum = vmlaq_f32(vnum, da, db);
        vda2 = vmlaq_f32(vda2, da, da);
        vdb2 = vmlaq_f32(vdb2, db, db);
    }

    float num = vaddvq_f32(vnum);
    float da2 = vaddvq_f32(vda2);
    float db2 = vaddvq_f32(vdb2);

    for (; i < n; i++) {
        float da = a[i] - mean_a;
        float db = b[i] - mean_b;
        num += da * db;
        da2 += da * da;
        db2 += db * db;
    }

    return num / (std::sqrt(da2 * db2) + EPS);
}
#endif

// ------------------------------------
// SVE
// ------------------------------------
#if defined(__ARM_FEATURE_SVE)
inline float pearson_sve(const float* a, const float* b, size_t n) {
    size_t i = 0;
    svfloat32_t vsum_a = svdup_f32(0);
    svfloat32_t vsum_b = svdup_f32(0);

    while (i < n) {
        svbool_t pg = svwhilelt_b32(i, n);
        vsum_a = svadd_f32_m(pg, vsum_a, svld1(pg, a + i));
        vsum_b = svadd_f32_m(pg, vsum_b, svld1(pg, b + i));
        i += svcntw();
    }

    float sum_a = svaddv_f32(svptrue_b32(), vsum_a);
    float sum_b = svaddv_f32(svptrue_b32(), vsum_b);

    float mean_a = sum_a / n;
    float mean_b = sum_b / n;

    i = 0;
    svfloat32_t vmean_a = svdup_f32(mean_a);
    svfloat32_t vmean_b = svdup_f32(mean_b);
    svfloat32_t vnum  = svdup_f32(0);
    svfloat32_t vda2  = svdup_f32(0);
    svfloat32_t vdb2  = svdup_f32(0);

    while (i < n) {
        svbool_t pg = svwhilelt_b32(i, n);
        svfloat32_t va = svld1(pg, a + i);
        svfloat32_t vb = svld1(pg, b + i);
        svfloat32_t da = svsub_f32_m(pg, va, vmean_a);
        svfloat32_t db = svsub_f32_m(pg, vb, vmean_b);
        vnum = svmla_f32_m(pg, vnum, da, db);
        vda2 = svmla_f32_m(pg, vda2, da, da);
        vdb2 = svmla_f32_m(pg, vdb2, db, db);
        i += svcntw();
    }

    float num = svaddv_f32(svptrue_b32(), vnum);
    float da2 = svaddv_f32(svptrue_b32(), vda2);
    float db2 = svaddv_f32(svptrue_b32(), vdb2);

    return num / (std::sqrt(da2 * db2) + EPS);
}
#endif

// ------------------------------------
// Runtime dispatch
// ------------------------------------
inline float pearson_dispatch(const float* a, const float* b, size_t n) {
#if defined(__x86_64__)
    #if defined(__AVX512F__) && (defined(__GNUC__) || defined(__clang__))
        if (__builtin_cpu_supports("avx512f"))
            return pearson_avx512(a, b, n);
    #endif
    #if defined(__AVX__) || defined(__AVX2__)
        if (__builtin_cpu_supports("avx2") || __builtin_cpu_supports("avx"))
            return pearson_avx2(a, b, n);
    #endif
    return pearson_scalar(a, b, n);

#elif defined(__aarch64__)
    #if defined(__ARM_FEATURE_SVE)
        return pearson_sve(a, b, n);
    #elif defined(__ARM_NEON)
        return pearson_neon(a, b, n);
    #else
        return pearson_scalar(a, b, n);
    #endif

#else
    return pearson_scalar(a, b, n);
#endif
}

inline float pearson_corr(const std::vector<float>& a, const std::vector<float>& b) {
    assert(a.size() == b.size());
    return pearson_dispatch(a.data(), b.data(), a.size());
}

} // namespace 

