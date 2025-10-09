// Util.cpp 

#include "Util.hpp"


static bool isSBERTGGMLFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);

    // GGML files start with a magic number (4 bytes): "ggml" or "ggjt" or other variants
    uint32_t magic;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    
    // Check for valid GGML magic numbers
    const uint32_t GGML_MAGIC = 0x67676d6c; // "ggml" in little-endian
    const uint32_t GGJT_MAGIC = 0x67676a74; // "ggjt" in little-endian
    const uint32_t GGLA_MAGIC = 0x67676c61; // "ggla" in little-endian
    const uint32_t GGMF_MAGIC = 0x67676d66; // "ggmf" in little-endian
    const uint32_t GGUF_MAGIC = 0x46554747; // "GGUF" in little-endian
    
    bool valid_magic = (magic == GGML_MAGIC || magic == GGJT_MAGIC || 
                        magic == GGLA_MAGIC || magic == GGMF_MAGIC ||
                        magic == GGUF_MAGIC);
    
    if (!valid_magic) {
        std::cerr << "Invalid magic number: 0x" << std::hex << magic << std::dec << std::endl;
        return false;
    }
    
    // Read version (4 bytes)
    uint32_t version;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    
    // For older GGML formats (not GGUF), the next fields are hyperparameters
    // The order is typically: n_vocab, n_embd, n_mult, n_head, n_layer, n_rot, ftype
    int32_t n_vocab, n_embd, n_mult, n_head, n_layer;
    file.read(reinterpret_cast<char*>(&n_vocab), sizeof(n_vocab));
    file.read(reinterpret_cast<char*>(&n_embd), sizeof(n_embd));
    file.read(reinterpret_cast<char*>(&n_mult), sizeof(n_mult));
    file.read(reinterpret_cast<char*>(&n_head), sizeof(n_head));
    file.read(reinterpret_cast<char*>(&n_layer), sizeof(n_layer));

    file.close();
    
    std::cerr << "Magic: 0x" << std::hex << magic << std::dec << std::endl;
    std::cerr << "Version: " << version << std::endl;
    std::cerr << "n_vocab = " << n_vocab << ", n_embd = " << n_embd 
              << ", n_mult = " << n_mult << ", n_head = " << n_head 
              << ", n_layer = " << n_layer << std::endl;
    
    // Validate the values make sense
    // n_mult is typically 4 * n_embd (for FFN intermediate size)
    return n_embd > 0 && n_embd <= 16384 &&
           n_layer > 0 && n_layer <= 128 &&
           n_vocab > 0 && n_vocab <= 200000 &&
           n_head > 0 && n_head <= 256 &&
           n_mult >= 0 && n_mult <= 65536 &&
           (n_embd % n_head == 0);  // n_embd must be divisible by n_head
}


#include <filesystem>
#include <optional>
#include <sstream>

namespace fs = std::filesystem;

static std::vector<std::string> splitPath(const std::string& pathStr, char delimiter = 
#if defined(_MSDOS) || defined(_WIN32)
	';'
#else
	':'
#endif
	) {
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
    auto paths = splitPath(searchPaths);
    
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



#ifdef __APPLE__
#include <malloc/malloc.h>
#include <dlfcn.h>
#include <iostream>

// Optional safety shim in case we build on older SDKs.
typedef void (*malloc_zone_pressure_relief_t)(void*, size_t);

void relax_macos_malloc_zones() {
    // On macOS 11+, malloc_zone_pressure_relief() is available.
    void* handle = dlopen("/usr/lib/libSystem.dylib", RTLD_NOW);
    if (!handle) return;

    malloc_zone_pressure_relief_t fn =
        (malloc_zone_pressure_relief_t)dlsym(handle, "malloc_zone_pressure_relief");

    if (fn) {
        malloc_zone_t* default_zone = malloc_default_zone();
        // 0 means "release as much as you can"
        fn(default_zone, 0);
        // std::cerr << "[INFO] macOS allocator zones relaxed.\n";
    }
    dlclose(handle);
}
#endif

