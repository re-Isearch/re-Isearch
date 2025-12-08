#pragma once

#include "Logger.hpp"
#include <hnswlib/hnswlib.h>
#include <vector>
#include <memory>
#include <cstring>
#include <algorithm>
#include <cmath>

#include "HnswConfig.hpp"

namespace hnswlib {

// ============================================
// BINARY QUANTIZER (1-bit)
// ============================================

class BinaryQuantizer {
public:
    static std::vector<uint64_t> quantize(const float* data, size_t dim) {
        size_t num_chunks = (dim + 63) / 64;
        std::vector<uint64_t> binary(num_chunks, 0);
        
        for (size_t i = 0; i < dim; i++) {
            if (data[i] > 0.0f) {
                binary[i / 64] |= (1ULL << (i % 64));
            }
        }
        return binary;
    }
    
    static std::vector<uint64_t> quantize_median(const float* data, size_t dim) {
        std::vector<float> sorted(data, data + dim);
        std::nth_element(sorted.begin(), sorted.begin() + dim/2, sorted.end());
        float median = sorted[dim/2];
        
        size_t num_chunks = (dim + 63) / 64;
        std::vector<uint64_t> binary(num_chunks, 0);
        
        for (size_t i = 0; i < dim; i++) {
            if (data[i] > median) {
                binary[i / 64] |= (1ULL << (i % 64));
            }
        }
        return binary;
    }
};

// ============================================
// TERNARY QUANTIZER (1.58-bit)
// ============================================

class TernaryQuantizer {
public:
    // Quantize to {-1, 0, +1}, stored as 2 bits per value
    static std::vector<uint8_t> quantize(const float* data, size_t dim) {
        size_t num_bytes = (dim + 3) / 4;
        std::vector<uint8_t> ternary(num_bytes, 0);
        
        for (size_t i = 0; i < dim; i++) {
            int8_t tval;
            if (data[i] > 0.5f) {
                tval = 1;
            } else if (data[i] < -0.5f) {
                tval = -1;
            } else {
                tval = 0;
            }
            
            size_t byte_idx = i / 4;
            size_t bit_offset = (i % 4) * 2;
            
            if (tval == 1) {
                ternary[byte_idx] |= (0x01 << bit_offset);
            } else if (tval == -1) {
                ternary[byte_idx] |= (0x02 << bit_offset);
            }
        }
        
        return ternary;
    }
    
    static std::vector<uint8_t> quantize_adaptive(const float* data, size_t dim) {
        float mean = 0.0f;
        for (size_t i = 0; i < dim; i++) {
            mean += data[i];
        }
        mean /= dim;
        
        float std_dev = 0.0f;
        for (size_t i = 0; i < dim; i++) {
            float diff = data[i] - mean;
            std_dev += diff * diff;
        }
        std_dev = std::sqrt(std_dev / dim);
        
        float pos_threshold = mean + 0.5f * std_dev;
        float neg_threshold = mean - 0.5f * std_dev;
        
        size_t num_bytes = (dim + 3) / 4;
        std::vector<uint8_t> ternary(num_bytes, 0);
        
        for (size_t i = 0; i < dim; i++) {
            int8_t tval;
            if (data[i] > pos_threshold) {
                tval = 1;
            } else if (data[i] < neg_threshold) {
                tval = -1;
            } else {
                tval = 0;
            }
            
            size_t byte_idx = i / 4;
            size_t bit_offset = (i % 4) * 2;
            
            if (tval == 1) {
                ternary[byte_idx] |= (0x01 << bit_offset);
            } else if (tval == -1) {
                ternary[byte_idx] |= (0x02 << bit_offset);
            }
        }
        
        return ternary;
    }
    
    static int8_t get_value(const uint8_t* data, size_t index) {
        size_t byte_idx = index / 4;
        size_t bit_offset = (index % 4) * 2;
        uint8_t bits = (data[byte_idx] >> bit_offset) & 0x03;
        
        if (bits == 0x01) return 1;
        if (bits == 0x02) return -1;
        return 0;
    }
};

// ============================================
// SIMD HEADERS
// ============================================

#ifdef __AVX2__
#include <immintrin.h>
#endif

#ifdef __AVX512F__
#include <immintrin.h>
#endif

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

// ============================================
// HAMMING DISTANCE (for Binary)
// ============================================

class HammingDistance {
public:
    static size_t compute(const uint64_t* a, const uint64_t* b, size_t num_chunks) {
        size_t dist = 0;
        for (size_t i = 0; i < num_chunks; i++) {
            dist += __builtin_popcountll(a[i] ^ b[i]);
        }
        return dist;
    }
    
#ifdef __ARM_NEON
    static size_t compute_neon(const uint64_t* a, const uint64_t* b, size_t num_chunks) {
        size_t dist = 0;
        size_t i = 0;
        
        for (; i + 1 < num_chunks; i += 2) {
            uint64x2_t va = vld1q_u64(a + i);
            uint64x2_t vb = vld1q_u64(b + i);
            uint64x2_t vxor = veorq_u64(va, vb);
            uint8x16_t vxor_8 = vreinterpretq_u8_u64(vxor);
            uint8x16_t vcnt = vcntq_u8(vxor_8);
            uint64x2_t vcnt_64 = vpaddlq_u32(vpaddlq_u16(vpaddlq_u8(vcnt)));
            dist += vgetq_lane_u64(vcnt_64, 0) + vgetq_lane_u64(vcnt_64, 1);
        }
        
        for (; i < num_chunks; i++) {
            dist += __builtin_popcountll(a[i] ^ b[i]);
        }
        return dist;
    }
#endif

#ifdef __AVX2__
    static size_t compute_avx2(const uint64_t* a, const uint64_t* b, size_t num_chunks) {
        size_t dist = 0;
        size_t i = 0;
        
        for (; i + 3 < num_chunks; i += 4) {
            __m256i va = _mm256_loadu_si256((__m256i*)(a + i));
            __m256i vb = _mm256_loadu_si256((__m256i*)(b + i));
            __m256i vxor = _mm256_xor_si256(va, vb);
            
            uint64_t temp[4];
            _mm256_storeu_si256((__m256i*)temp, vxor);
            for (int j = 0; j < 4; j++) {
                dist += __builtin_popcountll(temp[j]);
            }
        }
        
        for (; i < num_chunks; i++) {
            dist += __builtin_popcountll(a[i] ^ b[i]);
        }
        return dist;
    }
#endif

#ifdef __AVX512F__
    static size_t compute_avx512(const uint64_t* a, const uint64_t* b, size_t num_chunks) {
        size_t dist = 0;
        size_t i = 0;
        
        for (; i + 7 < num_chunks; i += 8) {
            __m512i va = _mm512_loadu_si512((__m512i*)(a + i));
            __m512i vb = _mm512_loadu_si512((__m512i*)(b + i));
            __m512i vxor = _mm512_xor_si512(va, vb);
            
            uint64_t temp[8];
            _mm512_storeu_si512((__m512i*)temp, vxor);
            for (int j = 0; j < 8; j++) {
                dist += __builtin_popcountll(temp[j]);
            }
        }
        
        for (; i < num_chunks; i++) {
            dist += __builtin_popcountll(a[i] ^ b[i]);
        }
        return dist;
    }
#endif

    static size_t compute_optimized(const uint64_t* a, const uint64_t* b, size_t num_chunks) {
#ifdef __AVX512F__
        return compute_avx512(a, b, num_chunks);
#elif defined(__AVX2__)
        return compute_avx2(a, b, num_chunks);
#elif defined(__ARM_NEON)
        return compute_neon(a, b, num_chunks);
#else
        return compute(a, b, num_chunks);
#endif
    }
};

// ============================================
// TERNARY DISTANCE (with SIMD)
// ============================================

class TernaryDistance {
public:
    // Scalar implementation
    static size_t compute_l2_squared(const uint8_t* a, const uint8_t* b, size_t dim) {
        size_t dist = 0;
        
        for (size_t i = 0; i < dim; i++) {
            int8_t val_a = TernaryQuantizer::get_value(a, i);
            int8_t val_b = TernaryQuantizer::get_value(b, i);
            int diff = val_a - val_b;
            dist += diff * diff;
        }
        
        return dist;
    }
    
#ifdef __ARM_NEON
    // NEON-optimized version for ARM
    static size_t compute_l2_squared_neon(const uint8_t* a, const uint8_t* b, size_t dim) {
        size_t dist = 0;
        size_t i = 0;
        
        // Process 16 ternary values at a time (8 bytes = 32 2-bit values, but we'll do 16)
        uint32x4_t sum_vec = vdupq_n_u32(0);
        
        // Process bytes in chunks
        size_t num_bytes = (dim + 3) / 4;
        size_t chunk_bytes = num_bytes & ~7;  // Round down to multiple of 8
        
        for (i = 0; i < chunk_bytes; i += 8) {
            // Load 8 bytes from each vector
            uint8x8_t va = vld1_u8(a + i);
            uint8x8_t vb = vld1_u8(b + i);
            
            // Process each ternary value (2 bits each)
            // Extract and compute differences for 4 values per byte
            for (int byte_idx = 0; byte_idx < 8; byte_idx++) {
                uint8_t byte_a = va[byte_idx];
                uint8_t byte_b = vb[byte_idx];
                
                // Extract 4 ternary values from each byte
                for (int bit_pair = 0; bit_pair < 4; bit_pair++) {
                    int shift = bit_pair * 2;
                    int8_t val_a = ((byte_a >> shift) & 0x03);
                    int8_t val_b = ((byte_b >> shift) & 0x03);
                    
                    // Convert 2-bit encoding to ternary (-1, 0, 1)
                    if (val_a == 0x02) val_a = -1;
                    else if (val_a == 0x01) val_a = 1;
                    else val_a = 0;
                    
                    if (val_b == 0x02) val_b = -1;
                    else if (val_b == 0x01) val_b = 1;
                    else val_b = 0;
                    
                    int diff = val_a - val_b;
                    dist += diff * diff;
                }
            }
        }
        
        // Handle remaining values
        for (i = chunk_bytes * 4; i < dim; i++) {
            int8_t val_a = TernaryQuantizer::get_value(a, i);
            int8_t val_b = TernaryQuantizer::get_value(b, i);
            int diff = val_a - val_b;
            dist += diff * diff;
        }
        
        return dist;
    }
#endif

#ifdef __AVX2__
    // AVX2-optimized version for x86_64
    static size_t compute_l2_squared_avx2(const uint8_t* a, const uint8_t* b, size_t dim) {
        size_t dist = 0;
        size_t i = 0;
        
        // Process bytes in chunks of 32
        size_t num_bytes = (dim + 3) / 4;
        size_t chunk_bytes = num_bytes & ~31;
        
        __m256i sum = _mm256_setzero_si256();
        
        for (i = 0; i < chunk_bytes; i += 32) {
            __m256i va = _mm256_loadu_si256((__m256i*)(a + i));
            __m256i vb = _mm256_loadu_si256((__m256i*)(b + i));
            
            // For simplicity, extract and process scalar
            // A full SIMD implementation would unpack 2-bit values
            uint8_t temp_a[32], temp_b[32];
            _mm256_storeu_si256((__m256i*)temp_a, va);
            _mm256_storeu_si256((__m256i*)temp_b, vb);
            
            for (int j = 0; j < 32; j++) {
                uint8_t byte_a = temp_a[j];
                uint8_t byte_b = temp_b[j];
                
                for (int bit_pair = 0; bit_pair < 4; bit_pair++) {
                    int shift = bit_pair * 2;
                    int8_t val_a = ((byte_a >> shift) & 0x03);
                    int8_t val_b = ((byte_b >> shift) & 0x03);
                    
                    if (val_a == 0x02) val_a = -1;
                    else if (val_a == 0x01) val_a = 1;
                    else val_a = 0;
                    
                    if (val_b == 0x02) val_b = -1;
                    else if (val_b == 0x01) val_b = 1;
                    else val_b = 0;
                    
                    int diff = val_a - val_b;
                    dist += diff * diff;
                }
            }
        }
        
        // Handle remaining values
        for (i = chunk_bytes * 4; i < dim; i++) {
            int8_t val_a = TernaryQuantizer::get_value(a, i);
            int8_t val_b = TernaryQuantizer::get_value(b, i);
            int diff = val_a - val_b;
            dist += diff * diff;
        }
        
        return dist;
    }
#endif

#ifdef __AVX512F__
    // AVX512-optimized version
    static size_t compute_l2_squared_avx512(const uint8_t* a, const uint8_t* b, size_t dim) {
        size_t dist = 0;
        size_t i = 0;
        
        size_t num_bytes = (dim + 3) / 4;
        size_t chunk_bytes = num_bytes & ~63;
        
        for (i = 0; i < chunk_bytes; i += 64) {
            __m512i va = _mm512_loadu_si512((__m512i*)(a + i));
            __m512i vb = _mm512_loadu_si512((__m512i*)(b + i));
            
            uint8_t temp_a[64], temp_b[64];
            _mm512_storeu_si512((__m512i*)temp_a, va);
            _mm512_storeu_si512((__m512i*)temp_b, vb);
            
            for (int j = 0; j < 64; j++) {
                uint8_t byte_a = temp_a[j];
                uint8_t byte_b = temp_b[j];
                
                for (int bit_pair = 0; bit_pair < 4; bit_pair++) {
                    int shift = bit_pair * 2;
                    int8_t val_a = ((byte_a >> shift) & 0x03);
                    int8_t val_b = ((byte_b >> shift) & 0x03);
                    
                    if (val_a == 0x02) val_a = -1;
                    else if (val_a == 0x01) val_a = 1;
                    else val_a = 0;
                    
                    if (val_b == 0x02) val_b = -1;
                    else if (val_b == 0x01) val_b = 1;
                    else val_b = 0;
                    
                    int diff = val_a - val_b;
                    dist += diff * diff;
                }
            }
        }
        
        for (i = chunk_bytes * 4; i < dim; i++) {
            int8_t val_a = TernaryQuantizer::get_value(a, i);
            int8_t val_b = TernaryQuantizer::get_value(b, i);
            int diff = val_a - val_b;
            dist += diff * diff;
        }
        
        return dist;
    }
#endif

    static size_t compute_l2_squared_optimized(const uint8_t* a, const uint8_t* b, size_t dim) {
#ifdef __AVX512F__
        return compute_l2_squared_avx512(a, b, dim);
#elif defined(__AVX2__)
        return compute_l2_squared_avx2(a, b, dim);
#elif defined(__ARM_NEON)
        return compute_l2_squared_neon(a, b, dim);
#else
        return compute_l2_squared(a, b, dim);
#endif
    }
};

// ============================================
// BINARY SPACE
// ============================================

class BinarySpace : public SpaceInterface<size_t> {
private:
    size_t dim_;
    size_t num_chunks_;
    size_t data_size_;
    DISTFUNC<size_t> fstdistfunc_;
    void* dist_func_param_;
    
public:
    BinarySpace(size_t dim) : dim_(dim) {
        num_chunks_ = (dim + 63) / 64;
        data_size_ = num_chunks_ * sizeof(uint64_t);
        
        fstdistfunc_ = [](const void* pVect1, const void* pVect2, const void* qty_ptr) -> size_t {
            size_t num_chunks = *((size_t*)qty_ptr);
            const uint64_t* a = (const uint64_t*)pVect1;
            const uint64_t* b = (const uint64_t*)pVect2;
            return HammingDistance::compute_optimized(a, b, num_chunks);
        };
        
        dist_func_param_ = &num_chunks_;
    }
    
    size_t get_data_size() override { return data_size_; }
    DISTFUNC<size_t> get_dist_func() override { return fstdistfunc_; }
    void* get_dist_func_param() override { return dist_func_param_; }
    
    ~BinarySpace() {}
};

// ============================================
// TERNARY SPACE
// ============================================

class TernarySpace : public SpaceInterface<size_t> {
private:
    size_t dim_;
    size_t data_size_;
    DISTFUNC<size_t> fstdistfunc_;
    void* dist_func_param_;
    
public:
    TernarySpace(size_t dim) : dim_(dim) {
        data_size_ = (dim + 3) / 4;
        
        fstdistfunc_ = [](const void* pVect1, const void* pVect2, const void* qty_ptr) -> size_t {
            size_t dim = *((size_t*)qty_ptr);
            const uint8_t* a = (const uint8_t*)pVect1;
            const uint8_t* b = (const uint8_t*)pVect2;
            return TernaryDistance::compute_l2_squared_optimized(a, b, dim);
        };
        
        dist_func_param_ = &dim_;
    }
    
    size_t get_data_size() override { return data_size_; }
    DISTFUNC<size_t> get_dist_func() override { return fstdistfunc_; }
    void* get_dist_func_param() override { return dist_func_param_; }
    
    ~TernarySpace() {}
};

} // namespace hnswlib


// ============================================
// L1 SPACE (Manhattan Distance)
// ============================================

namespace hnswlib {

#if 0

class L1Space : public SpaceInterface<float> {
    DISTFUNC<float> fstdistfunc_;
    size_t data_size_;
    size_t dim_;

public:
    L1Space(size_t dim) {
        fstdistfunc_ = [](const void *pVect1v, const void *pVect2v, const void *qty_ptr) -> float {
            float *pVect1 = (float *) pVect1v;
            float *pVect2 = (float *) pVect2v;
            size_t qty = *((size_t *) qty_ptr);

            float res = 0;
            for (size_t i = 0; i < qty; i++) {
                float diff = pVect1[i] - pVect2[i];
                res += std::abs(diff);
            }
            return res;
        };
        
        dim_ = dim;
        data_size_ = dim * sizeof(float);
    }

    size_t get_data_size() override {
        return data_size_;
    }

    DISTFUNC<float> get_dist_func() override {
        return fstdistfunc_;
    }

    void *get_dist_func_param() override {
        return &dim_;
    }

    ~L1Space() {}
};

#endif

// Peek at the index file to get element count
inline std::pair<size_t, size_t> peek_index_elements(std::istream& ifs) {
    // Save current position
    std::streampos original_pos = ifs.tellg();

    // HNSWlib saves these values at the start of the file (in order):
    size_t offsetLevel0;
    size_t max_elements;
    size_t cur_element_count;
    size_t size_data_per_element;
/*
    size_t label_offset;
    size_t offsetData;
    size_t max_level;
    size_t enterpoint_node;
    size_t maxM;
    size_t maxM0;
    size_t M;
    size_t mult;
    size_t ef_construction;
*/

    // Read the header
    readBinaryPOD(ifs, offsetLevel0);
    readBinaryPOD(ifs, max_elements); 
    readBinaryPOD(ifs, cur_element_count);
    readBinaryPOD(ifs, size_data_per_element); // Read this for debugging

    /* std::cout << "MAX elements = " << max_elements << " count=" << cur_element_count << 
	" data_per_element=" << size_data_per_element << std::endl; */

    // Restore position
    ifs.seekg(original_pos);

    return {cur_element_count,max_elements};
}


} // namespace hnswlib


// ============================================
// UNIFIED INDEX (supports all metrics)
// ============================================

struct UnifiedIndexMeta {
    MetricSpace    metric_;
    size_t         dim_;
    // Using uint8_t rather than bool since the sizeof(bool)
    // can vary depending upon the compiler from sizeof(char) to sizeof(int)
    bool           enable_rescoring_; // Only effects quantized metrics
    bool           normalize_; // Use normalized vectors

    UnifiedIndexMeta() {;}
    UnifiedIndexMeta(MetricSpace metric, size_t dim, bool enable_rescoring, bool normalize)
        : metric_(metric), dim_(dim), enable_rescoring_(enable_rescoring), normalize_(normalize) {}

    static size_t size() {
       // This needs to match the save/load below
       return 2*sizeof(uint32_t) + 2*sizeof(uint8_t); 
    }
    bool save(std::ofstream &out) const {
       if (!out.good()) return false;
       // Write header: magic number, metric type, rescoring flag
       out.write(reinterpret_cast<const char*>(&magic_), sizeof(uint32_t));
        
       uint32_t metric_id = static_cast<uint32_t>(metric_);
       out.write(reinterpret_cast<const char*>(&metric_id), sizeof(uint32_t));

       uint32_t dim = static_cast<uint32_t>(dim_);
       out.write(reinterpret_cast<const char*>(&dim), sizeof(uint32_t));
                   
       uint8_t val = static_cast<uint8_t>(normalize_);
       out.write(reinterpret_cast<const char*>(&val), sizeof(uint8_t));
       val = static_cast<uint8_t>(enable_rescoring_);
       out.write(reinterpret_cast<const char*>(&val), sizeof(uint8_t));
        
       return out.good();
    }
    bool load(std::ifstream &in) {
       uint32_t saved_magic;
       in.read(reinterpret_cast<char*>(&saved_magic), sizeof(uint32_t));
       if (saved_magic != magic_) {
         throw std::runtime_error("Invalid index file: bad magic number");
         return false; // We stop here since its not an index! 
       }

       MetricSpace saved_metric;
       in.read(reinterpret_cast<char*>(&saved_metric), sizeof(uint32_t));
       if (saved_metric != metric_) {
            const std::string metric_s = HnswConfig::metric_space_to_string(saved_metric);
            LOG_WARN_S() << "Index was saved with different metric: "
		<< metric_s << "!=" << HnswConfig::metric_space_to_string(metric_)
		<< "Using " << metric_s;
	    metric_ = saved_metric;
       }
 
       u_int32_t saved_dim;
       in.read(reinterpret_cast<char*>(&saved_dim), sizeof(uint32_t));
       if (saved_dim != dim_) {
            throw std::runtime_error("Dimension mismatch in saved index");
            return false;
       }

       uint8_t val;
       in.read(reinterpret_cast<char*>(&val), sizeof(uint8_t));
       if (val != normalize_) {
	    // Cosine is ALWAYS normalized!
	    if (metric_ != MetricSpace::Cosine) {
		LOG_WARN_S() << "Index was saved with normalization "
                << (val ? "enabled" : "disabled" ) << ". Status now aligned.";
	    }
            normalize_ = val;
       }
       in.read(reinterpret_cast<char*>(&val), sizeof(uint8_t));
       enable_rescoring_ = val;
       return in.good() && !in.eof();
    }
 private:
   const uint32_t magic_ = 0x484E5357; // "HNSW" in hex

} ;


namespace hnswlib {

// Look at a stored index file and fetch its 
// <curent_element_count, max_elements>
// We use this to read an index BEFORE we create a Unified Index to make
// use that we allocate suitable sizes
inline std::pair<size_t, size_t> peek_index_elements(const std::string path) {
    std::ifstream input(path, std::ios::binary);
    if (input.is_open()) {
      input.seekg(UnifiedIndexMeta::size());
      auto result = peek_index_elements(input);
      input.close();
      return result;
    }   
    return {};
}
} // namespace hnswlib



class UnifiedIndex {
private:
    UnifiedIndexMeta meta;

    // We create a reference to the members of meta to simplify things:
    MetricSpace &metric_           = meta.metric_;
    size_t &dim_              = meta.dim_;
    bool   &enable_rescoring_ = meta.enable_rescoring_; // Store original vectors for re-scoring
    bool   &normalize_        = meta.normalize_; // Metric::Cosine or normalize before quantization

    bool is_quantized_;  // Binary or Ternary
    bool is_binary_;     // true = Binary, false = Ternary

    const uint32_t magic_ = 0x484E5357; // "HNSW" in hex
    
    // Float-based index
    std::unique_ptr<hnswlib::HierarchicalNSW<float>> float_index_;
    std::unique_ptr<hnswlib::SpaceInterface<float>> float_space_;
    
    // Quantized indices
    std::unique_ptr<hnswlib::HierarchicalNSW<size_t>> quant_index_;
    std::unique_ptr<hnswlib::SpaceInterface<size_t>> quant_space_;
    
    // Original float vectors for re-scoring (only used with quantized indices)
    std::unordered_map<hnswlib::labeltype, std::vector<float>> original_vectors_;
    
    // ---------------------------------------------------------------------
    // Helper: Normalize for cosine similarity if required.
    // ---------------------------------------------------------------------
    std::vector<float> normalize_vector(const float* data) const {
        std::vector<float> normalized(data, data + dim_);
        float norm = 0.0f;
        for (size_t i = 0; i < dim_; i++) {
            norm += normalized[i] * normalized[i];
        }
        norm = std::sqrt(norm);
        if (norm > 1e-8f) {  // Avoid division by zero
            for (size_t i = 0; i < dim_; i++) {
                normalized[i] /= norm;
            }
        }
        return normalized;
    }
   
public:
    UnifiedIndex(MetricSpace metric, size_t dim, size_t max_elements, 
                 size_t M = 16, size_t ef_construction = 200,
                 bool enable_rescoring = true,
		 bool normalize_before_quantization = true)
        : meta {metric, dim, enable_rescoring, normalize_before_quantization} {

        is_quantized_ = (metric == MetricSpace::Binary || metric == MetricSpace::Ternary);
        if (metric == MetricSpace::Cosine) normalize_ = true; // Cosine is NORMALIZED!:
        else if (normalize_ && !is_quantized_) normalize_ = false; // !!
        is_binary_ = (metric == MetricSpace::Binary);
        // Re-scoring ONLY makes sense for quantized vectors
        // For Float Metrics (L1, L2, IP, Cosine):
        // No benefit from re-scoring because:
        // Already using original vectors: The HNSW index stores and searches with full float precision
        // Distances are exact: L2, Cosine, etc. computed on actual float values
        // No approximation: Nothing to "re-score" - the distances are already accurate
        // Would be redundant: Re-computing the same distance gives the same result
        if (!is_quantized_) enable_rescoring_ = false; 
        
        
        if (is_quantized_) {
            // Binary or Ternary quantization
            if (is_binary_) {
                quant_space_ = std::make_unique<hnswlib::BinarySpace>(dim);
            } else {
                quant_space_ = std::make_unique<hnswlib::TernarySpace>(dim);
            }
            quant_index_ = std::make_unique<hnswlib::HierarchicalNSW<size_t>>(
                quant_space_.get(), max_elements, M, ef_construction);
        } else {
            // Float-based space
            switch (metric) {
                case MetricSpace::L1:
                    float_space_ = std::make_unique<hnswlib::L1Space>(dim);
                    break;
                case MetricSpace::L2:
                    float_space_ = std::make_unique<hnswlib::L2Space>(dim);
                    break;
                case MetricSpace::InnerProduct:
                case MetricSpace::Cosine:
                    float_space_ = std::make_unique<hnswlib::InnerProductSpace>(dim);
                    break;
                default:
                    throw std::runtime_error("Unknown metric");
            }
            float_index_ = std::make_unique<hnswlib::HierarchicalNSW<float>>(
                float_space_.get(), max_elements, M, ef_construction);
        }
    }

    

    // This removes all elements leaving it empty.
    void clear() {
      // We re-use the space and fetch the params 
      if (is_quantized_) {
         original_vectors_.clear();
         size_t max_elements = quant_index_->max_elements_;
         size_t M = quant_index_->M_;
         size_t ef_construction = quant_index_->ef_construction_;
         quant_index_ = std::make_unique<hnswlib::HierarchicalNSW<size_t>>(
                quant_space_.get(), max_elements, M, ef_construction);
      } else {
         size_t max_elements = float_index_->max_elements_;
         size_t M = float_index_->M_;
         size_t ef_construction = float_index_->ef_construction_;
         float_index_ = std::make_unique<hnswlib::HierarchicalNSW<float>>(
                float_space_.get(), max_elements, M, ef_construction);
      }
    }
    // How many elements?
    size_t size() const {
      if (is_quantized_) return quant_index_->cur_element_count ;
      else return float_index_->cur_element_count ;
    }
    // Index empty?
    bool empty() const { return size() == 0; }
    
    void addPoint(const float* data, size_t label) {
        // Store original vector if rescoring is enabled and using quantization
        if (is_quantized_ && enable_rescoring_) {
            original_vectors_[label] = std::vector<float>(data, data + dim_);
        }

        if (is_quantized_) {
            // Normalize before quantization if requested (improves cosine-like behavior)
            const float* data_to_quantize = data;
            std::vector<float> normalized;

            if (normalize_) {
                normalized = normalize_vector(data);
                data_to_quantize = normalized.data();
            }

            if (is_binary_) {
                auto binary = hnswlib::BinaryQuantizer::quantize(data_to_quantize, dim_);
                quant_index_->addPoint(binary.data(), label);
            } else {
                auto ternary = hnswlib::TernaryQuantizer::quantize(data_to_quantize, dim_);
                quant_index_->addPoint(ternary.data(), label);
            }
        } else {
            // Normalize for cosine
            if (metric_ == MetricSpace::Cosine) {
                std::vector<float> normalized = normalize_vector(data);
                float_index_->addPoint(normalized.data(), label);
            } else {
                float_index_->addPoint(data, label);
            }
        }
    }
    
    std::vector<std::pair<float, hnswlib::labeltype>> searchKnn(
        const float* query, size_t k) {
        
        std::vector<std::pair<float, hnswlib::labeltype>> results;
        
        if (is_quantized_) {
            if (is_binary_) {
                auto binary = hnswlib::BinaryQuantizer::quantize(query, dim_);
                auto pq = quant_index_->searchKnn(binary.data(), k);
                
                while (!pq.empty()) {
                    auto [dist, label] = pq.top();
                    results.push_back({static_cast<float>(dist), label});
                    pq.pop();
                }
            } else {
                auto ternary = hnswlib::TernaryQuantizer::quantize(query, dim_);
                auto pq = quant_index_->searchKnn(ternary.data(), k);
                
                while (!pq.empty()) {
                    auto [dist, label] = pq.top();
                    results.push_back({static_cast<float>(dist), label});
                    pq.pop();
                }
            }
        } else {
            std::vector<float> query_vec(query, query + dim_);
            if (metric_ == MetricSpace::Cosine) {
                float norm = 0.0f;
                for (size_t i = 0; i < dim_; i++) {
                    norm += query_vec[i] * query_vec[i];
                }
                norm = std::sqrt(norm);
                if (norm > 0) {
                    for (size_t i = 0; i < dim_; i++) {
                        query_vec[i] /= norm;
                    }
                }
            }
            
            auto pq = float_index_->searchKnn(query_vec.data(), k);
            while (!pq.empty()) {
                auto [dist, label] = pq.top();
                results.push_back({dist, label});
                pq.pop();
            }
        }
        
        std::reverse(results.begin(), results.end());
        return results;
    }
    
    // Search k-NN with results ordered closer first (ascending distance)
    std::vector<std::pair<float, hnswlib::labeltype>> searchKnnCloserFirst(
        const float* query, size_t k) {
        
        std::vector<std::pair<float, hnswlib::labeltype>> results;
        
        if (is_quantized_) {
            if (is_binary_) {
                auto binary = hnswlib::BinaryQuantizer::quantize(query, dim_);
                auto candidates = quant_index_->searchKnnCloserFirst(binary.data(), k);
                
                for (const auto& [dist, label] : candidates) {
                    results.push_back({static_cast<float>(dist), label});
                }
            } else {
                auto ternary = hnswlib::TernaryQuantizer::quantize(query, dim_);
                auto candidates = quant_index_->searchKnnCloserFirst(ternary.data(), k);
                
                for (const auto& [dist, label] : candidates) {
                    results.push_back({static_cast<float>(dist), label});
                }
            }
        } else {
            std::vector<float> query_vec(query, query + dim_);
            if (metric_ == MetricSpace::Cosine) {
                float norm = 0.0f;
                for (size_t i = 0; i < dim_; i++) {
                    norm += query_vec[i] * query_vec[i];
                }
                norm = std::sqrt(norm);
                if (norm > 0) {
                    for (size_t i = 0; i < dim_; i++) {
                        query_vec[i] /= norm;
                    }
                }
            }
            
            results = float_index_->searchKnnCloserFirst(query_vec.data(), k);
        }
        
        return results;
    }
    
    // Search with re-scoring using original float vectors
    // Returns more accurate distances for quantized indices
    std::vector<std::pair<float, hnswlib::labeltype>> searchKnnWithRescoring(
        const float* query, size_t k, size_t fetch_k = 0) {
        
        if (!is_quantized_ || !enable_rescoring_) {
            // No rescoring needed for float indices or if rescoring disabled
            return searchKnnCloserFirst(query, k);
        }
        
        // Fetch more candidates than needed (default: 2x)
        if (fetch_k == 0) {
            fetch_k = k * 2;
        }
        
        // Get candidates using quantized index
        auto candidates = searchKnnCloserFirst(query, fetch_k);
        
        // Re-score with original float vectors using L2 distance
        std::vector<std::pair<float, hnswlib::labeltype>> rescored;
        rescored.reserve(candidates.size());
        
        for (const auto& [quant_dist, label] : candidates) {
            auto it = original_vectors_.find(label);
            if (it == original_vectors_.end()) {
                // Fallback to quantized distance if original not found
                rescored.push_back({quant_dist, label});
                continue;
            }
            
            const auto& original = it->second;
            
            // Compute L2 distance with original vectors
            float dist = 0.0f;
            for (size_t i = 0; i < dim_; i++) {
                float diff = query[i] - original[i];
                dist += diff * diff;
            }
            
            rescored.push_back({dist, label});
        }
        
        // Sort by rescored distance and keep top k
        std::sort(rescored.begin(), rescored.end(),
                  [](const auto& a, const auto& b) { return a.first < b.first; });
        
        if (rescored.size() > k) {
            rescored.resize(k);
        }
        
        return rescored;
    }
    
    std::vector<std::pair<float, hnswlib::labeltype>> searchWithStopCondition(
        const float* query, float epsilon, size_t min_cand, size_t max_cand) {
        
        std::vector<std::pair<float, hnswlib::labeltype>> results;
        
        if (is_quantized_) {
            // For quantized: epsilon is in integer space (Hamming or L2 squared)
            size_t eps_int = static_cast<size_t>(epsilon);
            hnswlib::EpsilonSearchStopCondition<size_t> cond(eps_int, min_cand, max_cand);
            
            if (is_binary_) {
                auto binary = hnswlib::BinaryQuantizer::quantize(query, dim_);
                auto candidates = quant_index_->searchStopConditionClosest(binary.data(), cond);
                
                for (const auto& [dist, label] : candidates) {
                    results.push_back({static_cast<float>(dist), label});
                }
            } else {
                auto ternary = hnswlib::TernaryQuantizer::quantize(query, dim_);
                auto candidates = quant_index_->searchStopConditionClosest(ternary.data(), cond);
                
                for (const auto& [dist, label] : candidates) {
                    results.push_back({static_cast<float>(dist), label});
                }
            }
        } else {
            std::vector<float> query_vec(query, query + dim_);
            if (metric_ == MetricSpace::Cosine) {
                float norm = 0.0f;
                for (size_t i = 0; i < dim_; i++) {
                    norm += query_vec[i] * query_vec[i];
                }
                norm = std::sqrt(norm);
                if (norm > 0) {
                    for (size_t i = 0; i < dim_; i++) {
                        query_vec[i] /= norm;
                    }
                }
            }
            
            hnswlib::EpsilonSearchStopCondition<float> cond(epsilon, min_cand, max_cand);
            results = float_index_->searchStopConditionClosest(query_vec.data(), cond);
        }
        
        return results;
    }
    
    void setEf(size_t ef) {
        if (is_quantized_) {
            quant_index_->setEf(ef);
        } else {
            float_index_->setEf(ef);
        }
    }
    
   bool saveIndex(const std::string& path) {
        bool result;
        std::ofstream output(path, std::ios::binary);
        if (!output.is_open()) { 
            LOG_ERROR_S() << "[saveIndex] Cannot open file for writing: " << path;
            return false;
        }   
        result = saveIndex(output);
        output.close(); 
        return result;
    }


// TODO: store the size of the HNSW index in the header! This way
// we can load it all..

// Prompt:  since we know how many active elements we have in a HNSW index: 
// cur_element_count (this is also stored in the HNSW index's own header)
// don't we want to store this is the magic and when we read the magic in
// load index we don't need to worry about the lib reading too far.
// The problem is: when the max_elements specified is less than the stored
// cur_element_count. In this light would it not make better sense to store
// the vectors BEFORE we store index HNSW index?
//
    bool saveIndex(std::ofstream &out) {
        if (!out.good()) {
            LOG_ERROR_S() << "[saveIndex] Stream error!";
            return false;
        }
#if 1
        // Write header: magic number, metric type, rescoring flag,..
        meta.save(out); 
#else 
        // Write header: magic number, metric type, rescoring flag
        out.write(reinterpret_cast<const char*>(&magic_), sizeof(uint32_t));
        
        uint32_t metric_id = static_cast<uint32_t>(metric_);
        out.write(reinterpret_cast<const char*>(&metric_id), sizeof(uint32_t));
        
        out.write(reinterpret_cast<const char*>(&enable_rescoring_), sizeof(bool));
//      out.write(reinterpret_cast<const char*>(&normalized_), sizeof(bool));

        out.write(reinterpret_cast<const char*>(&dim_), sizeof(size_t));
#endif

        // Save HNSW index
        // We save the element count to make sure that when we load it we can
        // trap when the max_elements < element count as that can result in
        // unpredictable behaviour.
        if (is_quantized_) {
//          out.write(reinterpret_cast<const char*>(&(quant_index_->cur_element_count)), sizeof(size_t));
            quant_index_->saveIndex(out);
        } else {
//          out.write(reinterpret_cast<const char*>(&(float_index_->cur_element_count)), sizeof(size_t));
            float_index_->saveIndex(out);
        }
        
        // Save original vectors if rescoring is enabled
        if (enable_rescoring_ && is_quantized_ && !original_vectors_.empty()) {
            size_t num_vectors = original_vectors_.size();
            out.write(reinterpret_cast<const char*>(&num_vectors), sizeof(size_t));
            
            // Write each vector with its label
            for (const auto& [label, vec] : original_vectors_) {
                out.write(reinterpret_cast<const char*>(&label), sizeof(hnswlib::labeltype));
                out.write(reinterpret_cast<const char*>(vec.data()), dim_ * sizeof(float));
            }
        }
        return true;    
    }

   bool loadIndex(const std::string& path, size_t max_elements) {
        std::ifstream input(path, std::ios::binary);
         if (!input.is_open()) {
            LOG_ERROR_S() << "[loadIndex] Cannot open file for reading: " << path;
            return false;
        }
        bool result = loadIndex(input, max_elements);
        input.close();
        return result;
    }
    
    bool loadIndex(std::ifstream &in, size_t max_elements) {
        if (!in.good()) {
            LOG_ERROR_S() << "[loadIndex] Stream eror!";
            return false;
        }
        
        // Read and verify header
        if (!meta.load(in)) return false;
        const bool saved_rescoring =  enable_rescoring_;

//      size_t element_count;
//      in.read(reinterpret_cast<char*>(&element_count), sizeof(size_t));

        // Check that the index capcity is sufficient
        auto [element_count, max_from_file] = hnswlib::peek_index_elements(in);
        if (element_count > max_elements) {
// TODO: IDEA.. If we allocate it first as 0 meaning we'll load up
// we can create a suitable space and index HERE
//
            if (max_from_file > max_elements)
                LOG_ERROR_S() << "Insufficient capacity. Index was stored with "
			<< element_count << " and max_elements " << max_from_file; 
            else throw std::runtime_error("Insufficient space specified.");
            return false;
        }
        
        // Load HNSW index
        if (is_quantized_) {
            quant_index_->loadIndex(in, quant_space_.get(), max_elements);
        } else {
            float_index_->loadIndex(in, float_space_.get(), max_elements);
        }
        
        // Load original vectors if they were saved
        if (saved_rescoring && is_quantized_) {
            size_t num_vectors;
            in.read(reinterpret_cast<char*>(&num_vectors), sizeof(size_t));
            
            original_vectors_.clear();
            for (size_t i = 0; i < num_vectors; i++) {
                hnswlib::labeltype label;
                in.read(reinterpret_cast<char*>(&label), sizeof(hnswlib::labeltype));
                
                std::vector<float> vec(dim_);
                in.read(reinterpret_cast<char*>(vec.data()), dim_ * sizeof(float));
                
                original_vectors_[label] = std::move(vec);
            }
            
            if (!enable_rescoring_) {
                LOG_WARN_S() << "Index was saved with rescoring enabled, "
                          << "but current index has rescoring disabled. "
                          << "Original vectors loaded but will not be used.";
            }
        } else if (enable_rescoring_ && is_quantized_ && !saved_rescoring) {
             LOG_WARN_S() << "Index was saved without rescoring, "
                      << "but current index has rescoring enabled. "
                      << "Re-scoring will not work correctly.";
        }
        
        return true;
    }

    // Cover a dist to a score [0-1]
    float score_from_dist(float dist) const;
    
    MetricSpace getMetric() const { return metric_; }
    size_t getDim() const { return dim_; }
    bool isRescoringEnabled() const { return enable_rescoring_; }
    
    // Get number of stored original vectors
    size_t getOriginalVectorCount() const { 
        return original_vectors_.size(); 
    }
    
    // Get storage size per vector
    size_t getStorageSize() const {
        size_t index_size;
        if (is_binary_) {
            index_size = (dim_ + 63) / 64 * 8;  // bytes
        } else if (metric_ == MetricSpace::Ternary) {
            index_size = (dim_ + 3) / 4;  // bytes
        } else {
            index_size = dim_ * sizeof(float);  // bytes
        }
        
        // Add original vector storage if rescoring enabled
        if (is_quantized_ && enable_rescoring_) {
            return index_size + (dim_ * sizeof(float));
        }
        return index_size;
    }
};

/*
USAGE IN YOUR APPLICATION:

// Create unified index with your desired metric
auto index = std::make_unique<UnifiedIndex>(cfg.metric, embedder.n_embd, max_elements);

// Add points
index->addPoint(embedding.data(), label);

// Search
auto results = index->searchKnn(query.data(), k);
auto results_closer = index->searchKnnCloserFirst(query.data(), k);

// Epsilon search
auto eps_results = index->searchWithStopCondition(query.data(), epsilon, min_cand, max_cand);

// Save/load
index->saveIndex("index.bin");
index->loadIndex("index.bin", max_elements);

SUPPORTED METRICS:
- Metric::L1 - Manhattan distance (float, sum of absolute differences)
- Metric::L2 - Euclidean distance squared (float)
- Metric::InnerProduct - Inner product / dot product (float, negative for similarity)
- Metric::Cosine - Cosine similarity (float, auto-normalized)
- Metric::Binary - Hamming distance (1-bit quantization, 32x compression)
- Metric::Ternary - L2 squared (1.58-bit quantization, 16x compression)

RE-SCORING (for Binary/Ternary):
- Enable with: UnifiedIndex(metric, dim, max_elements, M, ef_construction, true)
- Stores original float vectors alongside quantized index
- Use searchKnnWithRescoring() for accurate final ranking
- Trade-off: 2x memory but much better accuracy
- Example: Binary with rescoring = 16 + 512 = 528 bytes vs 16 bytes alone

When to use re-scoring:
✅ High accuracy requirements
✅ Small to medium datasets (memory allows storing originals)
✅ Two-stage retrieval (fast quantized filter → accurate rerank)

When to skip re-scoring:
❌ Very large datasets (memory constrained)
❌ Speed is critical and approximate is acceptable
❌ Already using float metrics (L1, L2, IP, Cosine)

DISTANCE TYPES:
- L1: sum(|a[i] - b[i]|) - always positive
- L2: sum((a[i] - b[i])^2) - always positive, squared Euclidean
- InnerProduct: sum(a[i] * b[i]) - can be negative, more negative = more similar
- Cosine: 1 - sum(a[i]*b[i])/(||a||*||b||) - range [0, 2], 0 = identical
- Binary (Hamming): count of differing bits - range [0, dimension]
- Ternary (L2): sum((a[i] - b[i])^2) where a[i], b[i] in {-1, 0, 1} - range [0, 4*dimension]
*/
