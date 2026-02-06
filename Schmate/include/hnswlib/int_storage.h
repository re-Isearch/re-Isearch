#pragma once
#include <vector>
#include <cstdint>
#include <fstream>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <stdexcept>

#if defined(__AVX512FP16__)
  #include <immintrin.h>
  #define HAS_AVX512FP16 1
#elif defined(__ARM_FEATURE_FP16_VECTOR_ARITHMETIC__) || defined(__ARM_FEATURE_FP16_SCALAR_ARITHMETIC__)
  #include <arm_neon.h>
  #define HAS_NEON_FP16 1
#endif

namespace hnswlib {

// ------------------------------------------------------------------
// Supported formats
// ------------------------------------------------------------------
enum class StorageType : uint32_t {
    BIN1,     // 1-bit
    INT2,     // 2-bit
    INT3,     // 3-bit
    INT4,     // 4-bit
    INT5,     // 5-bit
    INT6,     // 6-bit
    INT8,     // 8-bit
    INT16,    // 16-bit
    INT32,    // 32-bit (NOT SUPPORTED)
    INT64,    // 64-bit (NOT SUPPORTED)
    FP16,     // 16-bit float
    BF16,     // 16-bit float (but 8-bit exponent, 7-bit mantissa, 1-bit sign)
    FLOAT32,  // 32-bit float
    FLOAT64   // 64-bit float (NOT SUPPORTED)
};



// ------------------------------------------------------------------
// IntStorage class
// ------------------------------------------------------------------
class IntStorage {
public:
    StorageType type;
    size_t dim;
    std::vector<uint8_t> data;

    IntStorage(StorageType t = StorageType::FLOAT32, size_t d = 0)
        : type(t), dim(d) { resize_bytes(); }

    // ---------------------------------------------------------------
    // Memory / info helpers
    // ---------------------------------------------------------------
    void resize(size_t new_dim) { dim = new_dim; resize_bytes(); }

    void resize_bytes() {
        data.resize( bytes_per_vector() );
    }

    size_t bits_per_element() const noexcept {
       return bits_per_element(type);
    }

    static size_t bits_per_element(StorageType typ) {
        switch (typ) {
            case StorageType::BIN1: return 1;
            case StorageType::INT2: return 2;
            case StorageType::INT3: return 3;
            case StorageType::INT4: return 4;
            case StorageType::INT5: return 5;
            case StorageType::INT6: return 6;
            case StorageType::INT8: return 8;
            case StorageType::INT16:return 16;
            case StorageType::FP16: return 16;
            case StorageType::BF16: return 16;
            case StorageType::INT32: return 32;
            case StorageType::FLOAT32: return 32;
            case StorageType::INT64: return 64;
            case StorageType::FLOAT64: return 64;
        }
        return 0;
    }

    size_t bytes_per_vector() const noexcept {
        return (dim * bits_per_element() + 7) / 8;
    }

    // ---------------------------------------------------------------
    // Main API: pack/unpack
    // ---------------------------------------------------------------
//    void pack(const float *src) {
//       quantize(type, src, data, dim);
//    }

//    void unpack(float *dst) const {
//      unpack(type, src, data. dim);
//    };

    // convenience overloads
//    void pack(const std::vector<float> &v) { pack(v.data()); }
//    void unpack(std::vector<float> &v) const { v.resize(dim); unpack(v.data()); }

//---------------------------------------------------------------------
// External quantize interface (passthrough pack)
//---------------------------------------------------------------------
// Quantize (stateless passthrough bit-packing)
// Same behavior as the per-instance version, but you don’t need an object.

template<typename T>
static void quantize(StorageType type, const T* emb, uint8_t* out, size_t dim) {
    switch (type) {
        case StorageType::BIN1:
            pack_bin1_to(emb, out, dim);
            break;
        case StorageType::INT2:
            pack_int2_to(emb, out, dim);
            break;
        case StorageType::INT3:
            pack_int3_to(emb, out, dim);
            break;
        case StorageType::INT4:
            pack_int4_to(emb, out, dim);
            break;
        case StorageType::INT5:
            pack_int5_to(emb, out, dim);
            break;
        case StorageType::INT6:
            pack_int6_to(emb, out, dim);
            break;
        case StorageType::INT8:
            pack_int8_to(emb, out, dim);
            break;
        case StorageType::INT16:
            pack_int16_to(emb, out, dim);
            break;
        case StorageType::FP16: {
            auto* out16 = reinterpret_cast<uint16_t*>(out);
    #if defined(HAS_AVX512FP16)
            for (size_t i = 0; i < dim; ++i)
                out16[i] = _cvtss_sh(float(emb[i]), 0);
    #elif defined(HAS_NEON_FP16)
            for (size_t i = 0; i < dim; ++i)
                out16[i] = vcvt_f16_f32(vdup_n_f32(float(emb[i])))[0];
    #else
            for (size_t i = 0; i < dim; ++i)
                out16[i] = float_to_half_bits(float(emb[i]));
    #endif
            break;
        }

       case StorageType::BF16:
#if defined(__AVX512BF16__)
	pack_bf16_avx512bf16(emb, out, dim);
#else
	pack_bf16_to(emb, out, dim);
#endif
	break;

        case StorageType::FLOAT32:
            std::memcpy(out, emb, dim * sizeof(float));
            break;


        case StorageType::INT32:
        case StorageType::INT64:
        case StorageType::FLOAT64:
            std::cerr << "Unsupported Storage Type: " << bits_per_element(type) <<  "-bit\n";
            break;

    }
}


// =====================================================================
// Static helpers for one-shot packing/unpacking without object instance
// =====================================================================


#if 0
    // Generic pack (float→bytes)
    inline static void pack(StorageType type, const float* src, uint8_t* out, size_t dim) {
        IntStorage tmp(type, dim);
        tmp.pack(src);
        std::memcpy(out, tmp.data.data(), tmp.data.size());
    }

    // Generic unpack (bytes→float)
    inline static void unpack(StorageType type, const uint8_t* in, float* dst, size_t dim) {
        IntStorage tmp(type, dim);
        std::memcpy(tmp.data.data(), in, tmp.data.size());
        tmp.unpack(dst);
    }
#endif

    // Quantize (stateless passthrough bit-packing)
    template<typename T> void quantize(const T* emb, uint8_t* out) const {
      quantize(type, emb, out, dim);
    }

    // ---------------------------------------------------------------
    // Save / Load
    // ---------------------------------------------------------------
    void save(const std::string &filename) const {
        std::ofstream out(filename, std::ios::binary);
        if (!out) throw std::runtime_error("Cannot open file for writing");
        save(out);
    }

    void load(const std::string &filename) {
        std::ifstream in(filename, std::ios::binary);
        if (!in) throw std::runtime_error("Cannot open file for reading");
        load(in);
    }

    void save(std::ostream &os) const {
        const uint32_t magic = 0x4F545349; // "ISTO"
        const uint32_t version = 1;
        uint64_t dim64 = dim;
        uint32_t type32 = static_cast<uint32_t>(type);

        os.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
        os.write(reinterpret_cast<const char*>(&version), sizeof(version));
        os.write(reinterpret_cast<const char*>(&type32), sizeof(type32));
        os.write(reinterpret_cast<const char*>(&dim64), sizeof(dim64));
        os.write(reinterpret_cast<const char*>(data.data()), data.size());
    }

    void load(std::istream &is) {
        uint32_t magic = 0, version = 0, type32 = 0;
        uint64_t dim64 = 0;

        is.read(reinterpret_cast<char*>(&magic), sizeof(magic));
        if (magic != 0x4F545349)
            throw std::runtime_error("Bad magic in IntStorage file");
        is.read(reinterpret_cast<char*>(&version), sizeof(version));
        is.read(reinterpret_cast<char*>(&type32), sizeof(type32));
        is.read(reinterpret_cast<char*>(&dim64), sizeof(dim64));

        type = static_cast<StorageType>(type32);
        dim = static_cast<size_t>(dim64);
        resize_bytes();
        is.read(reinterpret_cast<char*>(data.data()), data.size());
    }

private:
    // ---------------------------------------------------------------
    // helpers
    // ---------------------------------------------------------------

    static inline uint16_t float_to_bf16(float f) {
        uint32_t bits;
        std::memcpy(&bits, &f, sizeof(bits));
        return uint16_t(bits >> 16);
    }

    static inline float bf16_to_float(uint16_t b) {
        uint32_t bits = uint32_t(b) << 16;
        float out;
        std::memcpy(&out, &bits, sizeof(out));
        return out;
    }

    static inline uint16_t float_to_half_bits(float f) {
        uint32_t x;
        std::memcpy(&x, &f, sizeof(x));
        uint16_t sign = (x >> 31) & 0x1;
        int32_t exp = ((x >> 23) & 0xFF) - 127 + 15;
        uint32_t mant = x & 0x7FFFFF;
        if (exp <= 0) return sign << 15;
        if (exp >= 31) return (sign << 15) | 0x7C00;
        return (sign << 15) | (exp << 10) | (mant >> 13);
    }

    static inline float half_bits_to_float(uint16_t h) {
        uint32_t sign = (h >> 15) & 0x1;
        uint32_t exp = (h >> 10) & 0x1F;
        uint32_t mant = h & 0x3FF;
        uint32_t f;
        if (exp == 0)
            f = mant << 13;
        else if (exp == 31)
            f = 0x7F800000 | (mant << 13);
        else
            f = ((exp - 15 + 127) << 23) | (mant << 13);
        f |= sign << 31;
        float result;
        std::memcpy(&result, &f, sizeof(result));
        return result;
    }

// -------------------------------------------------------------
// Stateless packers that write into external buffer
// -------------------------------------------------------------
template<typename T>
static void pack_bin1_to(const T* src, uint8_t* out, size_t dim) {
    std::fill(out, out + (dim + 7) / 8, 0);
    for (size_t i = 0; i < dim; ++i)
        if (src[i] > 0) out[i >> 3] |= uint8_t(1u << (i & 7));
}

inline void unpack_bin1_from(const uint8_t* in, float* dst, size_t dim) const {
    for (size_t i = 0; i < dim; ++i) {
        uint8_t byte = in[i >> 3];
        uint8_t bit  = (byte >> (i & 7)) & 1;
        dst[i] = float(bit);      // 0 or 1
    }
}

template<typename T>
static void pack_int2_to(const T* src, uint8_t* out, size_t dim) {
    size_t idx = 0;
    for (size_t i = 0; i < dim; i += 4) {
        uint8_t val = 0;
        for (int j = 0; j < 4 && i + j < dim; ++j) {
            int q = int(std::round(src[i + j]));
            q = std::clamp(q, 0, 3);
            val |= (q & 0x3) << (2 * j);
        }
        out[idx++] = val;
    }
}

inline void unpack_int2_from(const uint8_t* in, float* dst, size_t dim) const {
    size_t bytepos = 0;
    for (size_t i = 0; i < dim; i += 4) {
        uint8_t b = in[bytepos++];
        for (int j = 0; j < 4 && (i + j) < dim; ++j) {
            uint8_t v = (b >> (2 * j)) & 0x03;
            dst[i + j] = float(v);
        }
    }
}


template<typename T>
static void pack_int3_to(const T* src, uint8_t* out, size_t dim) {
    size_t bitpos = 0;
    size_t bytepos = 0;
    uint8_t cur = 0;

    for (size_t i = 0; i < dim; ++i) {
        int q = std::clamp(int(std::round(src[i])), 0, 7);

        // place into current byte
        cur |= (q & 0x07) << bitpos;
        bitpos += 3;

        if (bitpos >= 8) {
            out[bytepos++] = cur;
            bitpos -= 8;

            // carry leftover bits into the next byte
            cur = (q >> (3 - bitpos)) & ((1u << bitpos) - 1);
        }
    }
    if (bitpos > 0) out[bytepos++] = cur;
}

inline void unpack_int3_from(const uint8_t* in, float* dst, size_t dim) const {
    size_t bitpos = 0;
    size_t bytepos = 0;
    uint8_t cur = in[0];

    for (size_t i = 0; i < dim; ++i) {
        uint32_t v = (cur >> bitpos) & 0x07;
        bitpos += 3;

        if (bitpos >= 8) {
            bytepos++;
            cur = in[bytepos];
            bitpos -= 8;

            if (bitpos)
                v |= (cur & ((1u << bitpos) - 1)) << (3 - bitpos);
        }
        dst[i] = float(v);
    }
}



template<typename T>
static void pack_int4_to(const T* src, uint8_t* out, size_t dim) {
    size_t idx = 0;
    for (size_t i = 0; i < dim; i += 2) {
        int q0 = std::clamp(int(std::round(src[i])), 0, 15);
        int q1 = (i + 1 < dim) ? std::clamp(int(std::round(src[i + 1])), 0, 15) : 0;
        out[idx++] = uint8_t((q1 << 4) | (q0 & 0x0F));
    }
}

inline void unpack_int4_from(const uint8_t* in, float* dst, size_t dim) const {
    size_t bytepos = 0;
    for (size_t i = 0; i < dim; i += 2) {
        uint8_t b = in[bytepos++];

        // low nibble
        dst[i] = float(b & 0x0F);

        // high nibble (if exists)
        if (i + 1 < dim)
            dst[i + 1] = float((b >> 4) & 0x0F);
    }
}


template<typename T>
static void pack_int5_to(const T* src, uint8_t* out, size_t dim) {
    size_t bitpos = 0;
    size_t bytepos = 0;
    uint8_t cur = 0;

    for (size_t i = 0; i < dim; ++i) {
        int q = std::clamp(int(std::round(src[i])), 0, 31);

        cur |= (q & 0x1F) << bitpos;
        bitpos += 5;

        if (bitpos >= 8) {
            out[bytepos++] = cur;
            bitpos -= 8;

            // carry remainder bits
            cur = (q >> (5 - bitpos)) & ((1u << bitpos) - 1);
        }
    }
    if (bitpos > 0) out[bytepos++] = cur;
}

inline void unpack_int5_from(const uint8_t* in, float* dst, size_t dim) const {
    size_t bitpos = 0;
    size_t bytepos = 0;
    uint8_t cur = in[0];

    for (size_t i = 0; i < dim; ++i) {
        uint32_t v = (cur >> bitpos) & 0x1F;
        bitpos += 5;

        if (bitpos >= 8) {
            bytepos++;
            cur = in[bytepos];
            bitpos -= 8;

            if (bitpos)
                v |= (cur & ((1u << bitpos) - 1)) << (5 - bitpos);
        }
        dst[i] = float(v);
    }
}

template<typename T>
static void pack_int6_to(const T* src, uint8_t* out, size_t dim) {
    size_t bitpos = 0;
    size_t bytepos = 0;
    uint8_t cur = 0;

    for (size_t i = 0; i < dim; ++i) {
        int q = std::clamp(int(std::round(src[i])), 0, 63);

        cur |= (q & 0x3F) << bitpos;
        bitpos += 6;

        if (bitpos >= 8) {
            out[bytepos++] = cur;
            bitpos -= 8;

            // remainder
            cur = (q >> (6 - bitpos)) & ((1u << bitpos) - 1);
        }
    }
    if (bitpos > 0) out[bytepos++] = cur;
}


inline void unpack_int6_from(const uint8_t* in, float* dst, size_t dim) const {
    size_t bitpos = 0;
    size_t bytepos = 0;
    uint8_t cur = in[0];

    for (size_t i = 0; i < dim; ++i) {
        uint32_t v = (cur >> bitpos) & 0x3F;
        bitpos += 6;

        if (bitpos >= 8) {
            bytepos++;
            cur = in[bytepos];
            bitpos -= 8;

            if (bitpos)
                v |= (cur & ((1u << bitpos) - 1)) << (6 - bitpos);
        }
        dst[i] = float(v);
    }
}


template<typename T>
static void pack_int8_to(const T* src, uint8_t* out, size_t dim) {
    for (size_t i = 0; i < dim; ++i) {
        int v = int(std::round(src[i]));
        v = std::clamp(v, 0, 255);
        out[i] = uint8_t(v);
    }
}



inline void unpack_int8_from(const uint8_t* in, float* dst, size_t dim) const {
    for (size_t i = 0; i < dim; ++i)
        dst[i] = float(in[i]);
}


template<typename T>
static void pack_int16_to(const T* src, uint8_t* out, size_t dim) {
    uint16_t* p = reinterpret_cast<uint16_t*>(out);
    for (size_t i = 0; i < dim; ++i) {
        int v = int(std::round(src[i]));
        v = std::clamp(v, 0, 65535);
        p[i] = uint16_t(v);
    }
}


inline void unpack_int16_from(const uint8_t* in, float* dst, size_t dim) const {
    const uint16_t* p = reinterpret_cast<const uint16_t*>(in);
    for (size_t i = 0; i < dim; ++i)
        dst[i] = float(p[i]);
}


template<typename T>
static void pack_fp16_to(const T* src, uint8_t* out, size_t dim) {
    uint16_t* p = reinterpret_cast<uint16_t*>(out);
    for (size_t i = 0; i < dim; ++i) {
        float f = float(src[i]);
        p[i] = float_to_half_bits(f);   // your FP16 converter
    }
}

inline void unpack_fp16_from(const uint8_t* in, float* dst, size_t dim) const {
    const uint16_t *in16 = reinterpret_cast<const uint16_t*>(in);
    #if defined(HAS_AVX512FP16)
        for (size_t i = 0; i < dim; ++i) 
            dst[i] = _cvtsh_ss(in16[i]);
    #elif defined(HAS_NEON_FP16)
        for (size_t i = 0; i < dim; ++i)
            dst[i] = vcvt_f32_f16(vdup_n_f16(in16[i]))[0];
    #else
        for (size_t i = 0; i < dim; ++i)
            dst[i] = half_bits_to_float(in16[i]);
    #endif
}   

template<typename T>
static  void pack_bf16_to(const T* src, uint8_t* out, size_t dim) {
    uint16_t* out16 = reinterpret_cast<uint16_t*>(out);
    for (size_t i = 0; i < dim; ++i)
        out16[i] = float_to_bf16(float(src[i]));
}

inline void unpack_bf16_from(const uint8_t* in, float* dst, size_t dim) const {
    const uint16_t* p = reinterpret_cast<const uint16_t*>(in);
    for (size_t i = 0; i < dim; ++i)
        dst[i] = bf16_to_float(p[i]);
}


// SIMD is only availabel on x86. ARM has not yet implemented BF16.
#if defined(__AVX512BF16__)
#include <immintrin.h>

template<typename T>
inline void pack_bf16_avx512bf16(const T* src, uint8_t* out, size_t dim) {
    uint16_t* out16 = reinterpret_cast<uint16_t*>(out);
    size_t i = 0;
    for (; i + 32 <= dim; i += 32) {
        __m512 vf = _mm512_loadu_ps((const float*)(src + i));
        __m256i bf = _mm512_cvtneps_pbh(vf);   // convert 32 fp32 → 32 bf16
        _mm256_storeu_si256((__m256i*)(out16 + i), bf);
    }
    for (; i < dim; ++i)
        out16[i] = float_to_bf16(float(src[i]));
}
#endif




template<typename T>
static void pack_f32_to(const T* src, uint8_t* out, size_t dim) {
    std::memcpy(out, src, dim * sizeof(float));
}


inline void unpack_f32_from(const uint8_t* in, float* dst, size_t dim) const {
    std::memcpy(dst, in, dim * sizeof(float));
}


#if 0 /* SIMD */


// AVX2: float -> unsigned int8 (0..255) pack
template<typename T>
inline void pack_int8_to_avx2(const T* src, uint8_t* out, size_t dim) {
#if defined(__AVX2__)
    size_t i = 0;
    const size_t step = 8; // 8 floats -> 8 bytes after packing
    for (; i + 8 <= dim; i += 8) {
        // load 8 floats into two 256-bit lanes is tricky; use two 256 loads of 8? use 8->8 via 256
        __m256 vf = _mm256_loadu_ps((const float*)(src + i));           // 8 floats
        // convert to int32
        __m256i vi32 = _mm256_cvtps_epi32(vf);                          // 8 x int32
        // pack 32->16: pack signed 32 to signed 16
        __m128i lo_i16 = _mm_packs_epi32(_mm256_castsi256_si128(vi32),
                                         _mm256_extracti128_si256(vi32, 1));
        // pack 16->8 unsigned saturated
        __m128i i8 = _mm_packus_epi16(lo_i16, _mm_setzero_si128());
        // store lower 8 bytes
        _mm_storel_epi64((__m128i*)(out + i), i8);
    }
    // tail
    for (; i < dim; ++i) {
        int v = int(std::round(float(src[i])));
        v = std::clamp(v, 0, 255);
        out[i] = uint8_t(v);
    }
#else
    // fallback
    for (size_t i = 0; i < dim; ++i) {
        int v = int(std::round(float(src[i])));
        v = std::clamp(v, 0, 255);
        out[i] = uint8_t(v);
    }
#endif
}

// AVX2: float -> uint16_t pack (little-endian, 2 bytes per value)
template<typename T>
inline void pack_int16_to_avx2(const T* src, uint8_t* out, size_t dim) {
#if defined(__AVX2__)
    size_t i = 0;
    uint16_t* out16 = reinterpret_cast<uint16_t*>(out);
    const size_t step = 8; // process 8 floats -> 8 x int16 in a loop
    for (; i + step <= dim; i += step) {
        __m256 vf = _mm256_loadu_ps((const float*)(src + i));     // 8 floats
        __m256i vi32 = _mm256_cvtps_epi32(vf);                    // 8 x int32
        // pack to 16-bit signed (saturating)
        __m128i packed16 = _mm_packs_epi32(_mm256_castsi256_si128(vi32),
                                           _mm256_extracti128_si256(vi32, 1)); // 8 x int16
        // store 8 int16
        _mm_storeu_si128((__m128i*)(out16 + i), packed16);
    }
    for (; i < dim; ++i) {
        int v = int(std::round(float(src[i])));
        v = std::clamp(v, 0, 65535);
        out16[i] = uint16_t(v);
    }
#else
    uint16_t* out16 = reinterpret_cast<uint16_t*>(out);
    for (size_t i = 0; i < dim; ++i) {
        int v = int(std::round(float(src[i])));
        v = std::clamp(v, 0, 65535);
        out16[i] = uint16_t(v);
    }
#endif
}

template<typename T>
inline void pack_int8_to_neon(const T* src, uint8_t* out, size_t dim) {
#if defined(__aarch64__) && (defined(__ARM_NEON) || defined(__ARM_NEON__))
    size_t i = 0;
    for (; i + 4 <= dim; i += 4) {
        float32x4_t vf = vld1q_f32((const float*)(src + i));
        int32x4_t vi = vcvtq_s32_f32(vf);          // round toward zero; if you want nearest, use vrndnq_f32 then vcvtq_s32_f32
        // narrow to 16 then to 8 (saturating)
        int16x4_t v16 = vmovn_s32(vi);             // lower 4 lanes into 16-bit
        uint8x8_t v8 = vmovn_u16(vreinterpret_u16_s16(vcombine_s16(v16, v16))); // pack lower half and ignore upper
        // store first 4 bytes (v8's low 4 lanes)
        vst1_lane_u32(reinterpret_cast<uint32_t*>(out + i), vreinterpret_u32_u8(v8), 0);
    }
    for (; i < dim; ++i) {
        int v = int(std::round(float(src[i])));
        v = std::clamp(v, 0, 255);
        out[i] = uint8_t(v);
    }
#else
    for (size_t i = 0; i < dim; ++i) {
        int v = int(std::round(float(src[i])));
        v = std::clamp(v, 0, 255);
        out[i] = uint8_t(v);
    }
#endif
}

template<typename T>
inline void pack_int16_to_neon(const T* src, uint8_t* out, size_t dim) {
#if defined(__aarch64__) && (defined(__ARM_NEON) || defined(__ARM_NEON__))
    uint16_t* out16 = reinterpret_cast<uint16_t*>(out);
    size_t i = 0;
    for (; i + 4 <= dim; i += 4) {
        float32x4_t vf = vld1q_f32((const float*)(src + i));
        int32x4_t vi = vcvtq_s32_f32(vf);
        int16x4_t v16 = vmovn_s32(vi); // saturating narrow
        vst1_s16(reinterpret_cast<int16_t*>(out16 + i), v16);
    }
    for (; i < dim; ++i) {
        int v = int(std::round(float(src[i])));
        v = std::clamp(v, 0, 65535);
        out16[i] = uint16_t(v);
    }
#else
    uint16_t* out16 = reinterpret_cast<uint16_t*>(out);
    for (size_t i = 0; i < dim; ++i) {
        int v = int(std::round(float(src[i])));
        v = std::clamp(v, 0, 65535);
        out16[i] = uint16_t(v);
    }
#endif
}

// scalar fallback already exists float_to_half_bits()
// NEON vector FP16 (aarch64 with FP16)
template<typename T>
inline void pack_fp16_neon(const T* src, uint8_t* out, size_t dim) {
#if defined(__aarch64__) && defined(__ARM_FEATURE_FP16_VECTOR_ARITHMETIC__)
    uint16_t* out16 = reinterpret_cast<uint16_t*>(out);
    size_t i = 0;
    for (; i + 4 <= dim; i += 4) {
        float32x4_t vf = vld1q_f32((const float*)(src + i));
        float16x4_t vh = vcvt_f16_f32(vf);
        vst1_f16(reinterpret_cast<float16_t*>(out16 + i), vh);
    }
    for (; i < dim; ++i) out16[i] = float_to_half_bits(float(src[i]));
#else
    for (size_t i = 0; i < dim; ++i) out[i*2] = out[i*2]; // use scalar fallback below
#endif
}

#if defined(__AVX512FP16__)
// AVX-512-FP16 path (if your toolchain and CPU support it)
template<typename T>
inline void pack_fp16_avx512(const T* src, uint8_t* out, size_t dim) {
    uint16_t* out16 = reinterpret_cast<uint16_t*>(out);
    size_t i = 0;
    for (; i + 16 <= dim; i += 16) {
        __m512 vf = _mm512_loadu_ps((const float*)(src + i)); // 16 floats
        __m256i ph = _mm512_cvtps_ph(vf, 0);                  // convert to 16 half (packed in 256 bits)
        _mm256_storeu_si256((__m256i*)(out16 + i), ph);
    }
    for (; i < dim; ++i) out16[i] = float_to_half_bits(float(src[i]));
}
#endif

// Generic static pack_fp16_to dispatcher (callable)
template<typename T>
inline void pack_fp16_to(const T* src, uint8_t* out, size_t dim) {
#if defined(__AVX512FP16__)
    pack_fp16_avx512(src, out, dim);
#elif defined(__aarch64__) && defined(__ARM_FEATURE_FP16_VECTOR_ARITHMETIC__)
    pack_fp16_neon(src, out, dim);
#else
    // scalar fallback: call float_to_half_bits
    uint16_t* out16 = reinterpret_cast<uint16_t*>(out);
    for (size_t i = 0; i < dim; ++i) out16[i] = float_to_half_bits(float(src[i]));
#endif
}

#endif /* SIMD */

}; // Class end

} // namespace hnswlib


/*

#include "hnswlib/int_storage.h"
#include <iostream>
using namespace hnswlib;

int main() {
    std::vector<float> vals = {1, 2, 3, 4, 5, 6, 7, 8};
    IntStorage s(StorageType::INT4, vals.size());
    s.pack(vals);
    s.save("vec.int4");

    IntStorage t;
    t.load("vec.int4");
    std::vector<float> out;
    t.unpack(out);

    for (auto v : out) std::cout << v << " ";
    std::cout << "\n";
}


*/
