// Util.cpp 

#include "Util.hpp"

static bool validateGGMLHeader(std::ifstream& file) {
    uint32_t version;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    
    int32_t n_vocab, n_embd, n_layer;
    file.read(reinterpret_cast<char*>(&n_vocab), sizeof(n_vocab));
    file.read(reinterpret_cast<char*>(&n_embd), sizeof(n_embd));
    file.read(reinterpret_cast<char*>(&n_layer), sizeof(n_layer));
    
    return n_embd > 0 && n_embd <= 2048 &&
           n_layer > 0 && n_layer <= 48 &&
           n_vocab > 0 && n_vocab <= 100000;
}


static bool validateGGJTHeader(std::ifstream& file) {
    uint32_t version;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    
    // GGJT has similar structure but may have additional fields
    int32_t n_vocab, n_embd, n_mult, n_head, n_layer;
    file.read(reinterpret_cast<char*>(&n_vocab), sizeof(n_vocab));
    file.read(reinterpret_cast<char*>(&n_embd), sizeof(n_embd));
    file.read(reinterpret_cast<char*>(&n_mult), sizeof(n_mult));
    file.read(reinterpret_cast<char*>(&n_head), sizeof(n_head));
    file.read(reinterpret_cast<char*>(&n_layer), sizeof(n_layer));
    
    return n_embd > 0 && n_layer > 0 && n_vocab > 0;
}

static inline bool validateGGMFHeader(std::ifstream& file) {
    uint32_t version;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    
    // Similar to GGML but older format
    int32_t n_vocab, n_embd;
    file.read(reinterpret_cast<char*>(&n_vocab), sizeof(n_vocab));
    file.read(reinterpret_cast<char*>(&n_embd), sizeof(n_embd));
    
    return n_embd > 0 && n_vocab > 0;
}

static bool validateGGUFHeader(std::ifstream& file) {
    uint32_t version;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    
    // GGUF uses a key-value metadata system
    uint64_t tensor_count, metadata_kv_count;
    file.read(reinterpret_cast<char*>(&tensor_count), sizeof(tensor_count));
    file.read(reinterpret_cast<char*>(&metadata_kv_count), sizeof(metadata_kv_count));
    
    return tensor_count > 0 && metadata_kv_count >= 0;
}



static bool isSBERTGGMLFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) return false;
    
    uint32_t magic;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    
    switch(magic) {
        case 0x67676d6c: // "ggml"
            return validateGGMLHeader(file);
        case 0x67676a74: // "ggjt"
            return validateGGJTHeader(file);
        case 0x67676d66: // "ggmf"
            return validateGGMFHeader(file);
        case 0x46554747: // "GGUF"
            return validateGGUFHeader(file);
        default:
            return false;
    }
}

#include <filesystem>
#include <optional>
#include <sstream>

namespace fs = std::filesystem;

static std::vector<std::string> splitPath(const std::string& pathStr, char delimiter = ':') {
    std::vector<std::string> paths;
    std::stringstream ss(pathStr);
    std::string path;
    
    while (std::getline(ss, path, delimiter)) {
        if (!path.empty()) {
            paths.push_back(path);
        }
    }
    
    return paths;
}

static std::optional<std::string> findSBERTFile(const std::string& searchPaths, 
                                          const std::string& filename) {
    auto paths = splitPath(searchPaths
#if defined(_MSDOS) || defined(_WIN32) 
	, ';'
#endif
	);
    
    for (const auto& directory : paths) {
        try {
            for (const auto& entry : fs::directory_iterator(directory)) {
                if (entry.is_regular_file() && 
                    entry.path().filename() == filename &&
                    isSBERTGGMLFile(entry.path().string())) {
                    return entry.path().string();
                }
            }
        } catch (const fs::filesystem_error&) {
            // Continue to next directory if this one fails
            continue;
        }
    }
    
    return std::nullopt;
}


std::string find_ggml_model(const std::string &filename, const std::string& Path)
{
  auto result = findSBERTFile(Path, filename);
  if (result) return *result;
  return filename; // NOT FOUND

}
