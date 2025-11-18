#define DELAY_ALLOC  1

#include "unified_hnsw.hpp"
#include <hnswlib/cosine_similarity.h>
#include <hnswlib/l2_distance.h>

#ifdef __ARM_FEATURE_SVE
# include "arm_sve_suport.hpp"
#endif

#define UNIFIED_INDEX_ 0 /* Set to 1 when we activate the code */


namespace hnswlib {

// ============================================================================
// Utility Functions
// ============================================================================
SimdKind detect_simd() {
#ifdef __AVX512F__
    if (AVX512Capable()) return SimdKind::AVX512;
#endif
#if defined(__AVX2__)
    if (AVXCapable()) return SimdKind::AVX2;
#endif
#if defined(__ARM_FEATURE_SVE)
    if (has_sve_runtime()) return SimdKind::SVE;
#endif
#if defined(__ARM_NEON)
    return SimdKind::NEON;
#endif
    return SimdKind::NONE;
}



// ============================================================================
// UnifiedIndex Implementation - Helper Functions
// ============================================================================

void UnifiedIndex::create_space() {
    if (dim_ == 0) {
       throw std::runtime_error("Zero (0) dimension vector space specified!!!!");
       return; // This is evil
    }

    if (is_quantized())
        create_quantized_space();
    else
        create_float_space();

}

void UnifiedIndex::create_float_space() {
    switch (metric_) {
        case Metric::L1:
           space_ = std::make_unique<hnswlib::L1Space>(dim_);
           break;
        case Metric::L2:
            space_ = std::make_unique<hnswlib::L2Space>(dim_);
            break;
        case Metric::Cosine:
            normalize_ = true;
        case Metric::IP:
            space_ = std::make_unique<hnswlib::InnerProductSpace>(dim_);
            break;
    }
}

void UnifiedIndex::create_quantized_space() {
    switch (metric_) {
        case Metric::L1:
           throw std::runtime_error ("L1 Space Quantization is NOT SUPPORTED!");
           break;
        case Metric::L2:
	   space_ = std::make_unique<hnswlib::SpaceQuantized<float>>(dim_, quantization_, bin_mode_);
           break;
        case Metric::Cosine:
           normalize_ = true;
        case Metric::IP:
           space_ = std::make_unique<hnswlib::SpaceQuantizedIP<float>>(dim_, quantization_, bin_mode_);
           break;
    }

}

void UnifiedIndex::create_index() {
    if (!space_) create_space();
    index_ = std::make_unique<HierarchicalNSW<float>>( space_.get(), max_elements_, M_, ef_construction_);
}


// ============================================================================
// UnifiedIndex Implementation - Constructor
// ============================================================================


#if DELAY_ALLOC == 1
UnifiedIndex::UnifiedIndex(const UnifiedIndexMeta& meta) : meta_(meta) {
  normalize_ =  (metric_ == Metric::Cosine);
  vector_storage_.set_dim(dim_);
  
}
#endif

UnifiedIndex::UnifiedIndex(size_t dim, size_t max_elements, Metric metric,
    QuantMode quantization, OptBinMode bin_mode, bool enable_rescoring,
    size_t M, size_t ef_construction, size_t flush_threshold) {
    
    dim_ = dim;
    max_elements_ = max_elements;
    metric_ = metric;
    quantization_ = quantization;
    bin_mode_ = bin_mode;
    enable_rescoring_ = enable_rescoring;
    M_ = M;
    ef_construction_ = ef_construction;
    ef_ = 10;

    flush_threshold_ = flush_threshold;
 
    normalize_ = (metric == Metric::Cosine);

    vector_storage_.set_dim(dim_);

#if DELAY_ALLOC == 0
    create_index();
#endif
}

// ============================================================================
// UnifiedIndex Implementation - Public Methods
// ============================================================================

void UnifiedIndex::fit_quantizer(const std::vector<std::vector<float>>& sample_embeddings) {
    if (!space_) create_space(); 

  //// Train later when you have data
  // std::vector<std::vector<float>> samples = get_training_samples();
  // index->fit_quantizer(samples);  // Now fully trained!

    if (quantization_ != QuantMode::NONE) {
        space_->fit(sample_embeddings);
        quantizer_fitted_ = true;
    }
}


// NOTE: We now normalize here. When folded in we must
// remove the normalization for encode_text.
// TODO!!!!
void UnifiedIndex::addPoint(const float* data, labeltype label) {
    if (normalize_) {
        std::vector<float> normalized(data, data + dim_);
        normalize_l2(normalized.data(), dim_);
        addPoint_internal(normalized.data(), label);
    } else {
        addPoint_internal(data, label);
    }
}

void UnifiedIndex::addPoint_internal(const float* data, labeltype label) {
#if DELAY_ALLOC == 1
    // Make sure we have an index, create if needed
    if (!index_) create_index ();
#endif 
    index_->addPoint(data, label);

    if (enable_rescoring_) {
        original_vectors_[label] = std::vector<float>(data, data + dim_);
    }
    additions_since_flush_++;
}

std::priority_queue<std::pair<float, labeltype>> UnifiedIndex::searchKnn(
    const float* query, size_t k, bool use_rescoring) {
    
    if (normalize_) {
        std::vector<float> normalized(query, query + dim_);
        normalize_l2(normalized.data(), dim_);
        return searchKnn_internal(normalized.data(), k, use_rescoring);
    } else {
        return searchKnn_internal(query, k, use_rescoring);
    }
}

std::priority_queue<std::pair<float, labeltype>> UnifiedIndex::searchKnn_internal(
    const float* query, size_t k, bool use_rescoring) {
    
    if (quantization_ == QuantMode::NONE) {
        return index_->searchKnn(query, k);
    }
    
    std::vector<uint8_t> quantized;
    quantized.resize(space_->get_bytes_per_vector());
    space_->quantize(query, quantized.data());
    
    if (!use_rescoring || !enable_rescoring_) {
        auto results = index_->searchKnn(quantized.data(), k);
        std::priority_queue<std::pair<float, labeltype>> converted;
        while (!results.empty()) {
            auto [dist, label] = results.top();
            results.pop();
            converted.emplace(static_cast<float>(dist), label);
        }
        return converted;
    }
    
    size_t rescore_factor = std::max(size_t(3), k * 3);
    size_t num_candidates = std::min(rescore_factor, index_->getCurrentElementCount());
    
    auto binary_results = index_->searchKnn(quantized.data(), num_candidates);
    
    std::vector<std::pair<float, labeltype>> rescored;
    while (!binary_results.empty()) {
        auto [hamming_dist, label] = binary_results.top();
        binary_results.pop();
        
        if (original_vectors_.find(label) != original_vectors_.end()) {
            float dist;
            if (metric_ == Metric::Cosine || metric_ == Metric::IP) {
                float sim = hnswlib::cosine_similarity(query, original_vectors_[label].data(), dim_);
                dist = -sim;
            } else {
                dist = l2_distance(query, original_vectors_[label].data(), dim_);
            }
            rescored.emplace_back(dist, label);
        }
    }
    
    std::sort(rescored.begin(), rescored.end());
    
    std::priority_queue<std::pair<float, labeltype>> result;
    for (size_t i = 0; i < std::min(k, rescored.size()); i++) {
        result.push(rescored[i]);
    }
    
    return result;
}

std::vector<std::pair<float, labeltype>> UnifiedIndex::searchWithStopCondition(
    const float* query, float epsilon, size_t min_cand, size_t max_cand) {
    
    std::vector<float> query_normalized;
    const float* query_ptr = query;
    
    if (normalize_) {
        query_normalized.assign(query, query + dim_);
        normalize_l2(query_normalized.data(), dim_);
        query_ptr = query_normalized.data();
    }
    
    std::vector<std::pair<float, labeltype>> results;
    
    if (quantization_ == QuantMode::NONE) {
        size_t original_ef = index_->ef_;
        index_->setEf(max_cand);
        
        auto candidates = index_->searchKnn(query_ptr, max_cand);
        index_->setEf(original_ef);
        
        if (candidates.empty()) return results;
        
        float best_dist = std::numeric_limits<float>::max();
        std::vector<std::pair<float, labeltype>> all_candidates;
        
        while (!candidates.empty()) {
            auto [dist, label] = candidates.top();
            candidates.pop();
            all_candidates.emplace_back(dist, label);
            best_dist = std::min(best_dist, dist);
        }
        
        for (const auto& [dist, label] : all_candidates) {
            if (dist <= best_dist + epsilon) {
                results.emplace_back(dist, label);
            }
        }
        
        if (results.size() < min_cand) {
            std::sort(all_candidates.begin(), all_candidates.end());
            results.clear();
            for (size_t i = 0; i < std::min(min_cand, all_candidates.size()); i++) {
                results.push_back(all_candidates[i]);
            }
        }
        
    } else {
        std::vector<uint8_t> quantized;
        quantized.resize(space_->get_bytes_per_vector() );
        space_->quantize(query_ptr, quantized.data());
        
        size_t original_ef = index_->ef_;
        index_->setEf(max_cand);
        
        auto candidates = index_->searchKnn(quantized.data(), max_cand);
        index_->setEf(original_ef);
        
        if (candidates.empty()) return results;
        
        float best_dist = std::numeric_limits<float>::max();
        std::vector<std::pair<float, labeltype>> all_candidates;
        
        while (!candidates.empty()) {
            auto [dist, label] = candidates.top();
            candidates.pop();
            float fdist = static_cast<float>(dist);
            all_candidates.emplace_back(fdist, label);
            best_dist = std::min(best_dist, fdist);
        }
        
        if (enable_rescoring_) {
            std::vector<std::pair<float, labeltype>> rescored;
            for (const auto& [dist, label] : all_candidates) {
                if (original_vectors_.find(label) != original_vectors_.end()) {
                    float true_dist;
                    if (metric_ == Metric::Cosine || metric_ == Metric::IP) {
                        float sim = hnswlib::cosine_similarity(query_ptr, original_vectors_[label].data(), dim_);
                        true_dist = -sim;
                    } else {
                        true_dist = l2_distance(query_ptr, original_vectors_[label].data(), dim_);
                    }
                    rescored.emplace_back(true_dist, label);
                }
            }
            
            if (!rescored.empty()) {
                std::sort(rescored.begin(), rescored.end());
                best_dist = rescored[0].first;
                
                for (const auto& [dist, label] : rescored) {
                    if (dist <= best_dist + epsilon) {
                        results.emplace_back(dist, label);
                    }
                }
                
                if (results.size() < min_cand) {
                    results.clear();
                    for (size_t i = 0; i < std::min(min_cand, rescored.size()); i++) {
                        results.push_back(rescored[i]);
                    }
                }
                
                return results;
            }
        }
        
        for (const auto& [dist, label] : all_candidates) {
            if (dist <= best_dist + epsilon) {
                results.emplace_back(dist, label);
            }
        }
        
        if (results.size() < min_cand) {
            std::sort(all_candidates.begin(), all_candidates.end());
            results.clear();
            for (size_t i = 0; i < std::min(min_cand, all_candidates.size()); i++) {
                results.push_back(all_candidates[i]);
            }
        }
    }
    
    return results;
}

void UnifiedIndex::setEf(size_t ef) {
    ef_ = ef;
    if (index_) index_->setEf(ef);
}

size_t UnifiedIndex::getCurrentElementCount() const {
    if (index_) return index_->getCurrentElementCount();
    return 0;
}

void UnifiedIndex::saveIndex(const std::string& path) {
    if (additions_since_flush_ == 0) return; // Nothing to do yet
    if (!space_ || !index_) {
       HNSWERR << "Unintialized Index. Nothing to save in '" << path << "'!";
       return;
    }

    std::ofstream ofs(path, std::ios::binary);
    if (!ofs.is_open()) {
        HNSWERR << "Can't save index: '" << path << "' cannot be opened for writing";
        return; // Can't continue
    }
    meta_.save(ofs);

    space_->save_quantization_params(ofs);
    
    if (enable_rescoring_) {
        size_t num_vectors = original_vectors_.size();
        ofs.write(reinterpret_cast<const char*>(&num_vectors), sizeof(size_t));
        
        for (const auto& [label, vec] : original_vectors_) {
            ofs.write(reinterpret_cast<const char*>(&label), sizeof(labeltype));
            ofs.write(reinterpret_cast<const char*>(vec.data()), dim_ * sizeof(float));
        }
    }

    index_->saveIndex(ofs);
    ofs.close();
}

bool UnifiedIndex::loadIndex(const std::string& path, bool searchOnly) {
    bool changed = false;
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open()) {
        HNSWERR << "Cannot load index: '" << path << "' cannot be opened for reading";
	return false;
    }
    
    UnifiedIndexMeta loaded_meta;
    if (!loaded_meta.load(ifs)) return false;

    if (loaded_meta.dim_ != dim_) {
#if DELAY_ALLOC == 1
	// dim_ == 0 means we wanted to load something
	if (dim_ != 0) HNSWERR << "Dimension mismatch. Expected " << std::to_string(dim_) <<
                " but got " << std::to_string(loaded_meta.dim_) << std::endl ;
	changed = true;
#else
	throw std::runtime_error("Dimension mismatch. Expected " + std::to_string(dim_) +
                " but got " + std::to_string(loaded_meta.dim_));
	return false;
#endif
    }

    // By specifying max_elements_ = 0 we get a allocator
    size_t  max_elements = max_elements_;
    meta_ = loaded_meta;

    // We want the max of the configured and stored.
    if (max_elements > max_elements_) {
       max_elements_ = max_elements;
       changed = true;
    }
#if 0
    // Check that the index capacity is sufficient
    auto [element_count, max_from_file] = peek_index_elements(ifs);
    if (element_count > max_elements_) {
       if (searchOnly) max_elements_ = elements_count;
       else            max_elements_ = max_from_file;
       changed = true;
    }
#endif
    

#if DELAY_ALLOC == 1
    // Make sure we have an index, create if needed
     if (changed || !space_) {
	create_space(); // We force a create since dim might be different!
        create_index();
    }
#endif  
    space_->load_quantization_params(ifs);
    
    if (loaded_meta.enable_rescoring_) {
        size_t num_vectors;
        ifs.read(reinterpret_cast<char*>(&num_vectors), sizeof(size_t));
        
        if (enable_rescoring_) {
            original_vectors_.clear();
            for (size_t i = 0; i < num_vectors; i++) {
                labeltype label;
                ifs.read(reinterpret_cast<char*>(&label), sizeof(labeltype));
                std::vector<float> vec(dim_);
                ifs.read(reinterpret_cast<char*>(vec.data()), dim_ * sizeof(float));
		//
		if (!ifs.good()) return false;
		//
                original_vectors_[label] = std::move(vec);
            }
        } else {
            for (size_t i = 0; i < num_vectors; i++) {
                ifs.seekg(sizeof(labeltype) + dim_ * sizeof(float), std::ios::cur);
            }
        }
    }
    
    create_index(); // If index exists we write over it!

    index_->loadIndex(ifs, space_.get(), meta_.max_elements_);
    index_->setEf(meta_.ef_);

    ifs.close();
    return true;
}

// This removes all elements leaving it empty.
void UnifiedIndex::clear() {
   // We re-use the space
   if (is_quantized()) original_vectors_.clear();
   create_index();
}   

// Peek at the index file to get element count
std::pair<size_t, size_t> peek_index_elements(std::istream& ifs) {
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

// Look at a stored index file and fetch its
// <curent_element_count, max_elements>
// We use this to read an index BEFORE we create a Unified Index to make
// use that we allocate suitable sizes
std::pair<size_t, size_t> peek_index_elements(const std::string path) {
    std::ifstream input(path, std::ios::binary);
    if (input.is_open()) {
      input.seekg(UnifiedIndexMeta::size());
      auto result = peek_index_elements(input);
      input.close();
      return result;
    }   
    return {};
}


/*
Dimension of Common SBERT Models:

all-MiniLM-L6-v2            384    Most popular, fast, good quality
all-mpnet-base-v2           768    Higher quality, slower
all-MiniLM-L12-v2           384    Balance of speed/quality
paraphrase-MiniLM-L6-v2     384    Paraphrase detection
paraphrase-mpnet-base-v2    768    Paraphrase detection
multi-qa-MiniLM-L6-cos-v1   384    Question answering
multi-qa-mpnet-base-cos-v1  768    Question answering

*/

size_t UnifiedIndex::bytes_per_vector() const
{
    size_t vector_bytes = dim_ * sizeof(float);
    size_t graph_overhead = M_ * 10;  // ~160 bytes for M=16

    if (is_quantized()) {
       size_t quant_bytes =  space_->get_data_size();

       size_t total = quant_bytes + graph_overhead;
        if (enable_rescoring_) {
            total += vector_bytes;  // Add original 1536 bytes for 384D
        }
        return total;
    } else {
        // Float metrics
        size_t graph_overhead = M_ * 10;  // ~160 bytes for M=16
        return vector_bytes + graph_overhead;
    }
}


std::string metric_to_string(Metric m) {
  switch(m) {
      case Metric::L1:     return "L1";
      case Metric::L2:     return "L2";
      case Metric::IP:     return "IP";
      case Metric::Cosine: return "Cosine";
      default: return "Unknown";
   }
}

Metric string_to_metric(const std::string& s) {
   if (s.empty()) throw std::runtime_error("Empty distance metric name.");

   const char ch = s.at(0);
   if (s == "L1" || s == "l1" || ch == 'M' || ch == 'm')
        return Metric::L1; // Manhatttan
   if (s == "L2" || s == "l2" || ch == 'E' || ch == 'e')
        return Metric::L2; // Eucledian
   if (ch == 'I' || ch  == 'i')
        return Metric::IP; // InnerProduct
   if (ch == 'C' || ch == 'c')
        return Metric::Cosine;
    throw std::runtime_error("Unknown distance metric: " + s);
}


QuantMode  string_to_quantzation(const std::string &s) {
   if (s.empty()) throw std::runtime_error("Empty quantization name.");
   if (s == "Binary" || s == "BINARY" || s == "BIN1" || s == "INT1" || s.at(0) == 'b')
	return QuantMode::BIN1;
   if (s == "Ternary" || s == "1.58" || s == "INT158")
	return QuantMode::INT158;
   if (s == "Nibble" || s == "INT4" || s == "Tetrade" || s == "Semioctet")
	return QuantMode::INT4;
   if (s == "Octet" || s == "INT8" || s == "Quarter" || s.at(0) == 'o')
	return QuantMode::INT8;
   if (s == "Fp32" || s == "FLOAT32" || s == "NONE")
	return QuantMode::NONE;
    throw std::runtime_error("Unknown/unsupported quantization: " + s);
}

std::string quantization_to_string(QuantMode mode)
{
   switch (mode) {
     case QuantMode::NONE:   return "Fp32";
     case QuantMode::BIN1:   return "Binary";
     case QuantMode::INT158: return "Ternary";
     case QuantMode::INT4:   return "Nibble";
     case QuantMode::INT8:   return "Octet";
   }
}

// PASS means the Float32 vectors were already quantized!
OptBinMode  string_to_bin_mode(const std::string& s) {
   if (s == "pass")       return OptBinMode::PASS;
   if (s == "standard")   return OptBinMode::STANDARD;
   if (s == "better")     return OptBinMode::BETTER;
   if (s == "centroid")   return OptBinMode::CENTROID;
   if (s == "rotational") return OptBinMode::ROTATIONAL;
   if (s == "RaBitQ")     return OptBinMode::RABITQ;
   if (s == "RaBitQ-Ex")  return OptBinMode::RABITQ_EXTENDED; 
   throw std::runtime_error("Unknown/unsupported bin_mode: " + s);
}

std::string bin_mode_to_string(OptBinMode mode)
{   
   switch (mode) {
      case OptBinMode::PASS:            return "pass";
      case OptBinMode::STANDARD:        return "standard" ;
      case OptBinMode::BETTER:          return "better";
      case OptBinMode::CENTROID:        return "centroid";
      case OptBinMode::ROTATIONAL:      return "rotational";
      case OptBinMode::RABITQ:          return "RaBitQ";
      case OptBinMode::RABITQ_EXTENDED: return "RaBitQ-Ex";
   }
  // NOT REACHED
  return "";
}



}; // namespace hnswlib
