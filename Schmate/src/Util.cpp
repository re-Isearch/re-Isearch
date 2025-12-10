#include "Util.hpp"
#include "Logger.hpp"


#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <string>

#include <string>
#include <unordered_map>
#include <array>

using namespace hnswlib;

/*

GGML Codes:

| Code (`f16` value) | Quantization Type | Bits per weight | Description                     |
| :----------------: | :---------------- | :-------------: | :------------------------------ |
|        **0**       | `F32`             |        32       | Full precision                  |
|        **1**       | `F16`             |        16       | Half precision                  |
|        **2**       | `Q4_0`            |        4        | 4-bit block quantization        |
|        **3**       | `Q4_1`            |        4        | 4-bit with scale + bias         |
|      Beyond here bert.cpp does not provide support (4&5 are not even defined)              |
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

In some newer implmentations we see some different values > 19. 

*/


namespace {
    struct QuantMapping {
        uint32_t             code;         // 32-bit int code in the .gmml file
        const char*          name;         // Quantization Type name (.gguf)
        hnswlib::StorageType storage_type; // Int storage (important for pass-through)
    };
    

// The major issue are code 4 and 20. In some older models 20 was BF16. In newer that 
// code is a 4-bit quantization type and BF16 is code 30. 4 was by Ollama a FP4 now 39
//
// So depending upon version. 20 can be 4-bit int or a 16-bit floating point!
// A 4 can be an obsolete 4-bit int or a new 4-bit floating point.

    // See ggml/include/ggml.h
    constexpr std::array<QuantMapping, 32> quant_mappings = {{
        {0, "F32", StorageType::FLOAT32},
        {1, "F16", StorageType::FP16},
        {2, "Q4_0", StorageType::INT4},
        {3, "Q4_1", StorageType::INT4},

        // bert.cpp stops here
        // Rest are from ggml.h
//      {4, "Q4_2", StorageType::INT4}, // Support has been removed!
//      {5, "Q4_3", StorageType::INT4}, // Support has been removed!

        {6, "Q5_0", StorageType::INT5},
        {7, "Q5_1", StorageType::INT5},
        {8, "Q8_0", StorageType::INT8},
        {9, "Q8_1", StorageType::INT8},
        {10, "Q2_K", StorageType::INT2},
        {11, "Q3_K", StorageType::INT3},
        {12, "Q4_K", StorageType::INT4},
        {13, "Q5_K", StorageType::INT5},
        {14, "Q6_K", StorageType::INT6},
        {15, "Q8_K", StorageType::INT8},
        {16, "IQ2_XXS", StorageType::INT2},
        {17, "IQ2_XS", StorageType::INT2},
        {18, "IQ3_XXS", StorageType::INT3},
        {19, "IQ1_S", StorageType::BIN1},
        {20, "IQ4_NL", StorageType::INT4},  // Note: original code had "BF16" here
        {21, "IQ3_S", StorageType::INT3},
        {22, "IQ2_S", StorageType::INT2},
        {23, "IQ4_XS", StorageType::INT4},
        {24, "I8", StorageType::INT8},
        {25, "I16", StorageType::INT16},
        {26, "I32", StorageType::INT32},
        {27, "I64", StorageType::INT64}, // GGML does not support 64 bits
        {28, "F64", StorageType::FLOAT64}, // GGML does not support Fp64
        {29, "IQ1_M", StorageType::BIN1},
        {30, "BF16", StorageType::FP16},  // BFloat16 support added here, waas 20
        // GGML_TYPE_Q4_0_4_4 = 31, support has been removed from gguf files
        // GGML_TYPE_Q4_0_4_8 = 32,
        // GGML_TYPE_Q4_0_8_8 = 33,
        {34, "TQ1_0", StorageType::BIN1},
        {35, "TQ2_0", StorageType::INT2},
        // GGML_TYPE_IQ4_NL_4_4 = 36,
        // GGML_TYPE_IQ4_NL_4_8 = 37,
        // GGML_TYPE_IQ4_NL_8_8 = 38,

        // Below is very new
        {39, "MXFP4", StorageType::INT4} 
/*
        Name: "MXFP4" or sometimes "mxfp4-e8m0"
        Microscaling 4-bit Floating Point with 8-bit exponent, 0-bit mantissa

        A native training format: Unlike other quantization methods that reduce precision
        after training, gpt-oss models were trained directly in MXFP4 format Code number 39
        in the GGML type enum
        4-bit format: Uses 4 bits per weight but maintains higher quality than traditional Q4 quantization
        Block-based: Stores weights in blocks of 32 elements with shared scaling factors

        NOTE: There was an inconsistency where Ollama initially used code 4 for MXFP4, but
        llama.cpp officially assigned it code 39. This caused compatibility issues with models
        converted using different tools.

        This means that sometimes 4 -> MXFP4 and sometimes an obsolete 4-bit quantization.
        Since we are building around sBERT models it does not matter since we only expect to
        get from {0, 1, 2, 3}
*/



    }};
}

const std::string ggml_quant_name(uint32_t code) {
    for (const auto& mapping : quant_mappings) {
        if (mapping.code == code) {
            return mapping.name;
        }
    }
    return "Unknown(" + std::to_string(code) + ")";
}

int ggml_name_to_quant(const std::string& name) {
    static std::unordered_map<std::string, int> name_map;
    
    // Build map on first call
    if (name_map.empty()) {
        for (const auto& mapping : quant_mappings) {
            name_map[mapping.name] = mapping.code;
        }
    }
    
    auto it = name_map.find(name);
    return (it != name_map.end()) ? it->second : -1;
}



hnswlib::StorageType ggml_quant_type(uint32_t code) {
    for (const auto& mapping : quant_mappings) {
        if (mapping.code == code) {
            return mapping.storage_type;
        }
    }
    return hnswlib::StorageType::FLOAT32;  // Default for unknown codes
}


// The below could have also used the above.. 
hnswlib::StorageType ggml_name_to_quant_type(const std::string& name) {
    static std::unordered_map<std::string, StorageType> type_map;
    
    // Build map on first call
    if (type_map.empty()) {
        for (const auto& mapping : quant_mappings) {
            type_map[mapping.name] = mapping.storage_type;
        }
    }
    
    auto it = type_map.find(name);
    return (it != type_map.end()) ? it->second : StorageType::FLOAT32;
}


///////////


std::optional<GmmlHparams> read_ggml_info(const std::string& path) {
   GmmlHparams info;

   if (file_size(path) > sizeof(GmmlHparams)) {
       std::ifstream fin(path, std::ios::binary);
       if (fin) {
           return read_le<GmmlHparams>(fin);
//         GmmlHparams info;
//         fin.read(reinterpret_cast<char*>(&info), sizeof(info));
//         return info;
        }
    }
   return std::nullopt;
}

std::pair<hnswlib::StorageType, std::string>
	get_ggml_model_quant(const std::string &filepath)
{
    auto hdr = read_ggml_info (filepath);
    int code = hdr ? hdr->f16 : 0;
    return  {  ggml_quant_type(code), ggml_quant_name(code) };
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


#if 1

std::optional<GGUFInfo> read_gguf_info(const std::string &path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return std::nullopt;

    // --- GGUF header ---
    uint32_t magic = read_le<uint32_t>(f);
    if (magic != 0x46554747) {   // "GGUF"
        return std::nullopt; // Not a GGUF file
    }

    uint32_t version = read_le<uint32_t>(f);
    uint64_t n_kv = read_le<uint64_t>(f);
    uint64_t n_tensors = read_le<uint64_t>(f);

    GGUFInfo info;

    // ---- Read key/value metadata ----
    for (uint64_t i = 0; i < n_kv; i++) {
        uint32_t key_len = read_le<uint32_t>(f);
        std::string key(key_len, 0);
        f.read(&key[0], key_len);

        uint32_t vtype = read_le<uint32_t>(f);  // GGUF_METADATA_TYPE_*

        // Only reading strings + integers (all we need)
        if (vtype == 0) {  // string
            uint32_t vlen = read_le<uint32_t>(f);
            std::string value(vlen, 0);
            f.read(&value[0], vlen);

            if (key == "general.architecture")
                info.architecture = value;

        } else if (vtype == 2) {   // uint32
            uint32_t v = read_le<uint32_t>(f);

            if (key == "llama.embedding_length" ||
                key == "bert.embedding_length" ||
                key == "embedding_length")
                info.embedding_length = v;
        }
        else {
            // skip unsupported types
            uint64_t skip = 0;
            switch (vtype) {
                case 1: skip = read_le<uint32_t>(f); break;   // array – skip length then each element
                case 3: skip = sizeof(float); break;
                case 4: skip = 1; break;
                case 5: skip = 8; break;
                default: break;
            }
            f.seekg(skip, std::ios::cur);
        }
    }

    // --- Now read first tensor to get quantization ---
    // Each tensor record:
    //   uint32 name_len
    //   char[name_len] name
    //   uint32 n_dims
    //   uint32 dims[n_dims]
    //   uint32 type (ggml_type)
    //   uint64 offset

    if (n_tensors > 0) {
        uint32_t name_len = read_le<uint32_t>(f);
        f.seekg(name_len, std::ios::cur);  // skip name

        uint32_t n_dims = read_le<uint32_t>(f);
        f.seekg(sizeof(uint32_t) * n_dims, std::ios::cur);  // skip dims

        uint32_t type = read_le<uint32_t>(f);
        // Use the unified mapping function
        info.quant_type = ggml_quant_name(type);

        // skip offset
        f.seekg(sizeof(uint64_t), std::ios::cur);
    }

    return info;
}

#else

enum GGMLType : uint32_t {
    GGML_TYPE_F32 = 0,
    GGML_TYPE_F16 = 1,
    GGML_TYPE_Q4_0 = 2,
    GGML_TYPE_Q4_1 = 3,
    GGML_TYPE_Q5_0 = 6,
    GGML_TYPE_Q5_1 = 7,
    GGML_TYPE_Q8_0 = 8,
    GGML_TYPE_UNKNOWN = 0xFFFFFFFF
};

static const std::unordered_map<uint32_t,std::string> GGML_TYPE_NAMES = {
    {0, "F32"}, {1, "F16"}, {2, "Q4_0"}, {3, "Q4_1"},
    {6, "Q5_0"}, {7, "Q5_1"}, {8, "Q8_0"},        
};  

std::optional<GGUFInfo> read_gguf_info(const std::string &path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return std::nullopt;

    // --- GGUF header ---
    uint32_t magic = read_le<uint32_t>(f);
    if (magic != 0x46554747) {   // "GGUF"
        return std::nullopt; // Not a GGUF file
    }

    uint32_t version = read_le<uint32_t>(f);
    uint64_t n_kv = read_le<uint64_t>(f);
    uint64_t n_tensors = read_le<uint64_t>(f);

    GGUFInfo info;

    // ---- Read key/value metadata ----
    for (uint64_t i = 0; i < n_kv; i++) {
        uint32_t key_len = read_le<uint32_t>(f);
        std::string key(key_len, 0);
        f.read(&key[0], key_len);

        uint32_t vtype = read_le<uint32_t>(f);  // GGUF_METADATA_TYPE_*

        // Only reading strings + integers (all we need)
        if (vtype == 0) {  // string
            uint32_t vlen = read_le<uint32_t>(f);
            std::string value(vlen, 0);
            f.read(&value[0], vlen);

            if (key == "general.architecture")
                info.architecture = value;

        } else if (vtype == 2) {   // uint32
            uint32_t v = read_le<uint32_t>(f);

            if (key == "llama.embedding_length" ||
                key == "bert.embedding_length" ||
                key == "embedding_length")
                info.embedding_length = v;
        }
        else {
            // skip unsupported types
            uint64_t skip = 0;
            switch (vtype) {
                case 1: skip = read_le<uint32_t>(f); break;   // array — skip length then each element
                case 3: skip = sizeof(float); break;
                case 4: skip = 1; break;
                case 5: skip = 8; break;
                default: break;
            }
            f.seekg(skip, std::ios::cur);
        }
    }

    // --- Now read first tensor to get quantization ---
    // Each tensor record:
    //   uint32 name_len
    //   char[name_len] name
    //   uint32 n_dims
    //   uint32 dims[n_dims]
    //   uint32 type (ggml_type)
    //   uint64 offset

    if (n_tensors > 0) {
        uint32_t name_len = read_le<uint32_t>(f);
        f.seekg(name_len, std::ios::cur);  // skip name

        uint32_t n_dims = read_le<uint32_t>(f);
        f.seekg(sizeof(uint32_t) * n_dims, std::ios::cur);  // skip dims

        uint32_t type = read_le<uint32_t>(f);
        info.quant_type =
            GGML_TYPE_NAMES.count(type) ? GGML_TYPE_NAMES.at(type) :
            ("UNKNOWN(" + std::to_string(type) + ")");

        // skip offset
        f.seekg(sizeof(uint64_t), std::ios::cur);
    }

    return info;
}
#endif

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


