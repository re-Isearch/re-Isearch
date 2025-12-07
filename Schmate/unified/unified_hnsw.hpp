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

// Conversion to and from string names
std::string metric_to_string(Metric m);
Metric string_to_metric(const std::string& s);

// Quantization Mode conversions to and from string names
QuantMode  string_to_quantzation(const std::string &s);
std::string quantization_to_string(QuantMode mode);

OptBinMode  string_to_bin_mode(const std::string& s);
std::string bin_mode_to_string(OptBinMode mode);


StorageType string_to_storage_type(const std::string& s);
std::string storage_type_to_string(StorageType type) ;

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
// Parse a Specification String: Metric, Storage,..
// ============================================
class SpecificationString {
public:
   Metric      metric_ = Metric::L2;
   QuantMode   quantization_ = QuantMode::NONE;
   OptBinMode  mode_ = OptBinMode::PASS ;
   StorageType storage_type_ = StorageType::FLOAT32;
   SpecificationString(const std::string& str) {
     parse(str);
   }
   bool parse(const std::string& s);
   operator std::string() const;
private:
    bool use_storage_ = false; // true if PASS case
} ;

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
    size_t flush_threshold_ =100000;

    QuantMode quantization_ = QuantMode::NONE;
    OptBinMode bin_mode_ = OptBinMode::STANDARD;

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
	QuantMode quantization = QuantMode::NONE,
	OptBinMode bin_mode = OptBinMode::STANDARD,
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

//void normalize_l2(float* vec, size_t dim);
//void normalize_l2_batch(std::vector<std::vector<float>>& embeddings);

// ============================================
// UNIFIED INDEX (supports all metrics)
// ============================================


class UnifiedIndex {
private:
    std::string pathname_;
    size_t additions_since_flush_ = 0;
    size_t &flush_threshold_ = meta_.flush_threshold_;

    UnifiedIndexMeta meta_;
    Metric &metric_ = meta_.metric_;
    size_t &dim_ = meta_.dim_;
    bool &enable_rescoring_ = meta_.enable_rescoring_;
    bool &normalize_ = meta_.normalize_;
    QuantMode &quantization_ = meta_.quantization_;
    OptBinMode &bin_mode_ = meta_.bin_mode_;
    bool   &quantizer_fitted_ = meta_.quantizer_fitted_;
    // 
    size_t &max_elements_ = meta_.max_elements_;
    size_t &M_ = meta_.M_;
    size_t &ef_construction_ = meta_.ef_construction_;
    size_t &ef_ = meta_.ef_;

    LSMVectorStorage vector_storage_;
    
    std::unique_ptr<HierarchicalNSW<float>> index_;
    std::unique_ptr<SpaceInterface<float>> space_;

    // std::unordered_map<labeltype, std::vector<float>> original_vectors_;
    
    void create_space();
    void create_index();

    void create_float_space();
    void create_quantized_space();

    void addPoint_internal(const float* data, labeltype label);

    std::priority_queue<std::pair<float, labeltype>> searchKnn_internal(
        const float* query, size_t k, bool use_rescoring);

    std::vector<std::pair<float, labeltype>> searchKnnCloserFirst_internal(
	const float* query, size_t k, BaseFilterFunctor* isIdAllowed, bool use_rescoring) const;

    
    // static float cosine_similarity(const float* a, const float* b, size_t dim);
    // static float l2_distance(const float* a, const float* b, size_t dim);

public:

#if defined(DELAY_ALLOC) && DELAY_ALLOC == 1
   UnifiedIndex(const UnifiedIndexMeta& meta);
#endif

   UnifiedIndex(size_t dim, size_t max_elements, 
	const std::string& specification, bool enable_rescoring = false,
        size_t M = 16, size_t ef_construction = 200, size_t flush_threshold = 10000);


    UnifiedIndex(size_t dim, size_t max_elements, Metric metric = Metric::L2,
                 QuantMode quantization = QuantMode::NONE,
                 OptBinMode bin_mode = OptBinMode::STANDARD,
                 bool enable_rescoring = false,
                 size_t M = 16, size_t ef_construction = 200,
                 size_t flush_threshold = 10000);

    void fit(const std::vector<std::vector<float>>& sample_embeddings);
    void addPoint(const float* data, labeltype label);
    
    // We don't want to use searchKnn but the CloserFirst variant!
    std::priority_queue<std::pair<float, labeltype>> searchKnn(
        const float* query, size_t k, bool use_rescoring = false);

    std::vector<std::pair<float, labeltype>> searchWithStopCondition(
        const float* query, float epsilon, size_t min_cand, size_t max_cand);

    std::vector<std::pair<float, labeltype>>
        searchKnnCloserFirst(const float* query, size_t k, bool use_rescoring) const;

    std::vector<std::pair<float, labeltype>>
        searchKnnCloserFirst(const float* query, size_t k,
        BaseFilterFunctor* isIdAllowed = nullptr, bool use_rescoring = false) const;


    std::vector<std::pair<float, labeltype>> apply_rescoring( const float* query,
	const std::vector<std::pair<float, labeltype>>& candidates) const;
    
    void setEf(size_t ef);
    size_t getCurrentElementCount() const;

    void        set_filepath(const std::string& path) { pathname_ = path; }
    std::string get_filepath() const                  { return pathname_; }


    bool save();
    bool saveIndex(const std::string& path);
    bool load(bool SearchOnly = false);
    bool loadIndex(const std::string& path, bool SearchOnly = false);

    bool flush();

    // Get vector for rescoring
    const float* getOriginalVector(labeltype label) const;
    void printStats() const; // Only relevant with LSM VectorStorage

    void clear(); // This removes all elements leaving it empty.
    // How many elements? 
    size_t size() const { return index_->cur_element_count ; }

    size_t bytes_per_vector() const;
    inline size_t bytes_per_index() const { return bytes_per_vector() * max_elements_; }

    inline bool empty() const { return size() == 0; }
    
    bool is_quantized() const { return quantization_ != QuantMode::NONE; }
    Metric get_metric() const { return metric_; }
    size_t get_dim() const { return dim_; }
    bool is_rescoring_enabled() const { return enable_rescoring_; }
    const UnifiedIndexMeta& get_meta() const { return meta_; }

    inline bool is_binary() { return  (quantization_ == QuantMode::BIN1); };
    inline bool is_ternary() { return (quantization_ == QuantMode::INT158); };

    float score_from_dist(float dist) const;

static inline std::vector<std::pair<float, size_t>>
sort_best_first(std::vector<std::pair<float, size_t>> &res_vector) {
    return res_vector; // already sorted
}

static inline std::vector<std::pair<float, size_t>>
sort_best_first(const std::priority_queue<std::pair<float,size_t>>& pq) {
    auto tmp = pq;
    std::vector<std::pair<float,size_t>> out;
    out.reserve(tmp.size());
    while (!tmp.empty()) {
        out.push_back(tmp.top());
        tmp.pop();
    }
    std::reverse(out.begin(), out.end());
    return out;
}

};

} // namespace hnswlib
