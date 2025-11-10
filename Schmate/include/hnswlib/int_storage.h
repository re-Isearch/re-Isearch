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
    BIN1, INT2, INT4, INT8, INT16, FP16, FLOAT32
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
        size_t bits = bits_per_element();
        data.resize((dim * bits + 7) / 8);
    }

    size_t bits_per_element() const noexcept {
        switch (type) {
            case StorageType::BIN1: return 1;
            case StorageType::INT2: return 2;
            case StorageType::INT4: return 4;
            case StorageType::INT8: return 8;
            case StorageType::INT16: return 16;
            case StorageType::FP16: return 16;
            case StorageType::FLOAT32: return 32;
        }
        return 0;
    }

    size_t bytes_per_vector() const noexcept {
        return (dim * bits_per_element() + 7) / 8;
    }

    // ---------------------------------------------------------------
    // Main API: pack/unpack
    // ---------------------------------------------------------------
    void pack(const float *src);
    void unpack(float *dst) const;

    // convenience overloads
    void pack(const std::vector<float> &v) { pack(v.data()); }
    void unpack(std::vector<float> &v) const { v.resize(dim); unpack(v.data()); }

//---------------------------------------------------------------------
// External quantize interface (passthrough pack)
//---------------------------------------------------------------------
template<typename T> void quantize(const T* emb, uint8_t* out) const {
    switch (type) {
        case StorageType::BIN1:
            pack_bin1_to(emb, out);
            break;
        case StorageType::INT2:
            pack_int2_to(emb, out);
            break;
        case StorageType::INT4:
            pack_int4_to(emb, out);
            break;
        case StorageType::INT8:
            for (size_t i = 0; i < dim; ++i)
                out[i] = uint8_t(int(std::round(emb[i])) & 0xFF);
            break;
        case StorageType::INT16: {
            auto* out16 = reinterpret_cast<uint16_t*>(out);
            for (size_t i = 0; i < dim; ++i)
                out16[i] = uint16_t(int(std::round(emb[i])) & 0xFFFF);
            break;
        }
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
        case StorageType::FLOAT32:
            std::memcpy(out, emb, dim * sizeof(float));
            break;
    }
}


// =====================================================================
// Static helpers for one-shot packing/unpacking without object instance
// =====================================================================

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

// Quantize (stateless passthrough bit-packing)
// Same behavior as the per-instance version, but you don’t need an object.
template<typename T>
static void quantize(StorageType type, const T* emb, uint8_t* out, size_t dim) {
    switch (type) {
        case StorageType::BIN1: {
            std::fill(out, out + (dim + 7) / 8, 0);
            for (size_t i = 0; i < dim; ++i)
                if (emb[i] > 0) out[i >> 3] |= uint8_t(1u << (i & 7));
            break;
        }
        case StorageType::INT2: {
            size_t idx = 0;
            for (size_t i = 0; i < dim; i += 4) {
                uint8_t val = 0;
                for (int j = 0; j < 4 && i + j < dim; ++j) {
                    int q = int(std::round(emb[i + j]));
                    q = std::clamp(q, 0, 3);
                    val |= (q & 0x3) << (2 * j);
                }
                out[idx++] = val;
            }
            break;
        }
        case StorageType::INT4: {
            size_t idx = 0;
            for (size_t i = 0; i < dim; i += 2) {
                int q0 = std::clamp(int(std::round(emb[i])), 0, 15);
                int q1 = (i + 1 < dim) ? std::clamp(int(std::round(emb[i + 1])), 0, 15) : 0;
                out[idx++] = uint8_t((q1 << 4) | (q0 & 0x0F));
            }
            break;
        }
        case StorageType::INT8: {
            for (size_t i = 0; i < dim; ++i)
                out[i] = uint8_t(int(std::round(emb[i])) & 0xFF);
            break;
        }
        case StorageType::INT16: {
            auto* out16 = reinterpret_cast<uint16_t*>(out);
            for (size_t i = 0; i < dim; ++i)
                out16[i] = uint16_t(int(std::round(emb[i])) & 0xFFFF);
            break;
        }
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
        case StorageType::FLOAT32: {
            std::memcpy(out, emb, dim * sizeof(float));
            break;
        }
    }
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

    // ---------------------------------------------------------------
    // per-type pack/unpack implementations
    // ---------------------------------------------------------------
    void pack_bin1(const float *src) {
        std::fill(data.begin(), data.end(), 0);
        for (size_t i = 0; i < dim; ++i)
            if (src[i] > 0) data[i >> 3] |= uint8_t(1u << (i & 7));
    }

    void unpack_bin1(float *dst) const {
        for (size_t i = 0; i < dim; ++i)
            dst[i] = (data[i >> 3] >> (i & 7)) & 1 ? 1.f : -1.f;
    }

    void pack_int2(const float *src) {
        size_t idx = 0;
        for (size_t i = 0; i < dim; i += 4) {
            uint8_t out = 0;
            for (int j = 0; j < 4 && i + j < dim; ++j) {
                int q = int(std::round(src[i + j]));
                q = std::clamp(q, 0, 3);
                out |= (q & 0x3) << (2 * j);
            }
            data[idx++] = out;
        }
    }

    void unpack_int2(float *dst) const {
        size_t idx = 0;
        for (size_t i = 0; i < dim; i += 4) {
            uint8_t in = data[idx++];
            for (int j = 0; j < 4 && i + j < dim; ++j)
                dst[i + j] = float((in >> (2 * j)) & 0x3);
        }
    }

    void pack_int4(const float *src) {
        size_t idx = 0;
        for (size_t i = 0; i < dim; i += 2) {
            int q0 = std::clamp(int(std::round(src[i])), 0, 15);
            int q1 = (i + 1 < dim) ? std::clamp(int(std::round(src[i + 1])), 0, 15) : 0;
            data[idx++] = uint8_t((q1 << 4) | (q0 & 0x0F));
        }
    }

    void unpack_int4(float *dst) const {
        size_t idx = 0;
        for (size_t i = 0; i < dim; i += 2) {
            uint8_t in = data[idx++];
            dst[i] = float(in & 0x0F);
            if (i + 1 < dim) dst[i + 1] = float(in >> 4);
        }
    }

    void pack_int8(const float *src) {
        for (size_t i = 0; i < dim; ++i)
            data[i] = uint8_t(int(std::round(src[i])) & 0xFF);
    }

    void unpack_int8(float *dst) const {
        for (size_t i = 0; i < dim; ++i)
            dst[i] = float(int8_t(data[i]));
    }

    void pack_int16(const float *src) {
        auto *out16 = reinterpret_cast<uint16_t*>(data.data());
        for (size_t i = 0; i < dim; ++i)
            out16[i] = uint16_t(int(std::round(src[i])) & 0xFFFF);
    }

    void unpack_int16(float *dst) const {
        const uint16_t *in16 = reinterpret_cast<const uint16_t*>(data.data());
        for (size_t i = 0; i < dim; ++i)
            dst[i] = float(int16_t(in16[i]));
    }

    void pack_fp16(const float *src) {
        auto *out16 = reinterpret_cast<uint16_t*>(data.data());
    #if defined(HAS_AVX512FP16)
        for (size_t i = 0; i < dim; ++i)
            out16[i] = _cvtss_sh(src[i], 0);
    #elif defined(HAS_NEON_FP16)
        for (size_t i = 0; i < dim; ++i)
            out16[i] = vcvt_f16_f32(vdup_n_f32(src[i]))[0];
    #else
        for (size_t i = 0; i < dim; ++i)
            out16[i] = float_to_half_bits(src[i]);
    #endif
    }

    void unpack_fp16(float *dst) const {
        const uint16_t *in16 = reinterpret_cast<const uint16_t*>(data.data());
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

    void pack_f32(const float *src) {
        std::memcpy(data.data(), src, dim * sizeof(float));
    }

    void unpack_f32(float *dst) const {
        std::memcpy(dst, data.data(), dim * sizeof(float));
    }

// -------------------------------------------------------------
// Stateless packers that write into external buffer
// -------------------------------------------------------------
template<typename T>
void pack_bin1_to(const T* src, uint8_t* out) const {
    std::fill(out, out + (dim + 7) / 8, 0);
    for (size_t i = 0; i < dim; ++i)
        if (src[i] > 0) out[i >> 3] |= uint8_t(1u << (i & 7));
}

template<typename T>
void pack_int2_to(const T* src, uint8_t* out) const {
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

template<typename T>
void pack_int4_to(const T* src, uint8_t* out) const {
    size_t idx = 0;
    for (size_t i = 0; i < dim; i += 2) {
        int q0 = std::clamp(int(std::round(src[i])), 0, 15);
        int q1 = (i + 1 < dim) ? std::clamp(int(std::round(src[i + 1])), 0, 15) : 0;
        out[idx++] = uint8_t((q1 << 4) | (q0 & 0x0F));
    }
}

#if 0

// ------------------------------------------------------------------
// Dispatchers
// ------------------------------------------------------------------
inline void pack(const float *src) {
    switch (type) {
        case StorageType::BIN1: pack_bin1(src); break;
        case StorageType::INT2: pack_int2(src); break;
        case StorageType::INT4: pack_int4(src); break;
        case StorageType::INT8: pack_int8(src); break;
        case StorageType::INT16: pack_int16(src); break;
        case StorageType::FP16: pack_fp16(src); break;
        case StorageType::FLOAT32: pack_f32(src); break;
    }
}

inline void unpack(float *dst) const {
    switch (type) {
        case StorageType::BIN1: unpack_bin1(dst); break;
        case StorageType::INT2: unpack_int2(dst); break;
        case StorageType::INT4: unpack_int4(dst); break;
        case StorageType::INT8: unpack_int8(dst); break;
        case StorageType::INT16: unpack_int16(dst); break;
        case StorageType::FP16: unpack_fp16(dst); break;
        case StorageType::FLOAT32: unpack_f32(dst); break;
    }
}
#endif

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
