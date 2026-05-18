#include "HnswConfig.hpp"
#include "Util.hpp"
#include "IO.hpp"
#include "Logger.hpp"
#include <thread>


#ifndef WINDOWS
extern "C" {
#include <sys/utsname.h>
}
#endif

static const uint16_t config_version = 1;


namespace hnswlib {

    std::optional<Metric> string_to_metric(const std::string& s);
    std::optional<QuantMode>  string_to_quantzation(const std::string &s);
    std::optional<OptBinMode>  string_to_bin_mode(const std::string& s);
    StorageType string_to_storage_type(const std::string& s);

    std::string metric_to_string(Metric m);
    std::string quantization_to_string(QuantMode mode);
    std::string bin_mode_to_string(OptBinMode mode);
    std::string storage_type_to_string(StorageType type) ;


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
       if (!s.empty()) {
         const char ch = s.at(0);
         if (s == "L1" || s == "l1" || ch == 'M' || ch == 'm')
            return Metric::L1; // Manhatttan
         if (s == "L2" || s == "l2" || ch == 'E' || ch == 'e')
            return Metric::L2; // Eucledian
         if (ch == 'I' || ch  == 'i')
            return Metric::IP; // InnerProduct
         if (ch == 'C' || ch == 'c')
            return Metric::Cosine;
         LOG_ERROR_S() << "Unknown distance metric: '" << s << "'\n";
       } else LOG_ERROR_S() << "Empty distance metric!\n";
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
   if (s == "Int16" || s == "INT16")
	return QuantMode::INT16;
   if (s == "Fp32" || s == "FLOAT32" || s == "NONE" || s == "None")
	return QuantMode::NONE;
   LOG_ERROR_S() << "Unknown/unsupported quantization: " << s << "\n";
   return std::nullopt;
}

std::string quantization_to_string(QuantMode mode)
{
   switch (mode) {
     case QuantMode::NONE:   return "None";
     case QuantMode::BIN1:   return "Binary";
     case QuantMode::INT158: return "Ternary";
     case QuantMode::INT4:   return "Nibble";
     case QuantMode::INT8:   return "Octet";
     case QuantMode::INT16:  return "Int16";
     case QuantMode::FP16:   return "Fp16";
     case QuantMode::BF16:   return "Bf16";
   }
}

// PASS means the Float32 vectors were already quantized!
std::optional<OptBinMode>  string_to_bin_mode(const std::string& s) {
   if (s == "Pass")       return OptBinMode::PASS;
   if (s == "Standard")   return OptBinMode::STANDARD;
   if (s == "Better")     return OptBinMode::BETTER;
   if (s == "Centroid")   return OptBinMode::CENTROID;
   if (s == "Rotational") return OptBinMode::ROTATIONAL;
   if (s == "RaBitQ")     return OptBinMode::RABITQ;
   if (s == "RaBitQ-Ex")  return OptBinMode::RABITQ_EXTENDED; 
   LOG_ERROR_S() << "Unknown/unsupported bin_mode: " << s << "\n";
   return std::nullopt;
}

std::string bin_mode_to_string(OptBinMode mode)
{   
   switch (mode) {
      case OptBinMode::PASS:            return "Pass";
      case OptBinMode::STANDARD:        return "Standard" ;
      case OptBinMode::BETTER:          return "Better";
      case OptBinMode::CENTROID:        return "Centroid";
      case OptBinMode::ROTATIONAL:      return "Rotational";
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
    case StorageType::FP4:     return "FP4";    // 4-bit float
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


#if 1

inline int parse_storage_bits(const std::string& str) {
    if (str.empty()) throw std::invalid_argument("Empty storage string");

    // Convert prefix to uppercase for easier comparison
    std::string prefix = str.substr(0, 3);
    for (auto & c: prefix) c = (char)toupper(c);

    bool is_fp = (prefix.substr(0, 2) == "FP" || prefix.substr(0, 2) == "BF");
    bool is_int = (prefix == "INT");
    bool is_bin = (prefix == "BIN");

    if (!is_fp && !is_int && !is_bin) {
        throw std::invalid_argument("Storage string must start with FP, BF, INT or BIN: " + str);
    }

    size_t prefix_len = is_fp ? 2 : 3;
    
    if (str.length() <= prefix_len) {
        throw std::invalid_argument("No bit-width specified: " + str);
    }

    try {
        int bits = std::stoi(str.substr(prefix_len));
        if (bits <= 0) throw std::invalid_argument("Bit width must be positive");
        return bits;
    } catch (...) {
        throw std::invalid_argument("Invalid bit-width in: " + str);
    }
}

#else

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
#endif

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



    // Validation
    bool HnswConfig::validate() const {
        if (max_elements == 0) {
            LOG_ERROR_S() << "max_elements must be > 0";
            return false;
        }
        if (M == 0 || M > 128) {
            LOG_ERROR_S() << "M must be in range [1, 128]";
            return false;
        }
        if (ef_construction < M * 2) {
            LOG_WARN_S() << "ef_construction should be >= 2*M for good quality";
        }
        if (ef_search == 0) {
            LOG_ERROR_S() << "ef_search must be > 0";
            return false;
        }
	if (ef_search > 900) {
	    if (ef_search < 1500)
	      LOG_WARN_S() << "ef_search " << ef_search << " is at least 10x slower than 300-800";
	    else 
	      LOG_ERROR_S() << "ef_search " << ef_search << " seems wrong!";
	    auto val = std::clamp(20 * std::sqrt(default_k), 50.0, 400.0);
	    LOG_INFO_S() << "A value of " << val << " might be a better choice";
	    if (ef_search > 1500) return false;
        }
        if (overlap_percent < 0.0f || overlap_percent >= 1.0f) {
            LOG_ERROR_S() << "overlap_percent must be in [0, 1)";
            return false;
        }
        if (deletion_threshold_pc < 0.0f || deletion_threshold_pc > 1.0f) {
            LOG_ERROR_S() << "deletion_threshold_pc must be in [0,1]";
            return false;
        }
	// We reserve the value 0 for determine based upon the model
        if (max_tokens_per_chunk < 0) {
            LOG_ERROR_S() << "max_tokens_per_chunk must be > 0";
            return false;
        }
        if (default_k == 0) {
            LOG_ERROR_S() << "default_k must be > 0";
            return false;
        }
        if (default_radius < 0.0f || default_radius > 1.0f) {
            LOG_WARN_S() << "default_radius typically in [0, 1]";
        }
        return true;
    }

    float HnswConfig::get_epsilon() const {
        const float lowI = 0.000001f;
        const float lowC = 0.00001f;
        const float lowX = 0.001f;
        // Use metric-specific epsilon

        switch (specification.metric_) {
            case Metric::L2:
                if (default_epsilonL2 < lowX) break;  // Under threshold
		return default_epsilonL2 * default_epsilonL2; // Square it
            case Metric::Cosine:
		if (default_epsilonIP < lowI) break; // Under threshold
            case Metric::IP:
		if (default_epsilonIP < lowC) break; // Under threshold
		return default_epsilonIP;
            default:
                break;
        }
        if (default_epsilon >= lowX) return default_epsilon;
        if (default_radius >= lowX)  return default_radius;;
        // Default
        return 0.15f;
    }


    // Get effective max_candidates (with cap applied)
    size_t HnswConfig::get_max_candidates(size_t request) const {
        if (request == 0) request = default_k * knn_lookahead_scale;
        if (max_candidates_cap > 0) {
            return std::min(request, max_candidates_cap);
        }
        return request;
    }

    // Get number of merge threads
    unsigned HnswConfig::get_merge_threads() const {
        if (merge_threads == 0) {
            return std::thread::hardware_concurrency();
        }
        return merge_threads;
    }

    // Binary serialization
    void HnswConfig::save(std::ofstream& os) const {
       // Write each field individually (portable)
        auto write_value = [&os](const auto& val) {
            os.write((char*)&val, sizeof(val));
        };
        // Version marker
        write_value (config_version);

        // Specification string
        // NOTE: We use the string form to allow us to
        // easily extend quantization and not having to
        // worry about enums
        write_string(os, specification);

        // model 
        write_string(os, model_name);
        // Write all fields
        write_value(default_search_mode);
        write_value(max_elements);
        write_value(M);
        write_value(ef_construction);
        write_value(ef_search);
        // write_value(specification.metric_);
        // write_value(specification.rescore_);
        write_value(bert_n_threads);
        write_value(max_tokens_per_chunk);
        write_value(overlap_percent);
        write_value(debug);
        write_value(default_k);
        write_value(default_radius);
        write_value(default_alpha);
        write_value(default_minN);
        write_value(default_lookahead);
        write_value(default_gapDelta);
        write_value(default_epsilon);
        write_value(default_epsilonL2);
        write_value(default_epsilonIP);
        write_value(min_candidates);
        write_value(max_candidates_cap);
        write_value(knn_lookahead_scale);
        write_value(flush_threshold);
        write_value(flush_offsets_each);
        write_value(parallel_merge);
        write_value(merge_threads);
        write_value(normalize_embeddings);

        write_value(auto_tune_ef);
        write_value(auto_tune_eps);
        write_value(deletion_threshold_pc);
	write_value(unified_index);
    }

    void HnswConfig::load(std::ifstream& is) {
        auto read_value = [&is](auto& val) {
            is.read((char*)&val, sizeof(val));
        };

        // Read version
        uint16_t version;
        read_value(version);
        if (version != config_version) {
            throw std::runtime_error("Unsupported config version");
        }
        specification = read_string(is); // Get the specification
        model_name = read_string(is); // Get the model name
        // Read each field individually
        read_value(default_search_mode);
        read_value(max_elements);
        read_value(M);
        read_value(ef_construction);
        read_value(ef_search);
        // read_value(specification.metric_);
        // read_value(specification.rescore_);
        read_value(bert_n_threads);
        read_value(max_tokens_per_chunk);
        read_value(overlap_percent);
        read_value(debug);
        read_value(default_k);
        read_value(default_radius);
        read_value(default_alpha);
        read_value(default_minN);
        read_value(default_lookahead);
        read_value(default_gapDelta);
        read_value(default_epsilon);
        read_value(default_epsilonL2);
        read_value(default_epsilonIP);
        read_value(min_candidates);
        read_value(max_candidates_cap);
        read_value(knn_lookahead_scale);
        read_value(flush_threshold);
        read_value(flush_offsets_each);
        read_value(parallel_merge);
        read_value(merge_threads);
        read_value(normalize_embeddings);

        read_value(auto_tune_ef);
        read_value(auto_tune_eps);

        read_value(deletion_threshold_pc);
	read_value(unified_index);

        
        if (!validate()) {
            throw std::runtime_error("Loaded invalid configuration");
        }
    }

    // Save to file
    bool HnswConfig::save_to_file(const std::string& path) const {
        std::ofstream ofs(path, std::ios::binary);
        if (!ofs) {
            LOG_ERROR_S() << "Failed to open " << path << " for writing (save config)";
            return false;
        }
        save(ofs);
        return true;
    }

    // Load from file
    bool HnswConfig::load_from_file(const std::string& path) {
        if (path.empty() )
           return false; // nothing to load
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs) {
            return false;  // File doesn't exist, not an error
        }
        try {
            load(ifs);
            return true;
        } catch (const std::exception& e) {
            LOG_ERROR_S() << "Error loading config from " << path << ": " << e.what();
            return false;
        }
    }

#if 0 /* NOT NEEDED */
    // Merge/override with another config
    // Only copies fields that differ from defaults
    void HnswConfig::merge_from(const HnswConfig& other) {
        // Copy all fields from other
        // This is a simple approach - copies everything
        *this = other;
    }
#endif

    // Selective merge (only override non-default values)
    void HnswConfig::merge_overrides(const HnswConfig& override, const HnswConfig& defaults) {
        // Helper macro to check and override
        #define OVERRIDE_IF_DIFFERENT(field) \
            if (override.field != defaults.field) { \
                this->field = override.field; \
            }
        OVERRIDE_IF_DIFFERENT(model_name); 
        OVERRIDE_IF_DIFFERENT(default_search_mode);
        OVERRIDE_IF_DIFFERENT(max_elements);
        OVERRIDE_IF_DIFFERENT(M);
        OVERRIDE_IF_DIFFERENT(ef_construction);
        OVERRIDE_IF_DIFFERENT(ef_search);
        OVERRIDE_IF_DIFFERENT(specification.metric_);
        OVERRIDE_IF_DIFFERENT(bert_n_threads);
        OVERRIDE_IF_DIFFERENT(max_tokens_per_chunk);
        OVERRIDE_IF_DIFFERENT(overlap_percent);
        OVERRIDE_IF_DIFFERENT(debug);
        OVERRIDE_IF_DIFFERENT(default_k);
        OVERRIDE_IF_DIFFERENT(default_radius);
        OVERRIDE_IF_DIFFERENT(default_alpha);
        OVERRIDE_IF_DIFFERENT(default_minN);
        OVERRIDE_IF_DIFFERENT(default_lookahead);
        OVERRIDE_IF_DIFFERENT(default_gapDelta);
        OVERRIDE_IF_DIFFERENT(default_epsilon);
        OVERRIDE_IF_DIFFERENT(default_epsilonL2);
        OVERRIDE_IF_DIFFERENT(default_epsilonIP);
        OVERRIDE_IF_DIFFERENT(min_candidates);
        OVERRIDE_IF_DIFFERENT(max_candidates_cap);
        OVERRIDE_IF_DIFFERENT(specification.rescore_);
        OVERRIDE_IF_DIFFERENT(knn_lookahead_scale);
        OVERRIDE_IF_DIFFERENT(flush_threshold);
        OVERRIDE_IF_DIFFERENT(flush_offsets_each);
        OVERRIDE_IF_DIFFERENT(parallel_merge);
        OVERRIDE_IF_DIFFERENT(merge_threads);
        OVERRIDE_IF_DIFFERENT(normalize_embeddings);

        OVERRIDE_IF_DIFFERENT(specification);

        OVERRIDE_IF_DIFFERENT(auto_tune_ef);
        OVERRIDE_IF_DIFFERENT(auto_tune_eps);

        OVERRIDE_IF_DIFFERENT(deletion_threshold_pc);
	OVERRIDE_IF_DIFFERENT(unified_index);
        
        #undef OVERRIDE_IF_DIFFERENT
    }

    // Print configuration
    void HnswConfig::print(std::ostream& os) const {
        os << "=== HNSW Configuration ===\n";
        os << "Default search mode: " << search_mode_to_string(default_search_mode) << "\n";
        os << "\nIndex parameters:\n";
        os << "  max_elements: " << max_elements << "\n";
        os << "  M: " << M << "\n";
        os << "  ef_construction: " << ef_construction << "\n";
        os << "  ef_search: " << ef_search << "\n";
        os << "  specification: " << specification << "\n";
//        os << "  metric: " << hnswlib::metric_to_string(specification.metric_) << "\n";
        os << "  normalize_embeddings: " << (normalize_embeddings ? "yes" : "no") << "\n";
	os << "  unified_index: " << (unified_index ? "yes" : "no") << "\n";
        
        os << "\nEmbedding:\n";
        os << "  bert_n_threads: " << bert_n_threads << "\n";
        
        os << "\nChunking:\n";
	// max_tokens_per_chunk --> let the model decide
	os << "  max_tokens_per_chunk: ";
	if (max_tokens_per_chunk > 0) os << max_tokens_per_chunk;
	else os << "dynamic (by model)";
	
        os << "\n  overlap_percent: " << overlap_percent << "\n";
        
        os << "\nSearch defaults:\n";
        os << "  k (knn): " << default_k << "\n";
        os << "  radius: " << default_radius << "\n";
        os << "  alpha (relative): " << default_alpha << "\n";
        os << "  minN (adaptive): " << default_minN << "\n";
        os << "  lookahead (adaptive): " << default_lookahead << "\n";
        os << "  gapDelta (adaptive): " << default_gapDelta << "\n";
        os << "  enable_rescoring (quantized): " << (enable_rescoring() ? "yes" : "no" ) << "\n";
        os << "  deletion_threshold_pc: " <<  deletion_threshold_pc << "\n";
        
        os << "\nEpsilon search:\n";
        os << "  epsilon: " << default_epsilon << "\n";
        os << "  epsilonL2: " << default_epsilonL2 << "\n";
        os << "  epsilonIP: " << default_epsilonIP << "\n";
        // os << "  epsilonB: " << default_epsilonB << "\n";
        // os << "  epsilonT: " << default_epsilonB << "\n";
        os << "  min_candidates: " << min_candidates << "\n";
        os << "  max_candidates_cap: " << max_candidates_cap << "\n";
        
        os << "\nPerformance:\n";
        os << "  knn_lookahead_scale: " << knn_lookahead_scale << "\n";
        os << "  flush_threshold: " << flush_threshold << "\n";
        os << "  flush_offsets_each: " << (flush_offsets_each ? "yes" : "no") << "\n";
        os << "  parallel_merge: " << (parallel_merge ? "yes" : "no") << "\n";
        os << "  merge_threads: " << get_merge_threads() << "\n";

        os << "\nTuning:\n";
        os << "  auto_tune_ef: " << (auto_tune_ef ? "yes" : "no") << "\n";
        os << "  auto_tune_eps: " << (auto_tune_eps ? "yes" : "no") << "\n"; 
        
        os << "\nDebug: " << (debug ? "enabled" : "disabled") << "\n";
        os << "Model: " << (model_name.empty() ? "<Undefined>" : model_name ) << "\n";
	if (matryoshka_dim)
	os << "  Matryoshka Dimension: " << matryoshka_dim << "d\n"; 

        os << "\n===   This Platform    ===\nOS: ";
#ifdef _WIN32
        os << "Windows 32-bit\n";
#elif _WIN64
        os << "Windows 64-bit\n";
#else       
        utsname u;      // declare the variable to hold the result
        uname(&u);      // call the uname() function to fill the struct
        os  << u.sysname << " " << u.release << '\n';
        os << "Hardware: " << u.machine << " / "
		<< std::thread::hardware_concurrency() << " cores\n";
#endif  
        // Print SIMD capability
#ifdef __AVX512F__
        os << "SIMD: AVX-512 enabled\n\n";
#elif defined(__AVX2__)
        os  << "SIMD: AVX2 enabled\n\n";
#elif defined(__ARM_NEON)
        os  << "SIMD: ARM NEON enabled\n\n";
#else   
       os  << "SIMD: Scalar implementation (no SIMD)\n\n";
#endif      
   }



    // Dynamic setter by string key
    bool HnswConfig::set(const std::string& key, const std::string& value) {
        try {
            // size_t fields
            if (key == "max_elements") { max_elements = std::stoull(value); return true; }
            if (key == "M") { M = std::stoull(value); return true; }
            if (key == "ef_construction") { ef_construction = std::stoull(value); return true; }
            if (key == "ef_search") { ef_search = std::stoull(value); return true; }
            if (key == "bert_n_threads") { bert_n_threads = std::stoull(value); return true; }
            if (key == "default_k") { default_k = std::stoull(value); return true; }
            if (key == "default_minN") { default_minN = std::stoull(value); return true; }
            if (key == "default_lookahead") { default_lookahead = std::stoull(value); return true; }
            if (key == "min_candidates") { min_candidates = std::stoull(value); return true; }
            if (key == "max_candidates_cap") { max_candidates_cap = std::stoull(value); return true; }
            if (key == "knn_lookahead_scale") { knn_lookahead_scale = std::stoull(value); return true; }
            if (key == "merge_threads") { merge_threads = std::stoul(value); return true; }
            
            // int fields
            if (key == "max_tokens_per_chunk") { max_tokens_per_chunk = std::stoi(value); return true; }
            if (key == "flush_threshold") { flush_threshold = std::stoi(value); return true; }
            
            // float fields
            if (key == "overlap_percent") { overlap_percent = std::stof(value); return true; }
            if (key == "default_radius") { default_radius = std::stof(value); return true; }
            if (key == "default_alpha") { default_alpha = std::stof(value); return true; }
            if (key == "default_gapDelta") { default_gapDelta = std::stof(value); return true; }
            if (key == "default_epsilon") { default_epsilon = std::stof(value); return true; }
            if (key == "default_epsilonL2") { default_epsilonL2 = std::stof(value); return true; }
            if (key == "default_epsilonIP") { default_epsilonIP = std::stof(value); return true; }
            // if (key == "default_epsilonB") { default_epsilonB = std::stof(value); return true; }
            // if (key == "default_epsilonT") { default_epsilonT = std::stof(value); return true; }
            if (key == "deletion_threshold_pc") { deletion_threshold_pc = std::stof(value); return true; }
	    if (key == "deletion_threshold_percent") { deletion_threshold_pc = std::stof(value)/100.0f; return true;}

            // bool fields

#define PROCESS_BOOL_FIELD(field_name, field_var) \
            if (key == field_name) { \
                auto val = parse_bool(value); \
                if (val) { \
                    field_var = *val; \
                    return true; \
                } \
                return false; \
            }

            PROCESS_BOOL_FIELD("debug", debug);
            PROCESS_BOOL_FIELD("flush_offsets_each", flush_offsets_each);
            PROCESS_BOOL_FIELD("parallel_merge", parallel_merge);
            PROCESS_BOOL_FIELD("normalize_embeddings", normalize_embeddings);
            PROCESS_BOOL_FIELD("enable_rescoring", specification.rescore_);
            PROCESS_BOOL_FIELD("auto_tune_ef", auto_tune_ef);
            PROCESS_BOOL_FIELD("auto_tune_eps", auto_tune_eps);
	    PROCESS_BOOL_FIELD("unified_index", unified_index);
            
            // enum fields
            if (key == "metric") {
                auto val = hnswlib::string_to_metric(value);
                if (val) {
                   specification.metric_ = *val;
                   return true;
                }
                return false;
            }
            if (key == "default_search_mode") {
		auto mode = string_to_search_mode(value);
		if (mode) {
		   default_search_mode = *mode; return true;
                }
		return false;
             }

            if (key == "model") {
	        model_name = value;
		return true;
            }

            if (key == "specification") { return specification.parse(value); }
            
            std::cerr << "Unknown config key: " << key << "\n";
            return false;
            
        } catch (const std::exception& e) {
            std::cerr << "Error setting " << key << "=" << value << ": " << e.what() << "\n";
            return false;
        }
    }


    // Dynamic getter by string key
    std::string HnswConfig::get(const std::string& key) const {
        // size_t fields
        if (key == "max_elements") return std::to_string(max_elements);
        if (key == "M") return std::to_string(M);
        if (key == "ef_construction") return std::to_string(ef_construction);
        if (key == "ef_search") return std::to_string(ef_search);
        if (key == "bert_n_threads") return std::to_string(bert_n_threads);
        if (key == "default_k") return std::to_string(default_k);
        if (key == "default_minN") return std::to_string(default_minN);
        if (key == "default_lookahead") return std::to_string(default_lookahead);
        if (key == "min_candidates") return std::to_string(min_candidates);


        if (key == "knn_lookahead_scale") return std::to_string(knn_lookahead_scale);
        if (key == "merge_threads") return std::to_string(merge_threads);
        
        // int fields
        if (key == "max_tokens_per_chunk") return std::to_string(max_tokens_per_chunk);
        if (key == "flush_threshold") return std::to_string(flush_threshold);
	if (key == "max_candidates_cap") return max_candidates_cap > 0 ?
		std::to_string(max_candidates_cap) : "auto";
        
        // float fields
        if (key == "overlap_percent") return std::to_string(overlap_percent);
        if (key == "default_radius") return std::to_string(default_radius);
        if (key == "default_alpha") return std::to_string(default_alpha);
        if (key == "default_gapDelta") return std::to_string(default_gapDelta);
        if (key == "default_epsilon") return std::to_string(default_epsilon);
        if (key == "default_epsilonL2") return std::to_string(default_epsilonL2);
        if (key == "default_epsilonIP") return std::to_string(default_epsilonIP);
        // if (key == "default_epsilonB") return std::to_string(default_epsilonB);
        // if (key == "default_epsilonT") return std::to_string(default_epsilonT);

        if (key == "deletion_threshold_pc") return std::to_string(deletion_threshold_pc) ;
        
        // bool fields
        if (key == "debug") return debug ? "true" : "false";
        if (key == "flush_offsets_each") return flush_offsets_each ? "true" : "false";
        if (key == "parallel_merge") return parallel_merge ? "true" : "false";
        if (key == "normalize_embeddings") return normalize_embeddings ? "true" : "false";
        if (key == "enable_rescoring") return enable_rescoring() ? "true" : "false";
	if (key == "auto_tune_ef") return auto_tune_ef ? "enabled" : "off";
	if (key == "auto_tune_eps") return auto_tune_eps ? "enabled" : "off";
	if (key == "unified")        return unified_index ? "enabled" : "off";
        
        // enum fields
        if (key == "metric") return hnswlib::metric_to_string(specification.metric_);
        if (key == "default_search_mode") return search_mode_to_string(default_search_mode);
        
        throw std::runtime_error("Unknown config key: " + key);
    }


#if 0
std::optional<HnswConfig> findHnswConfig() {
   auto configResult = findInPathsWithData<HnswConfig>(
    "schmate.conf", 
    "/etc:/usr/local/etc:~/.config",
    [](const fs::path& path) -> std::optional<HnswConfig> {
        HnswConfig cfg;
        if (!cfg.load_from_file(path)) return std::nullopt;
        return cfg;
    });
   return configResult;
}

#endif

/*
Rule of Thumb:
| Index Size | ef_search |
| ---------- | --------- |
| < 10 K     | 64–100    |
| 10 K–100 K | 100–200   |
| 100 K–1 M  | 200–400   |
| > 1 M      | 300–800+  |

*/



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

    // Want to support: L2-BIN1-RABITQ-RESCORE. We'll assume if
    // a 4th element is defined than its to rescore!

    if (tokens.size() == 4) {
      rescore_ = true;
    } else if(tokens.size() != 3) {
        // throw std::runtime_error("Invalid format: expected Metric-Quant-Mode|Storage");
	return false;
    }

    // Parse metric 
    {
      auto metric = hnswlib::string_to_metric(tokens[0]);
      if (metric) metric_ = *metric;
    }

    // If quant == PASS, interpret the last token as StorageType
    if (tokens[1] == "PASS" || tokens[1] == "pass") {
        rescore_ = false;
        storage_type_ = string_to_storage_type(tokens[2]);
        auto q = hnswlib::toQuantMode(storage_type_);
	quantization_ =  q ? *q : QuantMode::NONE;

        use_storage_ = true;
    } else {
        // Normal case: last tokens are bin/quant mode
        auto q = string_to_quantzation(tokens[1]);
        if (q) {
            quantization_  = *q; 
            if (quantization_ == QuantMode::NONE) 
	    rescore_ = false;
        }
        auto mode = string_to_bin_mode(tokens[2]);
        if (mode) mode_ = *mode;
	if (mode_ == OptBinMode::RABITQ || mode_ == OptBinMode::RABITQ_EXTENDED)
	  quantization_ = QuantMode::BIN1;
        use_storage_ = false;
    }

    // Debug for now
    LOG_DEBUG_S() << "\"" << s << "\"-->\"" << *this << "\"\n"; 
    return true;
}

#if 1

// Fixup
SpecificationString::operator std::string() const {
    std::string result;

    // 1. Metric
    result += hnswlib::metric_to_string(metric_); // "IP"
    result += "-";

    // 2. Quantization Mode
    if (quantization_ == QuantMode::NONE) {
        // If it's a pre-quantized type (not FP32), we call it "PASS"
        // If it's standard high-precision, we call it "NONE"
        result += (storage_type_ == StorageType::FLOAT32) ? "NONE" : "PASS";
    } else {
        result += hnswlib::quantization_to_string(quantization_); // "RABITQ"
    }
    result += "-";

    // 3. Storage Type / Bin Mode
    // If we are quantizing (e.g. RABITQ), show the mode. 
    // Otherwise, show the storage type.
    if (quantization_ != QuantMode::NONE) {
        result += hnswlib::bin_mode_to_string(mode_);
    } else {
        result += storage_type_to_string(storage_type_); // "FP32" or "INT4"
    }

    // 4. Rescore (Only if we actually quantized)
    if (rescore_ && quantization_ != QuantMode::NONE) {
        result += "-RESCORE";
    }

    return result;
}




#else

SpecificationString::  operator std::string() const {
    std::string result;
    // Metric
    result += hnswlib::metric_to_string(metric_);
    result += "-";

    if (use_storage_ && quantization_ != QuantMode::NONE) {
        result += hnswlib::bin_mode_to_string(mode_);
    } else { // Quant
        if (use_storage_) result += "Pass";
        else result += hnswlib::quantization_to_string(quantization_);
    }
    result += "-";

    // Depending on PASS or not, pick mode vs storage
    if (use_storage_)
        result += storage_type_to_string(storage_type_);
    else
        result += bin_mode_to_string(mode_);

    // In parsing we only worry about 4th element so -ANYTHING
    // NOTE: We check plausibility here to not propigate @!#
    // RE-Scoring can only make sense when we have quantization
    if (rescore_ && !use_storage_ && quantization_ != QuantMode::NONE)
       result += "-RESCORE";

    return result;

}
#endif

}; // Namespace
