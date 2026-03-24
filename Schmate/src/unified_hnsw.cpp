#define DELAY_ALLOC  1

#include "unified_hnsw.hpp"
#include <hnswlib/cosine_similarity.h>
#include <hnswlib/l2_distance.h>

#ifdef __ARM_FEATURE_SVE
# include "arm_sve_suport.hpp"
#endif

#define UNIFIED_INDEX_ 0 /* Set to 1 when we activate the code */

#define LSMVECTORSTORAGE 1

/*

Our HNSW design assumes that each index contains fewer than 10 million vectors. Recall that each index represents only the contents of a specific field or path, and that a single document may contain many such fields.
In the worst-case scenario—such as a large textual document where each sentence (including headings) is embedded separately—the total number of sentences effectively corresponds to the entire document. Even in this case, 10 million sentences correspond to approximately 170–210 million words. For our target use cases, this capacity is more than sufficient. Assuming an average of 600 words per document, this supports 280K+ documents per index.
Additionally, we support sharding, allowing this capacity to scale easily to millions of documents per index while maintaining strong performance.
More generally, documents rarely provide complete coverage for a given field. As a result, the practical capacity of an index is typically significantly larger than this worst-case estimate.

*/

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
       space_.reset();
       throw std::runtime_error("Zero (0) dimension vector space specified!!!!");
       return; // This is evil
    }

    if (is_quantized()) {
        create_quantized_space();
    } else {
        create_float_space();
    }

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
  vector_storage_.set_storage_mode(storage_mode_);

#if 1
  // XXXX DEBUG
  meta_.print(); 
#endif

}
#endif

UnifiedIndex::UnifiedIndex(size_t dim, size_t max_elements, 
     const std::string& specification, bool enable_rescoring,
     size_t M, size_t ef_construction, size_t flush_threshold)
{
    SpecificationString spec (specification);
    dim_ = dim;
    max_elements_ = max_elements;
    metric_ = spec.metric_;
    quantization_ = spec.quantization_;
    storage_type_ = spec.storage_type_;
    bin_mode_ = spec.mode_;
    enable_rescoring_ = enable_rescoring && (storage_mode_ != VectorStorageMode::DISABLED);

    M_ = M;
    ef_construction_ = ef_construction;
    ef_ = 10;
    flush_threshold_ = flush_threshold;
    normalize_ = (metric_ == Metric::Cosine);
    vector_storage_.set_dim(dim_);
    vector_storage_.set_storage_mode(storage_mode_);
#if DELAY_ALLOC == 0
    create_index();
#endif

}

UnifiedIndex::UnifiedIndex(size_t dim, size_t max_elements, Metric metric,
    QuantMode quantization, OptBinMode bin_mode, bool enable_rescoring,
    size_t M, size_t ef_construction, size_t flush_threshold) {
    
    dim_ = dim;
    max_elements_ = max_elements;
    metric_ = metric;
    quantization_ = quantization;
    storage_type_ =  toStorageType( quantization_ );
    bin_mode_ = bin_mode;
    enable_rescoring_ = enable_rescoring && (storage_mode_ != VectorStorageMode::DISABLED);

    M_ = M;
    ef_construction_ = ef_construction;
    ef_ = 10;

    flush_threshold_ = flush_threshold;
 
    normalize_ = (metric == Metric::Cosine);

    vector_storage_.set_dim(dim_);
    vector_storage_.set_storage_mode(storage_mode_);

#if DELAY_ALLOC == 0
    create_index();
#endif
}

// ============================================================================
// UnifiedIndex Implementation - Public Methods
// ============================================================================

void UnifiedIndex::fit(const std::vector<std::vector<float>>& sample_embeddings) {
    if (!space_) create_space(); 

  //// Train later when you have data
  // std::vector<std::vector<float>> samples = get_training_samples();
  // index->fit(samples);  // Now fully trained!

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

    if (is_quantized()) {
      // Need to quantise "
      std::vector<uint8_t> coded(space_->get_data_size());
      space_->quantize(data, coded.data());
      index_->addPoint(coded.data(), label);
    } else
      index_->addPoint(data, label);

    if (enable_rescoring_) {
#if LSMVECTORSTORAGE
       vector_storage_.addPoint(label, data);
#else
        original_vectors_[label] = std::vector<float>(data, data + dim_);
#endif
    }
    if (++additions_since_flush_ > flush_threshold_ )
      flush();
}


bool UnifiedIndex::flush() {
   if (additions_since_flush_ == 0) return true;

   // Could do something here ...

   additions_since_flush_ = 0;
   return true;
}

// Get vector for rescoring
const float* UnifiedIndex::getOriginalVector(labeltype label) const {
    if (!enable_rescoring_) return nullptr;
#if LSMVECTORSTORAGE
    return vector_storage_.get_vector(label);
#else
    return original_vectors_[label].first;
#endif
}


std::vector<std::pair<float, labeltype>>
        UnifiedIndex::searchKnnCloserFirst(const float* query, size_t k,
	bool use_rescoring) const {
    return searchKnnCloserFirst(query, k, nullptr, use_rescoring);
}

std::vector<std::pair<float, labeltype>>
	UnifiedIndex::searchKnnCloserFirst(const float* query, size_t k,
	BaseFilterFunctor* isIdAllowed, bool use_rescoring) const {

    if (!index_) return {};
 
    if (normalize_) {
        std::vector<float> normalized(query, query + dim_);
        normalize_l2(normalized.data(), dim_);
	return searchKnnCloserFirst_internal(normalized.data(), k, isIdAllowed, use_rescoring);
    }

    return searchKnnCloserFirst_internal(query, k, isIdAllowed, use_rescoring);
}

std::vector<std::pair<float, labeltype>>
        UnifiedIndex::searchKnnCloserFirst_internal(const float* query, size_t k,
        BaseFilterFunctor* isIdAllowed, bool use_rescoring) const {

    if (quantization_ == QuantMode::NONE) {
        return index_->searchKnnCloserFirst(query, k, isIdAllowed);
    }


    std::vector<uint8_t> quantized;
    quantized.resize(space_->get_data_size()); // was get_bytes_per_vector());
    space_->quantize(query, quantized.data());

    // With PASS (pass-through we NEVER rescore)
    if (!use_rescoring || bin_mode_ == OptBinMode::PASS) {
	return index_->searchKnnCloserFirst(quantized.data(), k, isIdAllowed);
    }

     // Get more candidates for rescoring
     size_t rescore_factor = std::max(size_t(3), k * 3);
     size_t num_candidates = std::min(rescore_factor, index_->getCurrentElementCount());

     auto candidates = index_->searchKnnCloserFirst(quantized.data(), num_candidates, isIdAllowed);

     // Rescore using original vectors
     auto rescored = apply_rescoring(query, candidates);
        
     // Return top-k
     if (rescored.size() > k) {
	rescored.resize(k);
     }
     return rescored;
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
    quantized.resize(space_->get_data_size()); // was get_bytes_per_vector());
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

#if 1
        const float *data = getOriginalVector(label);
        if (data != nullptr) {
            float dist; 
            if (metric_ == Metric::Cosine || metric_ == Metric::IP) {
                float sim = hnswlib::cosine_similarity(query, data, dim_);
                dist = -sim;
            } else { 
                dist = l2_distance(query, data, dim_);
            }
            rescored.emplace_back(dist, label);
       }
#else
        
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
#endif
    }
    std::sort(rescored.begin(), rescored.end());
    
    std::priority_queue<std::pair<float, labeltype>> result;
    for (size_t i = 0; i < std::min(k, rescored.size()); i++) {
        result.push(rescored[i]);
    }
    return result;
}

// Rescore candidates
std::vector<std::pair<float, labeltype>> UnifiedIndex::apply_rescoring(
    const float* query_ptr,
    const std::vector<std::pair<float, labeltype>>& all_candidates) const {

    if (!enable_rescoring_) {
        return  all_candidates;
    }

    std::vector<std::pair<float, labeltype>> rescored;
    rescored.reserve(all_candidates.size());

    for (const auto& [dist, label] : all_candidates) {
        const float* vec_ptr = getOriginalVector(label); 

        if (vec_ptr != nullptr) {
            float true_dist;
            if (metric_ == Metric::Cosine || metric_ == Metric::IP) {
                float sim = hnswlib::cosine_similarity(query_ptr, vec_ptr, dim_);
                true_dist = -sim;
            } else {
                true_dist = l2_distance(query_ptr, vec_ptr, dim_);
            }
            rescored.emplace_back(true_dist, label);
        } else {
            // Fallback to approximate distance if vector not found
            rescored.emplace_back(dist, label);
        }
    }

   // Sort by distance (closest first)
   std::sort(rescored.begin(), rescored.end());

   return rescored;
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
#if 1
               const float *data = getOriginalVector(label);
               if (data != nullptr) {
                  float true_dist;
                    if (metric_ == Metric::Cosine || metric_ == Metric::IP) {
                        float sim = hnswlib::cosine_similarity(query_ptr, data, dim_);
                        true_dist = -sim;
                    } else {
                        true_dist = l2_distance(query_ptr, data, dim_);
                    } 
                    rescored.emplace_back(true_dist, label);
                }
#else
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
#endif
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

bool UnifiedIndex::save() {
  if (pathname_.empty()) HNSWERR << "Can't flush index: no path yet defined!\n";
  return saveIndex(pathname_);
}

bool UnifiedIndex::saveIndex(const std::string& path) {
    const size_t  additions = additions_since_flush_;
    if (path.empty()) return false;
    if (pathname_.empty()) pathname_ = path; 

    if (additions_since_flush_ == 0) return true; // Nothing to do yet
    if (!space_ || !index_) {
       HNSWERR << "Unintialized Index. Nothing to save in '" << path << "'!\n";
       return false;
    }

    std::ofstream ofs(path, std::ios::binary);
    if (!ofs.is_open()) {
        HNSWERR << "Can't save index: '" << path << "' cannot be opened for writing\n";
        return false; // Can't continue
    }
    // The meta carries also the magic number for the index
    meta_.save(ofs);

    if (meta_.enable_rescoring_ != enable_rescoring_) HNSWERR << "Serious logic error: " << __func__  << "()!!!!!!!\n";

    space_->save_quantization_params(ofs);

    if (enable_rescoring_) {

#if LSMVECTORSTORAGE
        // vector_storage_.compact();
        if (! vector_storage_.save_vectors_to_stream(ofs)) {
	    HNSWERR << "Save vectors to '" << path << "' FAILED.\n";
	    return false; // Write failed;
        }
#else
        size_t num_vectors = original_vectors_.size();
        ofs.write(reinterpret_cast<const char*>(&num_vectors), sizeof(size_t));
        
        for (const auto& [label, vec] : original_vectors_) {
            ofs.write(reinterpret_cast<const char*>(&label), sizeof(labeltype));
            ofs.write(reinterpret_cast<const char*>(vec.data()), dim_ * sizeof(float));
        }
#endif
    }

    index_->saveIndex(ofs);

    ofs.close();

    additions_since_flush_ -= additions;

    return true;
}

bool UnifiedIndex::load(bool searchOnly) {
   return loadIndex(pathname_, searchOnly) ;
}

bool UnifiedIndex::loadIndex(const std::string& path, bool searchOnly) {
    if (path.empty()) return false;

    if (pathname_.empty()) pathname_ = path; 

    bool changed = false;

    if (file_size(pathname_) < sizeof(UnifiedIndexMeta))
       return false;

    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open()) {
        HNSWERR << "Cannot load index: '" << path << "' cannot be opened for reading: " << strerror(errno) << "\n";
	return false;
    }
    
    // 1. Load Metadata
    UnifiedIndexMeta loaded_meta;

    if (!loaded_meta.load(ifs)) return false;

    if (loaded_meta.dim_ != dim_) {
#if DELAY_ALLOC == 1
	// dim_ == 0 means we wanted to load something
	if (dim_ != 0) HNSWERR << "Dimension mismatch. Expected " << std::to_string(dim_) <<
                " but got " << std::to_string(loaded_meta.dim_) << "\n" ;
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

   // 2. Load Quantisation Parameters
   space_->load_quantization_params(ifs);


    // 3. Load Vectors
    if (loaded_meta.enable_rescoring_) {

#if LSMVECTORSTORAGE
      // if rescoring: Pass the open stream (reads labels, then mmaps)
      // else skip
      vector_storage_.load_vectors(path, ifs,
	enable_rescoring_ ?  storage_mode_ : VectorStorageMode::DISABLED);
#else
        size_t num_vectors;
        ifs.read(reinterpret_cast<char*>(&num_vectors), sizeof(size_t));

        // Know now the number of vectors
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
	    ifs.seekg( num_vectors * (sizeof(labeltype) + dim_ * sizeof(float)), std::ios::cur) ;
        }
#endif
    }

    // 4. Load HNSW Index
    create_index(); // If index exists we write over it!
    index_->loadIndex(ifs, space_.get(), meta_.max_elements_);
    index_->setEf(meta_.ef_);


    ifs.close();
    return true;
}

// This removes all elements leaving it empty.
void UnifiedIndex::clear() {
   // We re-use the space
   if (is_quantized()) {
#if LSMVECTORSTORAGE
     vector_storage_.clear();
#else
     original_vectors_.clear();
#endif
   }
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
    const size_t vector_bytes = dim_ * sizeof(float);
    const size_t graph_overhead = M_ * 10;  // ~160 bytes for M=16
    size_t total = graph_overhead;

    if (is_quantized()) {
       // Add quant bytes
       total +=  space_->get_data_size(); // This includes RaBitQ residuals
        if (enable_rescoring_) {
#if LSMVECTORSTORAGE
          total += vector_storage_.bytes_per_vector();
#else
          total += vector_bytes;  // Add original 1536 bytes for 384D
#endif
        }
    } else {
        // Float metrics
        total += vector_bytes;
    }
   return total;
}


#if 0
std::string metric_to_string(Metric m) {
  switch(m) {
      case Metric::L1:     return "L1";
      case Metric::L2:     return "L2";
      case Metric::IP:     return "IP";
      case Metric::Cosine: return "Cosine";
      default: return "Unknown";
   }
}

std::optional<Metric> string_to_metric(const std::string& s) {
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
   LOG_ERROR_S() << "Unknown distance metric: " << s << "\n"; 
   return std::nullopt ;
}


std::optional<QuantMode>  string_to_quantzation(const std::string &s) {
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
   LOG_ERROR_S() << "Unknown/unsupported quantization: " << s << "\n";
   return std::nullopt;
}

std::string quantization_to_string(QuantMode mode)
{
   switch (mode) {
     case QuantMode::NONE:   return "Fp32";
     case QuantMode::BIN1:   return "Binary";
     case QuantMode::INT158: return "Ternary";
     case QuantMode::INT4:   return "Nibble";
     case QuantMode::INT8:   return "Octet";
     case QuantMode::FP16:   return "Fp16";
     case QuantMode::BF16:   return "Bf16";
   }
}

// PASS means the Float32 vectors were already quantized!
std::optional<OptBinMode>  string_to_bin_mode(const std::string& s) {
   if (s == "pass")       return OptBinMode::PASS;
   if (s == "standard")   return OptBinMode::STANDARD;
   if (s == "better")     return OptBinMode::BETTER;
   if (s == "centroid")   return OptBinMode::CENTROID;
   if (s == "rotational") return OptBinMode::ROTATIONAL;
   if (s == "RaBitQ")     return OptBinMode::RABITQ;
   if (s == "RaBitQ-Ex")  return OptBinMode::RABITQ_EXTENDED; 
   LOG_ERROR_S() << "Unknown/unsupported bin_mode: " << s << "\n";
   return std::nullopt;
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

std::string storage_type_to_string(StorageType type)
{
   switch (type) {
    case StorageType::BIN1:    return "BIN1";   // Binary
    case StorageType::INT2:    return "INT2";   // 2-bit
    case StorageType::INT3:    return "INT3";   // 3-bit
    case StorageType::INT4:    return "INT4";   // 4-bit
    case StorageType::INT5:    return "INT5";   // 5-bit
    case StorageType::INT6:    return "INT6";   // 6-bit
    case StorageType::INT8:    return "INT8";   // 8-bit
    case StorageType::INT16:   return "INT16";  // 16-bit
    case StorageType::INT32:   return "INT32";  // 32-bit 
    case StorageType::INT64:   return "INT64";  // 64-bit
    case StorageType::FP16:    return "FP16";   // 16-bit float
    case StorageType::BF16:    return "BF16";   // 16-bit float
    case StorageType::FLOAT32: return "FP32";   // 32-bit float
    case StorageType::FLOAT64: return "FP64";   // 64-bit float
   }
   // NOT REACHED
   return "";
};


inline int parse_storage_bits(const std::string& str) {
    if (str.empty()) {
        throw std::invalid_argument("Empty storage string");
        return 0;
    }
    // Check for valid prefixes
    bool floating_point = (str.compare(0,2, "FP") == 0 || str.compare(0,2, "Fp") ||
	str.compare(0,2, "BF") == 0 || str.compare(0,2, "Bf") == 0) ;
    if (!floating_point && str.compare(0, 3, "INT") != 0 && str.compare(0, 3, "Int") !=0 &&
	str.compare(0, 3, "BIN") != 0 && str.compare(0, 3, "Bin") !=0) {
        throw std::invalid_argument("Storage string must start with FP, INT or BIN: " + str);
        return 0;
    }
    
    // Extract number after prefix
    size_t prefix_len = floating_point ? 2 : 3; // "INT" or "BIN" (both are equivalent) or "FP"/"BF"
    if (str.length() <= prefix_len) {
        throw std::invalid_argument("No number after prefix in: " + str);
        return 0;
    }
    std::string num_str = str.substr(prefix_len);
    
    try {
        int bits = std::stoi(num_str);
        if (bits <= 0) {
            throw std::invalid_argument("Bit width must be positive: " + str);
            return 0;
        }
        return bits;
    } catch (const std::invalid_argument&) {
        throw std::invalid_argument("Invalid number in storage string: " + str);
    } catch (const std::out_of_range&) {
        throw std::invalid_argument("Number too large in storage string: " + str);
    }
}

StorageType string_to_storage_type(const std::string& s)
{
   int bits = parse_storage_bits(s);
   // FP16 or FP32?
   if (s.at(0) == 'F' && bits >= 16) {
     if (bits == 16) return StorageType::FP16;
     return StorageType::FLOAT32;
   }
   // BF16 ?
   if (bits == 16 && (s.at(1) == 'F' || s.at(1) == 'f')) 
     return StorageType::BF16;
   switch (bits) {
        case 1: return StorageType::BIN1;
        case 2: return StorageType::INT2;
        case 3: return StorageType::INT3;
        case 4: return StorageType::INT4;
        case 5: return StorageType::INT5;
        case 6: return StorageType::INT6;
        case 8: return StorageType::INT8;
        case 16:return StorageType::INT16;
	default:
	   HNSWERR << "Support for INT"<< bits << " not implemented!\n";
	   return StorageType::FLOAT32;
   }
}

#endif

#if 0 /* MOVED ELSEWHERE */
/*
Case 1: "L2-BIN1-RABITQ"

tokens = ["L2", "BIN1", "RABITQ"]

quant = BIN1 → not PASS → parse mode

storage unused

Case 2: "L2-PASS-INT4"

tokens = ["L2", "PASS", "INT4"]

quant = PASS → use storage_type instead of mode
*/

bool SpecificationString::parse(const std::string& s) {
    // Split on "-"
    std::vector<std::string> tokens;
    std::stringstream ss(s);
    std::string item;

    while (std::getline(ss, item, '-')) {
        tokens.push_back(item);
    }

    if (tokens.size() != 3) {
        // throw std::runtime_error("Invalid format: expected Metric-Quant-Mode|Storage");
	return false;
    }

    // Parse metric 
    {
      auto metric = string_to_metric(tokens[0]);
      if (metric) metric_ = metric;
    }

    // If quant == PASS, interpret the last token as StorageType
    if (tokens[1] == "PASS" || tokens[1] == "pass") {
        storage_type_ = string_to_storage_type(tokens[2]);
        auto q = hnswlib::toQuantMode(storage_type_);
	quantization_ =  q ? *q : QuantMode::NONE;

        use_storage_ = true;
    } else {
        // Normal case: last tokens are bin/quant mode
        { auto result = string_to_quantzation(tokens[1]);
          if (result) quantization_  = *result;
        }
        {
          auto result = string_to_bin_mode(tokens[2]);
          if (result) mode_ = *result;
        }
        use_storage_ = false;
    }

    return true;
}

SpecificationString::  operator std::string() const {
    std::string result;
    // Metric
    result += metric_to_string(metric_);
    result += "-";

    if (use_storage_ && quantization_ != QuantMode::NONE)
        result += bin_mode_to_string(mode_);
    else // Quant
        result += quantization_to_string(quantization_);
    result += "-";

    // Depending on PASS or not, pick mode vs storage
    if (use_storage_)
        result += storage_type_to_string(storage_type_);
    else
        result += bin_mode_to_string(mode_);

    return result;

}

#endif


float UnifiedIndex::score_from_dist(float dist) const {
    // Safety first: guard against invalid or extreme distances
    if (!std::isfinite(dist) || dist > 1e6f) return 0.0f;
    if (dist < -1e6f) return 1.0f;

    switch (metric_) {
        case Metric::L2:
            // For L2 distance: smaller = closer. Map inversely.
            // This keeps results in (0,1] for any reasonable range.
            return 1.0f / (1.0f + dist);

        case Metric::Cosine:
            // For cosine: distance = 1 - cosine_similarity
            // → similarity = 1 - distance
            // Clamp to [0,1] to avoid minor numeric drift.
            // return (std::clamp(1.0f - dist, 0.0f, 1.0f) + 1.0f)/2.0f;
            return (2.0f - dist)/2.0f;

        case Metric::IP:
            // Inner product: higher = closer. HNSWlib may return negatives
            // if embeddings aren't normalized. Clamp to [-1,1].
            return std::clamp(dist, -1.0f, 1.0f);

        default:
            // Unknown metric
            return 0.0f;
    }
}


// Statistics
void UnifiedIndex::printStats() const {
    auto stats = vector_storage_.get_stats();
  
    std::cout << "Space:\n";
    std::cout << "   Metric: " << metric_to_string(metric_) << "\n";
    std::cout << "   Dim: " << dim_ << "\n";
    std::cout << "   Storage: " << quantization_to_string(quantization_) << "\n";
    std::cout << "   Mode: " << bin_mode_to_string(bin_mode_) << "\n";

    std::cout << "Additions since flush: " << additions_since_flush_ << "\n";
    std::cout << "Vector Storage Stats:\n";
    std::cout << "  Main vectors: " << stats.main_vectors << "\n";
    std::cout << "  Delta files: " << stats.delta_files << "\n";
    std::cout << "  Pending additions: " << stats.pending_additions << "\n";
    std::cout << "  Total vectors: " << stats.total_vectors << "\n";
}


}; // namespace hnswlib
