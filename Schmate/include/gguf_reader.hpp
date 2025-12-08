#pragma once
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

struct GGUFInfo {
    std::string architecture;
    uint32_t embedding_length = 0;
    std::string quant_type;     // Human-readable ("F16", "Q4_0", ...)
};

enum GGMLType : uint32_t {
    GGML_TYPE_F32 = 0,
    GGML_TYPE_F16 = 1,
    GGML_TYPE_Q4_0 = 2,
    GGML_TYPE_Q4_1 = 3,
    GGML_TYPE_Q5_0 = 6,
    GGML_TYPE_Q5_1 = 7,
    GGML_TYPE_Q8_0 = 8,
};

static const std::unordered_map<uint32_t,std::string> GGML_TYPE_NAMES = {
    {0, "F32"}, {1, "F16"}, {2, "Q4_0"}, {3, "Q4_1"},
    {6, "Q5_0"}, {7, "Q5_1"}, {8, "Q8_0"},
};

// Utility to read little-endian values
template<typename T>
inline T read_le(std::ifstream &f) {
    T v;
    f.read(reinterpret_cast<char*>(&v), sizeof(T));
    return v;
}

std::optional<GGUFInfo> read_gguf_info(const std::string &path);

