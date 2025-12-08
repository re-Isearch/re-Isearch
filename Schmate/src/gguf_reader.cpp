#include "gguf_reader.hpp"
#include <stdexcept>
#include <iostream>

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

