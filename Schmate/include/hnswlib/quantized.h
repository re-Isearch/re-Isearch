#pragma once

namespace hnswlib {

enum class QuantMode {
    NONE=0, BIN1=1, INT158=2, INT4=3, INT8=4, INT16=5, FP16=6, BF16 =7
};

// PASS means the Float32 vectors were already quantized!
enum class OptBinMode  { PASS=0, STANDARD, BETTER, CENTROID, ROTATIONAL, RABITQ, RABITQ_EXTENDED };

}

#include <vector>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <numeric>
#include <thread>
#include <atomic>
#include <cassert>
#include <iostream>
#include <fstream>

#include "int_storage.h"
#include "hnswlib.h"

namespace hnswlib {

// Conversion functions
// ---------------------------------------------------------------------
// StorageType → actual bit-packing representation (BIN1, INT2, INT4, INT8, etc.)
// 
// QuantMode → higher-level quantization modes (binary, 1.58-bit, 4-bit, 8-bit).

inline std::optional<QuantMode> toQuantMode(StorageType st) noexcept { 
    switch (st) {        
        case StorageType::BIN1:
            return QuantMode::BIN1;    

        case StorageType::INT2:
        case StorageType::INT3:
            return QuantMode::INT158;  

        case StorageType::INT4:
        case StorageType::INT5:
            return QuantMode::INT4;

        case StorageType::INT6:
        case StorageType::INT8:
            return QuantMode::INT8;

        // There are technically not quantizations!
	case StorageType::FP16:
	    return QuantMode::FP16;
        case StorageType::BF16:
	    return QuantMode::BF16;

        case StorageType::FLOAT32:
            return QuantMode::NONE;
        default:
            return std::nullopt;   // INT16 → no quant mode
    }
}


// No need for optional as StorageTypes cover all quant modes
inline StorageType toStorageType(QuantMode mode) noexcept {
    switch (mode) {
        case QuantMode::NONE:  return StorageType::FLOAT32;
        case QuantMode::BIN1:  return StorageType::BIN1;
        case QuantMode::INT158:return StorageType::INT2;
        case QuantMode::INT4:  return StorageType::INT4;
        case QuantMode::INT8:  return StorageType::INT8;
        case QuantMode::INT16: return StorageType::INT16;
	case QuantMode::FP16:  return StorageType::FP16;
	case QuantMode::BF16:  return StorageType::BF16;
    }
}
// Convenience overload throwing on invalid type
inline QuantMode requireQuantMode(StorageType st) {
    auto q = toQuantMode(st);
    if (!q) throw std::invalid_argument("StorageType cannot be mapped to QuantMode");
    return *q;
}

#if 0 /* SIMD CODE */

#if defined(__AVX2__)
#include <immintrin.h>

inline float l2_int8_avx2(const uint8_t* a, const uint8_t* b, size_t dim)
{
    __m256i acc = _mm256_setzero_si256();
    size_t i = 0;

    for (; i + 31 < dim; i += 32) {
        __m256i va = _mm256_loadu_si256((const __m256i*)(a + i));
        __m256i vb = _mm256_loadu_si256((const __m256i*)(b + i));

        __m256i da = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(va, 0));
        __m256i db = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(vb, 0));
        __m256i diff = _mm256_sub_epi16(da, db);
        acc = _mm256_add_epi32(acc, _mm256_madd_epi16(diff, diff));

        da = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(va, 1));
        db = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(vb, 1));
        diff = _mm256_sub_epi16(da, db);
        acc = _mm256_add_epi32(acc, _mm256_madd_epi16(diff, diff));
    }

    alignas(32) int tmp[8];
    _mm256_store_si256((__m256i*)tmp, acc);

    int sum = tmp[0]+tmp[1]+tmp[2]+tmp[3]+tmp[4]+tmp[5]+tmp[6]+tmp[7];

    // tail
    for (; i < dim; ++i) {
        int d = int(int8_t(a[i])) - int(int8_t(b[i]));
        sum += d * d;
    }

    return float(sum);
}

inline float l2_int4_avx2(const uint8_t* a, const uint8_t* b, size_t dim)
{
    const size_t bytes = (dim + 1) >> 1;
    __m256i acc = _mm256_setzero_si256();

    const __m256i low_mask = _mm256_set1_epi8(0x0F);
    const __m256i sign_fix = _mm256_set1_epi8(8);

    size_t i = 0;
    for (; i + 31 < bytes; i += 32) {
        __m256i va = _mm256_loadu_si256((const __m256i*)(a + i));
        __m256i vb = _mm256_loadu_si256((const __m256i*)(b + i));

        // low nibbles
        __m256i a0 = _mm256_and_si256(va, low_mask);
        __m256i b0 = _mm256_and_si256(vb, low_mask);
        a0 = _mm256_sub_epi8(a0, _mm256_cmpgt_epi8(a0, sign_fix));
        b0 = _mm256_sub_epi8(b0, _mm256_cmpgt_epi8(b0, sign_fix));

        __m256i d0 = _mm256_sub_epi8(a0, b0);
        acc = _mm256_add_epi32(acc,
            _mm256_madd_epi16(_mm256_cvtepi8_epi16(_mm256_extracti128_si256(d0,0)),
                              _mm256_cvtepi8_epi16(_mm256_extracti128_si256(d0,0))));

        // high nibbles
        __m256i a1 = _mm256_and_si256(_mm256_srli_epi16(va, 4), low_mask);
        __m256i b1 = _mm256_and_si256(_mm256_srli_epi16(vb, 4), low_mask);
        a1 = _mm256_sub_epi8(a1, _mm256_cmpgt_epi8(a1, sign_fix));
        b1 = _mm256_sub_epi8(b1, _mm256_cmpgt_epi8(b1, sign_fix));

        __m256i d1 = _mm256_sub_epi8(a1, b1);
        acc = _mm256_add_epi32(acc,
            _mm256_madd_epi16(_mm256_cvtepi8_epi16(_mm256_extracti128_si256(d1,0)),
                              _mm256_cvtepi8_epi16(_mm256_extracti128_si256(d1,0))));
    }

    alignas(32) int tmp[8];
    _mm256_store_si256((__m256i*)tmp, acc);
    int sum = tmp[0]+tmp[1]+tmp[2]+tmp[3]+tmp[4]+tmp[5]+tmp[6]+tmp[7];

    // scalar tail
    for (; i < bytes; ++i) {
        uint8_t ba = a[i], bb = b[i];
        int a0 = (ba & 0x0F); if (a0 >= 8) a0 -= 16;
        int b0 = (bb & 0x0F); if (b0 >= 8) b0 -= 16;
        sum += (a0 - b0)*(a0 - b0);
        if (2*i + 1 < dim) {
            int a1 = (ba >> 4); if (a1 >= 8) a1 -= 16;
            int b1 = (bb >> 4); if (b1 >= 8) b1 -= 16;
            sum += (a1 - b1)*(a1 - b1);
        }
    }

    return float(sum);
}
#endif // defined(__AVX2__)

#if defined(__ARM_NEON)
#include <arm_neon.h>

inline float l2_int8_neon(const uint8_t* a,
                          const uint8_t* b,
                          size_t dim)
{
    int32x4_t acc = vdupq_n_s32(0);
    size_t i = 0;

    for (; i + 15 < dim; i += 16) {
        int8x16_t va = vld1q_s8((const int8_t*)(a + i));
        int8x16_t vb = vld1q_s8((const int8_t*)(b + i));
        int16x8_t d0 = vsubl_s8(vget_low_s8(va), vget_low_s8(vb));
        int16x8_t d1 = vsubl_s8(vget_high_s8(va), vget_high_s8(vb));
        acc = vaddq_s32(acc, vmull_s16(vget_low_s16(d0), vget_low_s16(d0)));
        acc = vaddq_s32(acc, vmull_s16(vget_high_s16(d0), vget_high_s16(d0)));
        acc = vaddq_s32(acc, vmull_s16(vget_low_s16(d1), vget_low_s16(d1)));
        acc = vaddq_s32(acc, vmull_s16(vget_high_s16(d1), vget_high_s16(d1)));
    }

    int sum = vaddvq_s32(acc);
    for (; i < dim; ++i) {
        int d = int(int8_t(a[i])) - int(int8_t(b[i]));
        sum += d*d;
    }
    return float(sum);
}
#endif // defined(__ARM_NEON)

#endif /* SIMD CODE */



// Compute distance with passthrough
inline float compute_dist_L2_pass(int bits, const uint8_t* a, const uint8_t* b, size_t dim)
{
    double acc = 0.0;

    // Fast paths for byte-aligned cases
    if (bits == 8) {
        for (size_t i = 0; i < dim; ++i) {
            int da = int(int8_t(a[i]));
            int db = int(int8_t(b[i]));
            int diff = da - db;
            acc += diff * diff;
        }
        return float(acc);
    }

    if (bits == 4) {
        size_t bytes = (dim + 1) >> 1;
        for (size_t i = 0; i < bytes; ++i) {
            uint8_t ba = a[i];
            uint8_t bb = b[i];

            // low nibble → dim 2*i
            int a0 = ba & 0x0F;
            int b0 = bb & 0x0F;
            if (a0 >= 8) a0 -= 16;
            if (b0 >= 8) b0 -= 16;

            int diff0 = a0 - b0;
            acc += diff0 * diff0;

            // high nibble → dim 2*i + 1
            if ((2*i + 1) < dim) {
                int a1 = ba >> 4;
                int b1 = bb >> 4;
                if (a1 >= 8) a1 -= 16;
                if (b1 >= 8) b1 -= 16;

                int diff1 = a1 - b1;
                acc += diff1 * diff1;
            }
        }
//std::cerr << "ACC = " << acc << std::endl;
        return float(acc);
    }

    // Generic bit-stream path for 1,2,3,5,6 bits
    const uint32_t mask = (1u << bits) - 1;
    const uint32_t sign = 1u << (bits - 1);

    size_t bitpos = 0;
    size_t bytepos = 0;

    for (size_t d = 0; d < dim; ++d) {
        uint32_t va = a[bytepos] >> bitpos;
        uint32_t vb = b[bytepos] >> bitpos;

        if (bitpos + bits > 8) {
            va |= uint32_t(a[bytepos + 1]) << (8 - bitpos);
            vb |= uint32_t(b[bytepos + 1]) << (8 - bitpos);
        }

        va &= mask;
        vb &= mask;

        // signed conversion
        if (va & sign) va -= (1u << bits);
        if (vb & sign) vb -= (1u << bits);

        int diff = int(va) - int(vb);
        acc += diff * diff;

        bitpos += bits;
        bytepos += bitpos >> 3;
        bitpos &= 7;
    }

    return float(acc);
}



} // namespace
