#include "Util.hpp"
#include "Logger.hpp"


#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <string>

using namespace hnswlib;

/*

GGML Codes:

| Code (`f16` value) | Quantization Type | Bits per weight | Description                     |
| :----------------: | :---------------- | :-------------: | :------------------------------ |
|        **0**       | `F32`             |        32       | Full precision                  |
|        **1**       | `F16`             |        16       | Half precision                  |
|        **2**       | `Q4_0`            |        4        | 4-bit block quantization        |
|        **3**       | `Q4_1`            |        4        | 4-bit with scale + bias         |
|        **6**       | `Q5_0`            |        5        | 5-bit quantization              |
|        **7**       | `Q5_1`            |        5        | 5-bit w/ bias                   |
|        **8**       | `Q8_0`            |        8        | 8-bit uniform                   |
|        **9**       | `Q8_1`            |        8        | 8-bit w/ bias                   |
|       **10**       | `Q2_K`            |        2        | 2-bit “K-block” quantization    |
|       **11**       | `Q3_K`            |        3        | 3-bit K-block                   |
|       **12**       | `Q4_K`            |        4        | 4-bit K-block base              |
|       **13**       | `Q5_K`            |        5        | 5-bit K-block                   |
|       **14**       | `Q6_K`            |        6        | 6-bit K-block                   |
|       **15**       | `Q8_K`            |        8        | 8-bit K-block                   |
|       **16**       | `IQ2_XXS`         |        2        | “Improved Quantization” variant |
|       **17**       | `IQ2_XS`          |        2        |                                 |
|       **18**       | `IQ3_XS`          |        3        |                                 |
|       **19**       | `IQ1_S`           |        1        | 1-bit ultra-low precision       |
|       **20**       | `BF16`            |        16       | bfloat16                        |

*/

static std::string ggml_quant_name(uint32_t code) {
    switch (code) {
        case 0: return "F32";
        case 1: return "F16";
        case 2: return "Q4_0";
        case 3: return "Q4_1";
        case 6: return "Q5_0";
        case 7: return "Q5_1";
        case 8: return "Q8_0";
        case 9: return "Q8_1";
        case 10: return "Q2_K";
        case 11: return "Q3_K";
        case 12: return "Q4_K";
        case 13: return "Q5_K";
        case 14: return "Q6_K";
        case 15: return "Q8_K";
        case 16: return "IQ2_XXS";
        case 17: return "IQ2_XS";
        case 18: return "IQ3_XS";
        case 19: return "IQ1_S";
        case 20: return "BF16";
        default: return "Unknown(" + std::to_string(code) + ")";
    }
}


static StorageType ggml_quant_type(uint32_t code) {
    switch (code) {
        case  1: return StorageType::FP16;
        case  2: return StorageType::INT4;
        case  3: return StorageType::INT4;
        case  6: return StorageType::INT5;           
        case  7: return StorageType::INT5;           
        case  8: return StorageType::INT8;           
        case  9: return StorageType::INT8;           
        case 10: return StorageType::INT2;
        case 11: return StorageType::INT3;          
        case 12: return StorageType::INT4;          
        case 13: return StorageType::INT5;          
        case 14: return StorageType::INT6;          
        case 15: return StorageType::INT8;          
        case 16: return StorageType::INT2;
        case 17: return StorageType::INT2;
        case 18: return StorageType::INT3;
        case 19: return StorageType::BIN1;
        case 20: return StorageType::FP16;          
        case  0:
        default:
            return StorageType::FLOAT32;
    }
}


std::optional<GmmlHparams> read_ggml_info(const std::string& path) {
   GmmlHparams info;

   if (file_size(path) > sizeof(GmmlHparams)) {
       std::ifstream fin(path, std::ios::binary);
       if (fin) {
           GmmlHparams info; 
           fin.read(reinterpret_cast<char*>(&info), sizeof(info));
           return info;
        }
    }
   return std::nullopt;
}

std::pair<hnswlib::StorageType, std::string>
	get_ggml_model_quant(const std::string &filepath)
{
    auto hdr = read_ggml_info (filepath);
    return  {  ggml_quant_type(hdr->f16), ggml_quant_name(hdr->f16) };
}


static GGML_TYPE FileType(const std::string& filepath) {
    enum GGML_TYPE type = GGML_TYPE::UNKNOWN;
    // Check for valid GGML magic numbers
    const uint32_t GGML_MAGIC = 0x67676d6c; // "ggml" in little-endian
    const uint32_t GGJT_MAGIC = 0x67676a74; // "ggjt" in little-endian
    const uint32_t GGLA_MAGIC = 0x67676c61; // "ggla" in little-endian
    const uint32_t GGMF_MAGIC = 0x67676d66; // "ggmf" in little-endian
    const uint32_t GGUF_MAGIC = 0x46554747; // "GGUF" in little-endian

    auto hdr = read_ggml_info (filepath);
    if (hdr) {
        uint32_t magic = hdr->magic;
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        magic = __builtin_bswap32(magic);
#endif
        bool valid_magic = (magic == GGML_MAGIC || magic == GGJT_MAGIC || 
                        magic == GGLA_MAGIC || magic == GGMF_MAGIC ||
                        magic == GGUF_MAGIC);
        if (!valid_magic) {
            std::cerr << "Invalid magic number: 0x" << std::hex << magic << std::dec << std::endl;
            return type ;
        }
        type = ( magic == GGUF_MAGIC) ? GGML_TYPE::GGUF : GGML_TYPE::GGML;
     }
#if 1
   return type ;
#else
    // Validate the values make sense
    return hdr.n_embd > 0 && hdr.n_embd <= 16384 &&
           hdr.n_layer > 0 && hdr.n_layer <= 128 &&
           hdr.n_vocab > 0 && hdr.n_vocab <= 200000 &&
           hdr.n_head > 0 && hdr.n_head <= 256 &&
           (hdr.n_embd % hdr.n_head == 0) ? type : GGML_TYPE::UNKNOWN;  // n_embd must be divisible by n_head
#endif
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

std::pair <std::string, enum GGML_TYPE>
	find_ggml_model(const std::string& filename, const std::string& searchPaths ){
    auto paths = splitPath(searchPaths);
    
    for (const auto& directory : paths) {
        enum GGML_TYPE type;
        try {
            for (const auto& entry : fs::directory_iterator(directory)) {
                if (entry.is_regular_file() && 
                    entry.path().filename() == filename &&
                    (type = FileType(entry.path().string())) != GGML_TYPE::UNKNOWN) {
                    return {entry.path().string(), type};
                }
            }
        } catch (const fs::filesystem_error&) {
            // Continue to next directory if this one fails
            continue;
        }
    }
    
    return {filename,  GGML_TYPE::UNKNOWN};
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

#include <thread>

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
#elif defined(__APPLE__) || defined(__MACH__)
    #include <sys/sysctl.h>
#elif defined(__linux__) || defined(__unix__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    #include <unistd.h>
    #if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
        #include <sys/sysctl.h>
    #endif
#endif

int getThreadCount() {
    // Try the standard C++ method first
    unsigned int threadCount = std::thread::hardware_concurrency();
    
    if (threadCount != 0) {
        return threadCount;
    }
    
    // Fallback to platform-specific methods if hardware_concurrency() returns 0
    #if defined(_WIN32) || defined(_WIN64)
        SYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo);
        return sysinfo.dwNumberOfProcessors;
    
    #elif defined(__APPLE__) || defined(__MACH__)
        int nm[2];
        size_t len = 4;
        uint32_t count;
        
        nm[0] = CTL_HW;
        nm[1] = HW_AVAILCPU;
        sysctl(nm, 2, &count, &len, NULL, 0);
        
        if(count < 1) {
            nm[1] = HW_NCPU;
            sysctl(nm, 2, &count, &len, NULL, 0);
            if(count < 1) { count = 1; }
        }
        return count;
    
    #elif defined(__linux__)
        return sysconf(_SC_NPROCESSORS_ONLN);
    
    #elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
        int nm[2];
        size_t len = 4;
        int count;
        
        nm[0] = CTL_HW;
        nm[1] = HW_NCPU;
        sysctl(nm, 2, &count, &len, NULL, 0);
        
        return count;
    
    #else
        return 1; // Default fallback
    #endif
}


