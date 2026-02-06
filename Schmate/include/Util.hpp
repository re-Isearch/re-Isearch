#pragma once
#include <cstdint>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <filesystem>

#include "hnswlib/int_storage.h"

// ========= Read GMML and GUUF Files  ============================== //


// See struct bert_hparams in bert.cpp
struct GmmlHparams {
    uint32_t magic;           // 'ggml' = 0x67676d6c
    int32_t n_vocab;          // Vocabulary size
    int32_t n_max_tokens;     // Max sequence length
    int32_t n_embd;           // Embedding dimensions
    int32_t n_intermediate;   // Intermediate/FFN dimensions
    int32_t n_head;           // Number of attention heads
    int32_t n_layer;          // Number of layers
    int32_t f16;              // Quantization type codes: 0=F32, 1=F16, 2=Q4_0, 3=Q4_1, etc.
};


struct GGUFInfo {
    std::string architecture;
    uint32_t embedding_length = 0;
    std::string quant_type;     // Human-readable ("F16", "Q4_0", ...)
};


// NOTE: bert.cpp only supports f16 values in {0, 1, 2, 3}

// Codes <-> names and names -> quant type
const std::string    ggml_quant_name(uint32_t code); // Takes code and returns name
hnswlib::StorageType ggml_quant_type(uint32_t code); // Takes code and returns storage type
int                  ggml_name_to_quant(const std::string& name); // Takes name and returns code
hnswlib::StorageType ggml_name_to_quant_type(const std::string &name);


std::optional<GmmlHparams> read_ggml_info(const std::string &path);

// Search for a filename in a UNIX style path (dir1:dir2) and confirm it's .ggml
// If found returns the now qualified path else just the filename
enum GGML_TYPE { UNKNOWN = 0, GGML, GGUF};
std::pair <std::string, enum GGML_TYPE> find_ggml_model(const std::string &filename, const std::string  &search_paths);


std::pair <std::string, enum GGML_TYPE> find_model(const std::string &identity, const std::string  &search_paths);



// Get the storage type/quantization of a GGML model
std::pair<hnswlib::StorageType, std::string> get_ggml_model_quant(const std::string &filename) ;


#include <filesystem>

// ==================== Misc ===================================== //

inline std::string basename(const std::string &p) {
   return std::filesystem::path(p).filename().string();
}

// File exist and length utils
inline bool file_exists(const std::string &p) {
      return std::filesystem::exists(p);
}
// Does not exist? Return -1 else return its size
inline off_t file_size(const std::string &p) {
    if (!file_exists(p)) return -1;
    return std::filesystem::file_size(p);
}

#if 1 // C++ 17
std::vector<std::string> splitPath(
    const std::string& pathStr,
    char delimiter = std::filesystem::path::preferred_separator == '\\' ? ';' : ':'
);


#else

std::vector<std::string> splitPath(const std::string& pathStr, char delimiter =       
  #if defined(_MSDOS) || defined(_WIN32)
        ';'
  #else   
        ':' 
  #endif
);
#endif


// Generic path search function that returns path and extracted data
// Return type includes the path and custom data
// NOTE: Must be inline or fully defined in header when used with lambdas
template<typename T, typename Predicate>
inline std::optional<std::pair<std::filesystem::path, T>> findInPathsWithData(
    const std::string& filename,
    const std::string& searchPaths,
    Predicate&& matchPredicate)
{
    if (std::filesystem::path(filename).is_absolute()) {
        std::filesystem::path absPath(filename);
        if (std::filesystem::exists(absPath) && std::filesystem::is_regular_file(absPath)) {
            if (auto data = matchPredicate(absPath)) {
                return std::make_pair(absPath, *data);
            }
        }
        return std::nullopt;
    }
    
    auto paths = splitPath(searchPaths);
    
    for (const auto& directory : paths) {
        try {
            if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory)) {
                continue;
            }
            
            for (const auto& entry : std::filesystem::directory_iterator(directory)) {
                if (entry.is_regular_file() && 
                    entry.path().filename() == filename) {
                    if (auto data = matchPredicate(entry.path())) {
                        return std::make_pair(std::filesystem::absolute(entry.path()), *data);
                    }
                }
            }
        } catch (const std::filesystem::filesystem_error&) {
            continue;
        }
    }
    
    return std::nullopt;
}





#ifdef __APPLE__
void relax_macos_malloc_zones();
#else
inline void relax_macos_malloc_zones() {}
#endif
