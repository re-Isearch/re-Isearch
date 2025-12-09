
#pragma once
#include <cstdint>
#include <iostream>
#include <fstream>
#include <stdexcept>

#include "hnswlib/int_storage.h"

inline void write_int64(std::ostream &os, int64_t v) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    int64_t out = v;
#else
    int64_t out = __builtin_bswap64(v);
#endif
    os.write(reinterpret_cast<const char *>(&out), sizeof(out));
}

inline int64_t read_int64(std::istream &is) {
    int64_t in;
    is.read(reinterpret_cast<char *>(&in), sizeof(in));
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return in;
#else
    return __builtin_bswap64(in);
#endif
}

struct GmmlHparams {
    uint32_t magic;           // 'ggml' = 0x67676d6c
    int32_t n_vocab;          // Vocabulary size
    int32_t n_max_tokens;     // Max sequence length
    int32_t n_embd;           // Embedding dimensions
    int32_t n_intermediate;   // Intermediate/FFN dimensions
    int32_t n_head;           // Number of attention heads
    int32_t n_layer;          // Number of layers
    int32_t f16;              // Quantization type: 0=F32, 1=F16, 2=Q4_0, 3=Q4_1, etc.
};

struct GGUFInfo {
    std::string architecture;
    uint32_t embedding_length = 0;
    std::string quant_type;     // Human-readable ("F16", "Q4_0", ...)
};

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

// Get the storage type/quantization of a GGML model
std::pair<hnswlib::StorageType, std::string> get_ggml_model_quant(const std::string &filename) ;


#include <filesystem>


// File exist and length utils
inline bool file_exists(const std::string &p) {
      return std::filesystem::exists(p);
}
// Does not exist? Return -1 else return its size
inline off_t file_size(const std::string &p) {
    if (!file_exists(p)) return -1;
    return std::filesystem::file_size(p);
}

#pragma once
#ifdef __APPLE__
void relax_macos_malloc_zones();
#else
inline void relax_macos_malloc_zones() {}
#endif
