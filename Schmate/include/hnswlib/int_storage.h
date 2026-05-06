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
#ifdef NEON
            pack_int4_to_neon(emb, out, dim);
#elif defined(AVX2)
            pack_int4_to_avx2(emb, out, dim);
#else
            pack_int4_to(emb, out, dim);
#endif
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
            for (size_t i = 0; i < dim; ++i) {
		// Cast emb[i] to float explicitly to handle T=double
                float val = static_cast<float>(emb[i]);
    #if defined(HAS_AVX512FP16)
                out16[i] = _cvtss_sh(val, 0);
    #elif defined(HAS_NEON_FP16)
                out16[i] = vcvt_f16_f32(vdup_n_f32(val))[0];
    #else
                out16[i] = float_to_half_bits(val);
    #endif
	    }
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
#if 1
          if constexpr (std::is_same_v<T, float>) {
             std::memcpy(out, emb, dim * sizeof(float));
          } else {
             float* out_f32 = reinterpret_cast<float*>(out);
             for (size_t i = 0; i < dim; ++i) out_f32[i] = static_cast<float>(emb[i]);
         }
#else
            std::memcpy(out, emb, dim * sizeof(float));
#endif
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



#if 1


template<typename T>
static void pack_int4_to(const T* src, uint8_t* out, size_t dim) {
    const float SCALE = 64.0f; 

    for (size_t i = 0; i < dim; i += 2) {
        auto q = [&](float val) {
            // SBERT elements are small; let's use the full signed range
            int8_t v = (int8_t)std::round(val * SCALE);
            return (int)std::max(-8, std::min(7, (int)v));
        };
        int q0 = q(src[i]);
        int q1 = (i + 1 < dim) ? q(src[i + 1]) : 0;
        
        // Store as two 4-bit signed nibbles in one byte
        out[i / 2] = (uint8_t)((q1 & 0x0F) << 4) | (uint8_t)(q0 & 0x0F);
    }
}

#elif

template<typename T>
static void pack_int4_to(const T* src, uint8_t* out, size_t dim) {
    // Try a higher scale. 100.0f is safer for 384-dim.
    const float SCALE = 100.0f; 

    for (size_t i = 0; i < dim; i += 2) {
        auto quantize = [&](float val) {
            // Force explicit rounding to nearest integer
            int q = static_cast<int>(std::floor(val * SCALE + 0.5f));
            return std::clamp(q, -8, 7);
        };

        int q0 = quantize(static_cast<float>(src[i]));
        int q1 = (i + 1 < dim) ? quantize(static_cast<float>(src[i+1])) : 0;

        // Use bit masking to ensure only 4 bits are stored
        uint8_t nibble0 = static_cast<uint8_t>(q0) & 0x0F;
        uint8_t nibble1 = static_cast<uint8_t>(q1) & 0x0F;
        
        out[i / 2] = (nibble1 << 4) | nibble0;
    }
}


#elif 0
template<typename T>
static void pack_int4_to(const T* src, uint8_t* out, size_t dim) {
    // 127 is the max scale to keep values within -8 to 7 
    // for standard SBERT distributions.
    const float FIXED_SCALE = 127.0f; 

    for (size_t i = 0; i < dim; i += 2) {
        auto q = [&](T val) {
            float v = static_cast<float>(val) * FIXED_SCALE;
            return std::clamp((int)std::round(v), -8, 7);
        };
        
        int q0 = q(src[i]);
        int q1 = (i + 1 < dim) ? q(src[i + 1]) : 0;
        
        // Ensure q0 and q1 are treated as 4-bit nibbles
        out[i / 2] = (uint8_t)((q1 & 0x0F) << 4) | (uint8_t)(q0 & 0x0F);
    }
}


#elif 0
template<typename T>
static void pack_int4_to(const T* src, uint8_t* out, size_t dim) {
    // A scale of 100.0 means the range [-0.08, 0.07] maps to [-8, 7]
    // Adjust this based on your specific BERT model's distribution
    const float FIXED_SCALE = 100.0f; 

    size_t idx = 0;
    for (size_t i = 0; i < dim; i += 2) {
        auto q = [&](T val) {
            float scaled = static_cast<float>(val) * FIXED_SCALE;
            return std::clamp((int)std::round(scaled), -8, 7);
        };
        
        int q0 = q(src[i]);
        int q1 = (i + 1 < dim) ? q(src[i + 1]) : 0;
        
        // Store as raw nibbles
        out[idx++] = (uint8_t)((q1 & 0x0F) << 4) | (uint8_t)(q0 & 0x0F);
    }
}


#elif 0

template<typename T>
static void pack_int4_to(const T* src, uint8_t* out, size_t dim) {
    // SBERT values are small. Let's map -0.12 to 0 and +0.12 to 15.
    // This uses the full 4-bit spectrum.
    const float SCALE = 60.0f; 
    const int OFFSET = 8; // Center point

    size_t idx = 0;
    for (size_t i = 0; i < dim; i += 2) {
        auto q = [&](T val) {
            // Transform float to 0-15 range
            int quantized = std::round(static_cast<float>(val) * SCALE + OFFSET);
            return std::clamp(quantized, 0, 15);
        };
        
        int q0 = q(src[i]);
        int q1 = (i + 1 < dim) ? q(src[i + 1]) : 8; // Default to neutral offset
        out[idx++] = (uint8_t)((q1 << 4) | (q0 & 0x0F));
    }
}


#elif 0
template<typename T>
static void pack_int4_to(const T* src, uint8_t* out, size_t dim) {
    // SBERT vectors have a max value around 0.1 - 0.2.
    // 64.0 * 0.11 = 7 (the max for INT4). 
    const float FIXED_SCALE = 64.0f; 

    size_t idx = 0;
    for (size_t i = 0; i < dim; i += 2) {
        auto q = [&](T val) {
            return std::clamp(int(std::round(static_cast<float>(val) * FIXED_SCALE)), -8, 7);
        };
        int q0 = q(src[i]);
        int q1 = (i + 1 < dim) ? q(src[i + 1]) : 0;
        out[idx++] = (uint8_t)((q1 & 0x0F) << 4) | (uint8_t)(q0 & 0x0F);
    }
}



#elif 0

template<typename T>
static void pack_int4_to(const T* src, uint8_t* out, size_t dim) {
    // We need to find the multiplier that turns these small floats
    // back into the -8...7 range.
    float max_abs = 0.00001f;
    for(size_t i = 0; i < dim; ++i) {
        max_abs = std::max(max_abs, std::abs(float(src[i])));
    }
    
    // Scale so the largest value hits 7 (the INT4 max)
    float multiplier = 7.0f / max_abs;

    size_t idx = 0;
    for (size_t i = 0; i < dim; i += 2) {
        auto q = [&](T val) {
            return std::clamp(int(std::round(float(val) * multiplier)), -8, 7);
        };
        int q0 = q(src[i]);
        int q1 = (i + 1 < dim) ? q(src[i + 1]) : 0;
        out[idx++] = (uint8_t)((q1 & 0x0F) << 4) | (uint8_t)(q0 & 0x0F);
    }
}



#elif 0


template<typename T>
static void pack_int4_to(const T* src, uint8_t* out, size_t dim) {
    // 1. Find the max absolute value in THIS vector
    float max_v = 1e-6f; 
    for(size_t i = 0; i < dim; ++i) {
        float abs_v = std::abs(static_cast<float>(src[i]));
        if (abs_v > max_v) max_v = abs_v;
    }

    // 2. Calculate the scale to stretch the max value to 7
    float scale = 7.0f / max_v;

    // 3. Pack
    size_t idx = 0;
    for (size_t i = 0; i < dim; i += 2) {
        auto q = [&](T val) {
            return std::clamp(int(std::round(static_cast<float>(val) * scale)), -8, 7);
        };
        int q0 = q(src[i]);
        int q1 = (i + 1 < dim) ? q(src[i + 1]) : 0;
        out[idx++] = (uint8_t)((q1 & 0x0F) << 4) | (uint8_t)(q0 & 0x0F);
    }
}



#elif 0

template<typename T>
static void pack_int4_to(const T* src, uint8_t* out, size_t dim) {
    // SBERT values are typically small. Let's use a scale that maps
    // -0.25 to -8 and +0.22 to +7.
    const float scale = 32.0f; 

    size_t idx = 0;
    for (size_t i = 0; i < dim; i += 2) {
        // Helper to map float -> signed 4-bit (-8 to 7)
        auto q = [&](T val) {
            float v = static_cast<float>(val) * scale;
            return std::clamp(int(std::round(v)), -8, 7);
        };

        int q0 = q(src[i]);
        int q1 = (i + 1 < dim) ? q(src[i + 1]) : 0;

        // Use 0x0F mask to ensure only the low 4 bits are kept before shifting
        out[idx++] = (uint8_t)((q1 & 0x0F) << 4) | (uint8_t)(q0 & 0x0F);
    }
}

#elif 0

// Need to dynamically scale since we were jsquashing a high-precision normalized
// vector into just two possible values (0 and 1), losing almost all the information
template<typename T>
static void pack_int4_to(const T* src, uint8_t* out, size_t dim) {
    // 1. Find the range (or use a pre-computed scale)
    // For SBERT, values are small, so let's map -0.2 to 0.2 -> 0 to 15
    float scale = 7.0f / 0.2f; 
    // float scale = 15.0f / 0.4f; 
    float offset = 0.2f;

    size_t idx = 0;
    for (size_t i = 0; i < dim; i += 2) {
        auto quantize = [&](T val) {
            float normalized = (static_cast<float>(val) + offset) * scale;
            return std::clamp(int(std::round(normalized)), 0, 15);
        };

        int q0 = std::clamp(int(std::round(src[i] * scale)), -8, 7);
        // int q0 = quantize(src[i]);
        int q1 = (i + 1 < dim) ? quantize(src[i + 1]) : 0;
        out[idx++] = uint8_t((q1 << 4) | (q0 & 0x0F));
    }
}


#else

template<typename T>
static void pack_int4_to(const T* src, uint8_t* out, size_t dim) {
    size_t idx = 0;
    for (size_t i = 0; i < dim; i += 2) {
        int q0 = std::clamp(int(std::round(src[i])), 0, 15);
        int q1 = (i + 1 < dim) ? std::clamp(int(std::round(src[i + 1])), 0, 15) : 0;
        out[idx++] = uint8_t((q1 << 4) | (q0 & 0x0F));
    }
}

#endif

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


#if defined(__aarch64__) && (defined(__ARM_NEON) || defined(__ARM_NEON__))

template<typename T>
static void pack_int4_neon(const T* src, uint8_t* out, size_t dim) {
    float32x4_t vscale = vdupq_n_f32(64.0f);
    int32x4_t vmin = vdupq_n_s32(-8);
    int32x4_t vmax = vdupq_n_s32(7);
    
    size_t i = 0;
    for (; i + 8 <= dim; i += 8) {
        // Load 8 floats
        float32x4_t f0 = vld1q_f32((const float*)src + i);
        float32x4_t f1 = vld1q_f32((const float*)src + i + 4);

        // Scale and Round
        int32x4_t i0 = vcvtnq_s32_f32(vmulq_f32(f0, vscale));
        int32x4_t i1 = vcvtnq_s32_f32(vmulq_f32(f1, vscale));

        // Clamp/Saturate to signed 4-bit range (-8 to 7)
        i0 = vmaxq_s32(vmin, vminq_s32(vmax, i0));
        i1 = vmaxq_s32(vmin, vminq_s32(vmax, i1));

        // Narrow: 32-bit -> 16-bit -> 8-bit
        int16x4_t n0 = vmovn_s32(i0);
        int16x4_t n1 = vmovn_s32(i1);
        int8x8_t combined8 = vcombine_s8(vmovn_s16(vcombine_s16(n0, n0)), 
                                         vmovn_s16(vcombine_s16(n1, n1))); 
        
        // At this point, combined8 has 8 signed bytes. 
        // We need to pack nibbles: Low nibbles from indices 0,2,4,6 and High from 1,3,5,7
        // A simpler way:
        uint8_t raw[8];
        vst1_u8(raw, vreinterpret_u8_s8(combined8));
        
        out[i/2]   = (raw[0] & 0x0F) | (raw[1] << 4);
        out[i/2+1] = (raw[2] & 0x0F) | (raw[3] << 4);
        out[i/2+2] = (raw[4] & 0x0F) | (raw[5] << 4);
        out[i/2+3] = (raw[6] & 0x0F) | (raw[7] << 4);
    }
    // ... scalar fallback for remainder ...
}
#endif

#ifdef AVX

template<typename T>
static void pack_int4_avx2(const T* src, uint8_t* out, size_t dim) {
    __m256 vscale = _mm256_set1_ps(64.0f);
    
    size_t i = 0;
    for (; i + 16 <= dim; i += 16) {
        // Load and Scale
        __m256 f0 = _mm256_loadu_ps((const float*)src + i);
        __m256 f1 = _mm256_loadu_ps((const float*)src + i + 8);
        
        // Convert to Int32 with rounding
        __m256i i0 = _mm256_cvtps_epi32(_mm256_mul_ps(f0, vscale));
        __m256i i1 = _mm256_cvtps_epi32(_mm256_mul_ps(f1, vscale));

        // Pack 32-bit -> 16-bit (Saturating)
        __m256i p16 = _mm256_packs_epi32(i0, i1);
        
        // Pack 16-bit -> 8-bit (Saturating)
        __m256i p8 = _mm256_packs_epi16(p16, p16); 

        // Fix AVX2 lane permutation and Extract low 128-bit
        __m128i final8 = _mm_castps_si128(_mm256_extractf128_ps(_mm256_castsi256_ps(_mm256_permute4x64_epi64(p8, 0xD8)), 0));

        // Mask and Shift to create nibbles
        alignas(16) int8_t raw[16];
        _mm_storeu_si128((__m128i*)raw, final8);

        for(int j=0; j<8; ++j) {
            out[(i/2) + j] = (raw[j*2] & 0x0F) | (raw[j*2+1] << 4);
        }
    }
    // ... scalar fallback ...
}

#endif /* AVX */

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
