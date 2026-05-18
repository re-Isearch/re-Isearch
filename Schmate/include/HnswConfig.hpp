#pragma once
#include <cstddef>
#include <string>
#include <fstream>
#include <iostream>
//#include "unified_hnsw.hpp"
#include "IO.hpp"
#include "hnswlib/quantized.h"
#include "Logger.hpp"

namespace hnswlib {

struct IndexFileExtensions {
  static constexpr const char* sentences= ".txt"; // Text of strings for debugging
  static constexpr const char* offsets  = ".obn"; // Offset file
  static constexpr const char* hnsw     = ".hix"; // HNSW index including Meta
  static constexpr const char* tuner    = ".eft"; // Ef search-time paramter learning
  static constexpr const char* eps      = ".eps"; // Epsilon learning
  static constexpr const char* hyparam  = ".hyp"; // Hyperparameters
  static constexpr const char* lock     = ".lock"; // Lock, 0 length = unlocked
  static constexpr const char* merge    ="_merged_tmp"; // Merge temp file

} ;

enum class Metric { L1 = 0, L2 = 1, IP = 2, Cosine = 3 };

std::string metric_to_string(Metric m);


enum class SearchModes {
    Knn,
    Radius,
    Relative,
    Adaptive,
    Epsilon
};


// ============================================
// Parse a Specification String: Metric, Storage,..
// ============================================
// Example: "L2-BIN1-RABITQ"
class SpecificationString {
public:
   // We set defaults here since parse below can fail!
   Metric      metric_ = Metric::IP;
   QuantMode   quantization_ = QuantMode::NONE;
   OptBinMode  mode_ = OptBinMode::PASS ;
   StorageType storage_type_ = StorageType::FLOAT32;

   // Change this to VectorStorageMode 
   //  DISABLED means NO RESCORE
   // Then there is a choice between IN_MEMORY and MEMORY_MAPPED

   bool        rescore_ = false;
   // NOTE: dim is not in the specification since it is defined
   // by the chosen embedding model -- this is not 100% true since
   // we have a strong interdepency between the model's quantization
   // and the specification. We don't want (its, of course, possible but
   // counter-productive) to binarize an already quantized model. 
   // ==> a X-bit quantized model should use the defaults above but set
   // the storage_type_ to X-bit!

   SpecificationString() {}
   SpecificationString(const std::string& str) {
     parse(str);
   }

   // Check if the spec's match
   bool operator==(const SpecificationString& other) const {
      if (metric_       != other.metric_ ||
          quantization_ != other.quantization_ ||
          mode_         != other.mode_ ||
          storage_type_ != other.storage_type_ ||
          rescore_      != other.rescore_)
        return false;
      return true;
   }
   void save(std::ofstream& os) const {
      write_string(os, *this);
   }
   void load(std::ifstream& is) {
      parse (read_string(is));
   }


  
  // Set spec
   SpecificationString operator=(const SpecificationString& other) {
      metric_       = other.metric_;
      quantization_ = other.quantization_;
      mode_         = other.mode_;
      storage_type_ = other.storage_type_;
      use_storage_  = other.use_storage_;
      return *this;
   }

   bool parse(const std::string& s);
   operator std::string() const; 

    friend std::ostream& operator<<(std::ostream& os, const SpecificationString& spec) {
        return os << std::string(spec);
    }
//private:
    bool use_storage_ = false; // true if PASS case
} ;


struct HnswConfig {
    SearchModes default_search_mode = SearchModes::Knn;
    std::string  model_name;

    size_t max_elements = 500000;
    size_t M = 16;
    size_t ef_construction = 200;

/*   ef_search Rule of Thumb:
    | Index Size | ef_search |
    | ---------- | --------- |
    | < 10 K     | 64–100    |
    | 10 K–100 K | 100–200   |
    | 100 K–1 M  | 200–400   |
    | > 1 M      | 300–800+  | */
    size_t ef_search = 64;

    size_t matryoshka_dim = 0;

    // This contains the index specification
    // --> This allows us to better extend in the future
    SpecificationString specification;

    Metric      metric () const          { return specification.metric_;}
    bool        enable_rescoring() const { return specification.rescore_;}
    QuantMode   quantization() const     { return specification.quantization_; }
    OptBinMode  mode() const             { return specification.mode_;}
    StorageType storage_type() const     { return specification.storage_type_;}

    void set_metric (Metric val)          { specification.metric_ = val;}
    void set_enable_rescoring(bool val)   { specification.rescore_ = val;}
    void set_quantization(QuantMode val)  {
       specification.quantization_ = val;
       if (val != QuantMode::NONE) {
	if (specification.mode_ ==  OptBinMode::PASS)
	  specification.mode_ = OptBinMode::STANDARD;
        } else if (specification.storage_type_ != StorageType::FLOAT32)
	  specification.mode_ = OptBinMode::PASS;
    }
    void set_mode(OptBinMode val)         {
	specification.mode_ = val;
	if (val == OptBinMode::PASS) {
	  if (specification.storage_type_ != StorageType::FLOAT32)
	    specification.use_storage_ = true;
            specification.quantization_ =  QuantMode::NONE;
        } else if (specification.quantization_ == QuantMode::NONE) {
	  auto val = toQuantMode(specification.storage_type_);
	  if (val) specification.quantization_ = *val;
        }
    }
    void set_storage_type(StorageType val){
	specification.storage_type_ = val;
        if (specification.mode_ == OptBinMode::PASS && val != StorageType::FLOAT32) {
           specification.use_storage_ = true;
           specification.quantization_ =  QuantMode::NONE; 
        }
    }

    size_t bert_n_threads = 4;

    // chunking
    int max_tokens_per_chunk = 0;
    float overlap_percent = 0.125f;

    bool lock_on_append = true;

    // debug
    bool debug = false;  // default debug disabled

    // search defaults
    size_t default_k = 5;          // k for knn
    float default_radius = 0.7f;   // min score for radius
    float default_alpha = 0.8f;    // relative threshold multiplier
    size_t default_minN = 3;       // adaptive: minimum results
    size_t default_lookahead = 10; // adaptive: lookahead window
    float default_gapDelta = 0.1f; // adaptive: gap threshold

    // epsilon controls how aggressively the Epsilon search stops when candidates are
    // within a certain distance tolerance of the best current match.
    //
    // A smaller epsilon → stricter search, fewer results (like a tighter radius).
    //
    // A larger epsilon → looser search, potentially many more candidates (and longer runtime).
    //
    // Epsilon here is relative to the distance scale of your space:
    //
    // For L2 distance, typical values are in 0.001–0.1 range.
    // For cosine similarity, the “distance” HNSW uses is often 1 - cosine, so the effective
    // range is 0 → 2 (but normally results cluster in 0–0.5).
    //
    // NOTE: We have some methods for runtime tuning of these values!

    float default_epsilon   = 0.15f; // epsilon, if < 0 then use radius
    float default_epsilonL2 = 1.41;  // Distance threshold, this is then ^2
    float default_epsilonIP = 0.5f;  // 

    float deletion_threshold_pc = 0.2f;

    size_t min_candidates = 10;    // Min candidates for epsilon
    size_t max_candidates_cap = 0; // 0 = auto

    bool unified_index = false; // Don't fold into single HNSW index

    // performance tuning
    size_t knn_lookahead_scale = 5;
    int    flush_threshold = 100; // Save index every, -1 only on explicit flush or exit.
    bool   flush_offsets_each = false;


    //
    bool parallel_merge = true;
    unsigned merge_threads = 0; // 0 = auto
    //
    bool normalize_embeddings = false;

    // Dynamic auto-tuning
    bool auto_tune_ef = false;
    bool auto_tune_eps = false;

    void set_autotune(bool val = true) {
      auto_tune_ef  = val;
      auto_tune_eps = val;
    }

    // Validation
    bool validate() const;

    // Get epsilon for current spec
    float get_epsilon() const;

    // Get effective max_candidates (with cap applied)
    size_t get_max_candidates(size_t request = 0) const;

    // Get number of merge threads
    unsigned get_merge_threads() const;

    // Print configuration
    void print(std::ostream& os = std::cout) const;

    // Binary serialization
    void save(std::ofstream& os) const;

    void load(std::ifstream& is);

    // Save to file
    bool save_to_file(const std::string& path) const;

    // Load from file
    bool load_from_file(const std::string& path);

    // Merge/override with another config
    // void merge_from(const HnswConfig& other);

    // Selective merge (only override non-default values)
    void merge_overrides(const HnswConfig& override, const HnswConfig& defaults);


    // Dynamic setter by string key
    bool set(const std::string& key, const std::string& value);

    // Typed setters for convenience
    bool set(const std::string& key, size_t value) {
        return set(key, std::to_string(value));
    }
    
    bool set(const std::string& key, int value) {
        return set(key, std::to_string(value));
    }
    
    bool set(const std::string& key, float value) {
        return set(key, std::to_string(value));
    }
    
    bool set(const std::string& key, bool value) {
        return set(key, value ? "true" : "false");
    }
    
    bool set(const std::string& key, Metric value) {
        return set(key, hnswlib::metric_to_string(value));
    }
    
    bool set(const std::string& key, SearchModes value) {
        return set(key, search_mode_to_string(value));
    }

    // Dynamic getter by string key
    std::string get(const std::string& key) const;

    // Get all keys
    static std::vector<std::string> get_all_keys() {
        return {
            "model",
            "max_elements", "M", "ef_construction", "ef_search", "metric",
            "bert_n_threads", "max_tokens_per_chunk", "overlap_percent",
            "debug", "default_k", "default_radius", "default_alpha",
            "default_minN", "default_lookahead", "default_gapDelta",
            "default_epsilon", "default_epsilonL2", "default_epsilonIP",
            "enable_rescoring", "specification",
            "min_candidates", "max_candidates_cap", "knn_lookahead_scale",
            "flush_threshold", "flush_offsets_each", "parallel_merge",
            "merge_threads", "normalize_embeddings", "default_search_mode",
            "auto_tune_ef", "auto_tune_eps",
            "deletion_threshold_pc"
        };
    }

#if 0
    // String conversions
    static std::string metric_space_to_string(MetricSpace m) {
        switch (m) {
            case MetricSpace::L2: return "L2";
            case MetricSpace::InnerProduct: return "InnerProduct";
            case MetricSpace::Cosine: return "Cosine";
            case MetricSpace::Binary: return "Binary";
            case MetricSpace::Ternary: return "Ternary";
            default: return "Undefined";
        }
    }


   static std::string metric_to_string(Metric m) {
    switch(m) {   
      case Metric::L1:     return "L1";
      case Metric::L2:     return "L2";
      case Metric::IP:     return "InnerProduct";
      case Metric::Cosine: return "Cosine";
      default: return "Unknown";
     }
   }   
#endif
    
   std::optional<Metric> string_to_metric(const std::string& s) {
   if (s.empty()) {
     LOG_ERROR_S() << "Empty distance metric name.\n";
   } else { 
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
   }
   return std::nullopt ;
}



private:

    static std::optional<bool> parse_bool(const std::string& s) {
        std::string lower = s;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower == "true" || lower == "1" || lower == "yes" || lower == "on") return true;
        if (lower == "false" || lower == "0" || lower == "no" || lower == "off") return false;
        LOG_ERROR_S() << "Invalid boolean value: '" << s << "'\n";
        return std::nullopt;
    }

#if 0
    static std::string distance_metric_to_string(DistanceMetric m) {
       switch(m) {
           case DistanceMetric::L1: return "L1";
           case DistanceMetric::L2: return "L2";
           case DistanceMetric::IP: return "IP";
           default: return "Unknown";
       }
    }
#endif

    static std::string search_mode_to_string(SearchModes m) {
        switch (m) {
            case SearchModes::Knn: return "Knn";
            case SearchModes::Radius: return "Radius";
            case SearchModes::Relative: return "Relative";
            case SearchModes::Adaptive: return "Adaptive";
            case SearchModes::Epsilon: return "Epsilon";
            default: return "Unknown";
        }
    }

#if 0
    static MetricSpace string_to_metric_space(const std::string& s) {
        if (s == "L2" || s == "l2") return MetricSpace::L2;
        if (s == "InnerProduct" || s == "IP" || s == "ip") return MetricSpace::InnerProduct;
        if (s == "Cosine" || s == "cosine") return MetricSpace::Cosine;
        if (s == "Binary" || s == "binary") return MetricSpace::Binary;
        if (s == "Ternary" || s == "Ternary" || s == "b1.58" || "1.58" ) return MetricSpace::Ternary;
        if (s == "Undefined") return MetricSpace::Undefined;
        throw std::runtime_error("Unknown metric: " + s);
    }
    static DistanceMetric string_to_distance_metric(const std::string& s) {
        if (s == "L1" || s == "l1") return DistanceMetric::L1;
        if (s == "L2" || s == "l2") return DistanceMetric::L2;
        if (s == "IP" || s == "ip") return DistanceMetric::IP;
        throw std::runtime_error("Unknown distance metric: " + s);
    }
#endif

    static std::optional<SearchModes> string_to_search_mode(const std::string& s) {
        if (s == "knn" || s == "Knn") return SearchModes::Knn;
        if (s == "radius" || s == "Radius") return SearchModes::Radius;
        if (s == "relative" || s == "Relative") return SearchModes::Relative;
        if (s == "adaptive" || s == "Adaptive") return SearchModes::Adaptive;
        if (s == "epsilon" || s == "Epsilon") return SearchModes::Epsilon;
        LOG_ERROR_S() << "Unknown search mode: '" << s << "'\n";
        return std::nullopt;
    }

};


}; // namespace



