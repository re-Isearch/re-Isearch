// ARM SVE (Scalable Vector Extension) Support for Binary Quantization
#pragma once

#ifdef __ARM_FEATURE_SVE
#include <arm_sve.h>
#endif

/*
Key Advantages of SVE over NEON:

Scalable vectors: Works with 128-bit, 256-bit, 512-bit, 1024-bit, or even 2048-bit vectors
Predication: Better handling of loop remainders
Future-proof: Code works on future ARM CPUs with different vector lengths

4. Hardware Support:

Fujitsu A64FX (512-bit SVE)
AWS Graviton3 (256-bit SVE)
NVIDIA Grace (128-bit SVE)
Future ARM Neoverse CPUs
*/

namespace hnswlib {

#ifdef __ARM_FEATURE_SVE

// ============================================================================
// SVE Binarization
// ============================================================================

void binarize_sve(const float* emb, const float* thr, size_t dim, uint8_t* out) {
    size_t nbytes = (dim + 7) / 8;
    std::memset(out, 0, nbytes);
    
    // Get the vector length in elements (for float32)
    size_t vl = svcntw();  // Count of 32-bit elements in a vector
    
    size_t d = 0;
    
    // Process full vectors
    while (d + vl <= dim) {
        // Load vectors
        svbool_t pg = svptrue_b32();
        svfloat32_t v = svld1_f32(pg, emb + d);
        svfloat32_t t = svld1_f32(pg, thr + d);
        
        // Compare: v > t
        svbool_t cmp = svcmpgt_f32(pg, v, t);
        
        // Convert predicate to bitmask
        // Process in chunks of 8 (one byte)
        for (size_t i = 0; i < vl && (d + i) < dim; i++) {
            if (svptest_any(pg, cmp)) {
                // Extract bit at position i
                svbool_t bit_mask = svwhilelt_b32((uint32_t)i, (uint32_t)(i + 1));
                svbool_t bit = svand_b_z(pg, cmp, bit_mask);
                
                if (svptest_any(pg, bit)) {
                    size_t idx = d + i;
                    out[idx >> 3] |= 1u << (idx & 7);
                }
            }
            
            // Rotate comparison mask
            cmp = svext_b32(cmp, cmp, 1);
        }
        
        d += vl;
    }
    
    // Handle remaining elements with predicated operations
    if (d < dim) {
        size_t remaining = dim - d;
        svbool_t pg = svwhilelt_b32((uint32_t)0, (uint32_t)remaining);
        
        svfloat32_t v = svld1_f32(pg, emb + d);
        svfloat32_t t = svld1_f32(pg, thr + d);
        svbool_t cmp = svcmpgt_f32(pg, v, t);
        
        for (size_t i = 0; i < remaining; i++) {
            svbool_t bit_mask = svwhilelt_b32((uint32_t)i, (uint32_t)(i + 1));
            svbool_t bit = svand_b_z(pg, cmp, bit_mask);
            
            if (svptest_any(pg, bit)) {
                size_t idx = d + i;
                out[idx >> 3] |= 1u << (idx & 7);
            }
            
            cmp = svext_b32(cmp, cmp, 1);
        }
    }
}

// Alternative optimized version using byte packing
void binarize_sve_optimized(const float* emb, const float* thr, size_t dim, uint8_t* out) {
    size_t nbytes = (dim + 7) / 8;
    std::memset(out, 0, nbytes);
    
    size_t vl = svcntw();
    size_t d = 0;
    
    // Process 8 elements at a time to pack into bytes efficiently
    while (d + 8 <= dim) {
        svbool_t pg = svwhilelt_b32((uint32_t)0, (uint32_t)8);
        
        svfloat32_t v = svld1_f32(pg, emb + d);
        svfloat32_t t = svld1_f32(pg, thr + d);
        svbool_t cmp = svcmpgt_f32(pg, v, t);
        
        // Pack 8 comparison results into one byte
        uint8_t byte = 0;
        for (int i = 0; i < 8; i++) {
            svbool_t bit_mask = svwhilelt_b32((uint32_t)i, (uint32_t)(i + 1));
            if (svptest_any(svand_b_z(pg, cmp, bit_mask), pg)) {
                byte |= 1u << i;
            }
            cmp = svext_b32(cmp, cmp, 1);
        }
        
        out[d >> 3] = byte;
        d += 8;
    }
    
    // Handle remaining elements
    for (; d < dim; d++) {
        if (emb[d] > thr[d]) {
            out[d >> 3] |= 1u << (d & 7);
        }
    }
}

// ============================================================================
// SVE Hamming Distance
// ============================================================================

uint32_t hamming_distance_sve(const uint8_t* a, const uint8_t* b, size_t nbytes) {
    uint32_t dist = 0;
    
    // Get vector length for 8-bit elements
    size_t vl = svcntb();
    size_t i = 0;
    
    // Process full vectors
    while (i + vl <= nbytes) {
        svbool_t pg = svptrue_b8();
        
        // Load vectors
        svuint8_t va = svld1_u8(pg, a + i);
        svuint8_t vb = svld1_u8(pg, b + i);
        
        // XOR
        svuint8_t vxor = sveor_u8_z(pg, va, vb);
        
        // Count bits using cnt instruction (population count)
        svuint8_t vcnt = svcnt_u8_z(pg, vxor);
        
        // Sum all counts
        dist += svaddv_u8(pg, vcnt);
        
        i += vl;
    }
    
    // Handle remaining bytes
    if (i < nbytes) {
        size_t remaining = nbytes - i;
        svbool_t pg = svwhilelt_b8((uint32_t)0, (uint32_t)remaining);
        
        svuint8_t va = svld1_u8(pg, a + i);
        svuint8_t vb = svld1_u8(pg, b + i);
        svuint8_t vxor = sveor_u8_z(pg, va, vb);
        svuint8_t vcnt = svcnt_u8_z(pg, vxor);
        
        dist += svaddv_u8(pg, vcnt);
    }
    
    return dist;
}

// Alternative: Process 16-bit chunks for potentially better performance
uint32_t hamming_distance_sve_16bit(const uint8_t* a, const uint8_t* b, size_t nbytes) {
    uint32_t dist = 0;
    
    // Process as 16-bit to use wider operations
    size_t n16 = nbytes / 2;
    const uint16_t* a16 = reinterpret_cast<const uint16_t*>(a);
    const uint16_t* b16 = reinterpret_cast<const uint16_t*>(b);
    
    size_t vl = svcnth();  // Vector length for 16-bit elements
    size_t i = 0;
    
    while (i + vl <= n16) {
        svbool_t pg = svptrue_b16();
        
        svuint16_t va = svld1_u16(pg, a16 + i);
        svuint16_t vb = svld1_u16(pg, b16 + i);
        svuint16_t vxor = sveor_u16_z(pg, va, vb);
        
        // Count bits in 16-bit chunks
        svuint16_t vcnt = svcnt_u16_z(pg, vxor);
        dist += svaddv_u16(pg, vcnt);
        
        i += vl;
    }
    
    // Handle remaining 16-bit chunks
    if (i < n16) {
        size_t remaining = n16 - i;
        svbool_t pg = svwhilelt_b16((uint32_t)0, (uint32_t)remaining);
        
        svuint16_t va = svld1_u16(pg, a16 + i);
        svuint16_t vb = svld1_u16(pg, b16 + i);
        svuint16_t vxor = sveor_u16_z(pg, va, vb);
        svuint16_t vcnt = svcnt_u16_z(pg, vxor);
        
        dist += svaddv_u16(pg, vcnt);
    }
    
    // Handle odd byte if nbytes is odd
    if (nbytes % 2 == 1) {
        dist += __builtin_popcount(a[nbytes - 1] ^ b[nbytes - 1]);
    }
    
    return dist;
}

#endif // __ARM_FEATURE_SVE

// ============================================================================
// Compilation flags needed for SVE
// ============================================================================

/*
To compile with SVE support:

For GCC:
  g++ -std=c++17 -O3 -march=armv8-a+sve -DUSE_SVE ...

For Clang:
  clang++ -std=c++17 -O3 -march=armv8-a+sve -DUSE_SVE ...

To check if SVE is available at runtime (Linux):
  cat /proc/cpuinfo | grep sve

For specific SVE vector lengths:
  -msve-vector-bits=256   (256-bit vectors)
  -msve-vector-bits=512   (512-bit vectors)
  -msve-vector-bits=1024  (1024-bit vectors)
  -msve-vector-bits=2048  (2048-bit vectors)

Note: SVE supports scalable vectors, so code should work across
different implementations with different vector lengths.
*/

} // namespace hnswlib

// ============================================================================
// Example usage with SVE detection
// ============================================================================

#ifdef INCLUDE_EXAMPLE

#include <iostream>

int main() {
    auto simd = hnswlib::detect_simd();
    
    std::cout << "Detected SIMD: ";
    switch (simd) {
        case hnswlib::SimdKind::AVX512:
            std::cout << "AVX-512\n";
            break;
        case hnswlib::SimdKind::AVX2:
            std::cout << "AVX2\n";
            break;
        case hnswlib::SimdKind::SVE:
            std::cout << "ARM SVE\n";
#ifdef __ARM_FEATURE_SVE
            std::cout << "  Vector length (32-bit): " << svcntw() << " elements\n";
            std::cout << "  Vector length (8-bit): " << svcntb() << " bytes\n";
#endif
            break;
        case hnswlib::SimdKind::NEON:
            std::cout << "ARM NEON (128-bit fixed)\n";
            break;
        default:
            std::cout << "Scalar (no SIMD)\n";
            break;
    }
    
    return 0;
}

#endif // INCLUDE_EXAMPLE
