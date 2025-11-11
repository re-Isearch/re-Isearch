#pragma once

#include <hnswlib/hnswlib.h>
#include <vector>
#include <cstring>
#include <numeric>
#include <cmath>
#include <memory>
#include <unordered_map>
#include <fstream>
#include <algorithm>
#include <cassert>
#include <queue>

#ifdef __AVX2__
#include <immintrin.h>
#endif

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

#include "LSMVectorStorage.h"

namespace hnswlib {

enum class Metric { L1 = 0, L2 = 1, IP = 2, Cosine = 3 };
enum class QuantizationType { NONE = 0, BINARY = 1, TERNARY = 2 };
enum class BinMode { STANDARD = 0, BETTER = 1 };


enum class SimdKind { NONE = 0, AVX2, AVX512, NEON, SVE };
// AVX2:
// Intel Haswell processors (Q2 2013) and newer, except models branded as
// Celeron and Pentium. Celeron and Pentium branded processors starting with
// Tiger Lake (Q3 2020) and newer.
//
// Most modern AMD processors, including AMD's Ryzen and Zen-based CPUs,
// support AVX2 instructions. This means processors like the Ryzen 2000,
// 3000, 5000, and 7000 series, as well as newer generations like Ryzen
// 9000, all have AVX2 support. 

// AVX512:
// Intel processors that support AVX-512 include the Xeon Phi x200 (Knights Landing)
// (first implementation), Xeon Scalable processors (Skylake, Cascade Lake, Cooper Lake,
// Ice Lake, and Rocket Lake), 11th Gen Core processors (Rocket Lake, LGA1200 and
// certain laptop chips like Tiger Lake), and Core X-series processors. Conversely,
// newer 12th, 13th, and 14th Gen Core processors have AVX-512 disabled by default.  
//
// AMD processors with the Zen 4 and newer architectures support AVX-512. This includes
// server-grade processors like the 4th Gen EPYC "Genoa" and client/desktop processors
// like the Ryzen 7000 series and newer (e.g., Ryzen 9 9950X). 

// NEON:
// Most ARM processors support NEON. ARM processors that do not support NEON include
// those with an architecture older than ARMv7, and some ARMv7 Cortex-A series processors
// that had NEON as an optional feature. This includes many older single-board computers
// like the Raspberry Pi 1 and Zero, which use ARMv6 architecture. 

// SVE:
// ARM processors that support SVE include the Fujitsu A64FX, AWS Graviton3, and ARM's
// Neoverse N2 and Cortex-A710 cores, which implement SVE or SVE2 extensions to the
// Armv8-A architecture. However, availability depends on the specific CPU, the
// implementation by the chip vendor (such as Qualcomm, which disables SVE on some of
// its processors), and even the specific Linux kernel being used on a device. 
SimdKind detect_simd();


// ============================================
// UNIFIED INDEX META HEADER 
// ============================================
struct UnifiedIndexMeta {
    size_t dim_    = 0;
    size_t max_elements_ = 0;
    Metric metric_ = Metric::L2;

    // Memory overhead:
    // Each label set stores just sizeof(labeltype) per label:
    // 10000 labels × 4 bytes = 40 KB per delta
    // 10 deltas = 400 KB total
    size_t flush_threshold_ = 10000;

    QuantizationType quantization_ = QuantizationType::NONE;
    BinMode bin_mode_ = BinMode::STANDARD;

    bool   enable_rescoring_ = false; // Only effects quantized metrics
    bool   normalize_ = false; // Use normalized vectors, Always true for Cosine

    /* Don't really need these as they are stored in the HNSW index */
    size_t M_ = 16;
    size_t ef_construction_ = 200;
    size_t ef_ = 10;

    bool quantizer_fitted_ = false;


    static size_t size() {
       return sizeof(UnifiedIndexMeta) - sizeof(magic_) - sizeof(version_);
    }

    UnifiedIndexMeta() {;}
    UnifiedIndexMeta(size_t dim, size_t max_elements,
	Metric metric = Metric::L2,
	QuantizationType quantization = QuantizationType::NONE,
	BinMode bin_mode = BinMode::STANDARD,
	bool enable_rescoring = false,
	size_t M = 16,
	size_t ef_construction = 200) :
	dim_(dim), max_elements_(max_elements), metric_(metric),
	quantization_(quantization), bin_mode_(bin_mode),
        enable_rescoring_(enable_rescoring),
        M_(M), ef_construction_(ef_construction) {;}

    UnifiedIndexMeta& operator = (const UnifiedIndexMeta& other) {
	dim_    = other.dim_ ;
	max_elements_ = other.max_elements_;
	metric_ = other.metric_;
	quantization_ = other.quantization_;
	bin_mode_ = other.bin_mode_;
	enable_rescoring_ = other.enable_rescoring_;
	normalize_ = other.normalize_;
	M_ = other.M_;
	ef_construction_ = other.ef_construction_;
	ef_ = other.ef_;
	quantizer_fitted_ = other.quantizer_fitted_;
	return *this;
    }

    bool save(std::ofstream &out) const {
	if (!out.good()) return false;
	// Write header: magic number, metric type, rescoring flag
	writeBinaryPOD(out, magic_);
	writeBinaryPOD(out, version_);
	writeBinaryPOD(out, metric_);
	writeBinaryPOD(out, dim_);
	writeBinaryPOD(out, normalize_);
	writeBinaryPOD(out, enable_rescoring_);
	writeBinaryPOD(out, quantization_);
	writeBinaryPOD(out, bin_mode_);
	writeBinaryPOD(out, quantizer_fitted_);

       // Redundant elements (also in HNSW index
	writeBinaryPOD(out, M_);
	writeBinaryPOD(out, ef_construction_);
	writeBinaryPOD(out, ef_);
	writeBinaryPOD(out, max_elements_);

       return out.good();
    }


    bool load(std::ifstream &input) {
	if (!input.good()) return false;

	uint32_t saved_magic;
	readBinaryPOD(input, saved_magic);
	if (saved_magic != magic_) {
         throw std::runtime_error("Invalid index file: bad magic number");
         return false; // We stop here since its not an index!
	}
	uint8_t saved_version;
	readBinaryPOD(input, saved_version);
	if (saved_version != version_) {
           if (saved_version > version_) 
	      throw std::runtime_error("Newer format index file: upgrade this software.!");
	   else
	      throw std::runtime_error("Obsolete format index file: re-index!");
           return false; // We stop here since its the wrong version!
	}
	readBinaryPOD(input, metric_);
	readBinaryPOD(input, dim_);
	readBinaryPOD(input, normalize_);
	readBinaryPOD(input, enable_rescoring_);
	readBinaryPOD(input, quantization_);
	readBinaryPOD(input, bin_mode_);
	readBinaryPOD(input, quantizer_fitted_);
       // Redundant elements
	readBinaryPOD(input, M_);
	readBinaryPOD(input, ef_construction_);
	readBinaryPOD(input, ef_);
	readBinaryPOD(input, max_elements_);
       //
       return input.good() && !input.eof();
    }

private:
    const uint32_t magic_  = sizeof(size_t) == sizeof(uint64_t) ?  0x484E5357 : 0x57534E48;
    const uint8_t version_ = 1;
};


// Peek at the index file to get element count and max_elements
std::pair<size_t, size_t> peek_index_elements(std::istream& ifs);

// Look at a stored index file and fetch its
// <curent_element_count, max_elements>
std::pair<size_t, size_t> peek_index_elements(const std::string path);

//SimdKind detect_simd();

void normalize_l2(float* vec, size_t dim);
void normalize_l2_batch(std::vector<std::vector<float>>& embeddings);

void binarize_scalar(const float* emb, const float* thr, size_t dim, uint8_t* out);
#ifdef __AVX2__
void binarize_avx2(const float* emb, const float* thr, size_t dim, uint8_t* out);
#endif
#ifdef __AVX512F__
void binarize_avx512(const float* emb, const float* thr, size_t dim, uint8_t* out);
#endif
#ifdef __ARM_NEON
void binarize_neon(const float* emb, const float* thr, size_t dim, uint8_t* out);
#endif

class BinaryQuantizer {
private:
    size_t dim_;
    std::vector<float> thresholds_;
    SimdKind simd_;
    BinMode mode_;
    bool normalize_before_quantization_;
    void quantize_internal(const float* emb, uint8_t* out) const;

public:
    BinaryQuantizer(size_t dim, BinMode mode = BinMode::STANDARD, 
                    bool normalize_before_quantization = false);
    void fit(const std::vector<std::vector<float>>& sample_embeddings);
    void quantize(const float* emb, uint8_t* out) const;
    size_t get_dim() const { return dim_; }
    size_t get_nbytes() const { return (dim_ + 7) / 8; }
    const std::vector<float>& get_thresholds() const { return thresholds_; }
    void set_thresholds(const std::vector<float>& thresholds);
    SimdKind get_simd() const { return simd_; }
    BinMode get_mode() const { return mode_; }
    bool get_normalize_before_quantization() const { return normalize_before_quantization_; }
};

class TernaryQuantizer {
private:
    size_t dim_;
    std::vector<float> thresholds_pos_;
    std::vector<float> thresholds_neg_;
    bool normalize_before_quantization_;

public:
    TernaryQuantizer(size_t dim, bool normalize_before_quantization = false);
    void fit(const std::vector<std::vector<float>>& sample_embeddings);
    void quantize(const float* emb, uint8_t* out) const;
    size_t get_dim() const { return dim_; }
    size_t get_nbytes() const { return (dim_ * 2 + 7) / 8; }
    const std::vector<float>& get_thresholds_pos() const { return thresholds_pos_; }
    const std::vector<float>& get_thresholds_neg() const { return thresholds_neg_; }
    void set_thresholds(const std::vector<float>& pos, const std::vector<float>& neg);
};

class HammingDistance {
public:
    static uint32_t compute(const uint8_t* a, const uint8_t* b, size_t nbytes);
#ifdef __ARM_NEON
    static uint32_t compute_neon(const uint8_t* a, const uint8_t* b, size_t nbytes);
#endif
#ifdef __AVX2__
    static uint32_t compute_avx2(const uint8_t* a, const uint8_t* b, size_t nbytes);
#endif
    static uint32_t compute_optimized(const uint8_t* a, const uint8_t* b, size_t nbytes);
};

class TernaryDistance {
public:
    static uint32_t compute(const uint8_t* a, const uint8_t* b, size_t dim);
};

// ============================================
// 1-bit SPACE 
// ============================================

class BinarySpace : public SpaceInterface<size_t> {
private:
    size_t dim_,nbytes_;
    DISTFUNC<size_t> fstdistfunc_;
    void* dist_func_param_;
public:
    BinarySpace(size_t dim);
    size_t get_data_size() override;
    DISTFUNC<size_t> get_dist_func() override;
    void* get_dist_func_param() override;
};

// ============================================
// 1.58 bit SPACE 
// ============================================

class TernarySpace : public SpaceInterface<size_t> {
private:
    size_t dim_, nbytes_;
    DISTFUNC<size_t> fstdistfunc_;
    void* dist_func_param_;
public:
    TernarySpace(size_t dim);
    size_t get_data_size() override;
    DISTFUNC<size_t> get_dist_func() override;
    void* get_dist_func_param() override;
};


// ============================================
// L1 SPACE (Manhattan Distance)
// ============================================
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


// ============================================
// UNIFIED INDEX (supports all metrics)
// ============================================


class UnifiedIndex {
private:
    LSMVectorStorage vector_storage_;
    size_t additions_since_flush_ = 0;
    size_t &flush_threshold_ = meta.flush_threshold_;

    UnifiedIndexMeta meta_;
    Metric &metric_ = meta_.metric_;
    size_t &dim_ = meta_.dim_;
    bool &enable_rescoring_ = meta_.enable_rescoring_;
    bool &normalize_ = meta_.normalize_;
    QuantizationType &quantization_ = meta_.quantization_;
    BinMode &bin_mode_ = meta_.bin_mode_;
    bool   &quantizer_fitted_ = meta_.quantizer_fitted_;
    // 
    size_t &max_elements_ = meta_.max_elements_;
    size_t &M_ = meta_.M_;
    size_t &ef_construction_ = meta_.ef_construction_;
    size_t &ef_ = meta_.ef_;
    
    std::unique_ptr<HierarchicalNSW<float>> float_index_;
    std::unique_ptr<SpaceInterface<float>> float_space_;
    std::unique_ptr<HierarchicalNSW<size_t>> quant_index_;
    std::unique_ptr<SpaceInterface<size_t>> quant_space_;
    std::unique_ptr<BinaryQuantizer> binary_quantizer_;
    std::unique_ptr<TernaryQuantizer> ternary_quantizer_;
    std::unordered_map<labeltype, std::vector<float>> original_vectors_;
    
    void create_float_space();
    void create_quantized_space();
    void create_index();

    void addPoint_internal(const float* data, labeltype label);
    std::priority_queue<std::pair<float, labeltype>> searchKnn_internal(
        const float* query, size_t k, bool use_rescoring);
    
    // static float cosine_similarity(const float* a, const float* b, size_t dim);
    // static float l2_distance(const float* a, const float* b, size_t dim);

public:

#if defined(DELAY_ALLOC) && DELAY_ALLOC == 1
   UnifiedIndex(const UnifiedIndexMeta& meta);
#endif

    UnifiedIndex(size_t dim, size_t max_elements, Metric metric = Metric::L2,
                 QuantizationType quantization = QuantizationType::NONE,
                 BinMode bin_mode = BinMode::STANDARD,
                 bool enable_rescoring = false,
                 size_t M = 16, size_t ef_construction = 200,
                 size_t flush_threshold = 10000);
    
    void fit_quantizer(const std::vector<std::vector<float>>& sample_embeddings);
    void addPoint(const float* data, labeltype label);
    
    std::priority_queue<std::pair<float, labeltype>> searchKnn(
        const float* query, size_t k, bool use_rescoring = false);
    
    std::vector<std::pair<float, labeltype>> searchWithStopCondition(
        const float* query, float epsilon, size_t min_cand, size_t max_cand);
    
    void setEf(size_t ef);
    size_t getCurrentElementCount() const;
    void saveIndex(const std::string& path);
    bool loadIndex(const std::string& path, bool SearchOnly = false);

    void clear(); // This removes all elements leaving it empty.
    // How many elements? 
    size_t size() const {
      if (is_quantized()) return quant_index_->cur_element_count ;
      else return float_index_->cur_element_count ;
    }

    size_t bytes_per_vector() const;
    inline size_t bytes_per_index() const { return bytes_per_vector() * max_elements_; }

    inline bool empty() const { return size() == 0; }
    
    bool is_quantized() const { return quantization_ != QuantizationType::NONE; }
    bool is_binary() const { return quantization_ == QuantizationType::BINARY; }
    bool is_ternary() const { return quantization_ == QuantizationType::TERNARY; }
    Metric get_metric() const { return metric_; }
    size_t get_dim() const { return dim_; }
    bool is_rescoring_enabled() const { return enable_rescoring_; }
    const UnifiedIndexMeta& get_meta() const { return meta_; }
};

} // namespace hnswlib
