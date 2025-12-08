#pragma once
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <stdexcept>
#include <cstring>

//
// GGUF format reference: https://github.com/ggerganov/ggml/blob/master/docs/gguf.md
//

struct GGUFField {
    std::string key;
    std::string value;
};

class GGUFParser {
public:
    explicit GGUFParser(const std::string &path) {
        parse(path);
    }

    bool is_valid() const { return valid; }

    const std::string &get_arch() const { return arch; }
    const std::string &get_quantization() const { return quantization; }
    uint32_t get_embedding_size() const { return embedding_size; }

    void print_info() const {
        std::cout << "File: " << file_path << "\n";
        std::cout << "Magic: " << magic << "\n";
        std::cout << "Version: " << version << "\n";
        std::cout << "Architecture: " << arch << "\n";
        std::cout << "Quantization: " << quantization << "\n";
        std::cout << "Embedding size: " << embedding_size << "\n";
        std::cout << "Fields: " << fields.size() << "\n";
        for (auto &f : fields) {
            std::cout << "  - " << f.key << " = " << f.value << "\n";
        }
    }

private:
    std::string file_path;
    std::string magic;
    uint32_t version = 0;
    bool valid = false;

    std::vector<GGUFField> fields;

    std::string arch;
    std::string quantization;
    uint32_t embedding_size = 0;

    void parse(const std::string &path) {
        file_path = path;
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs) throw std::runtime_error("Cannot open file: " + path);

        // read magic
        char magic_buf[4];
        ifs.read(magic_buf, 4);
        if (ifs.gcount() != 4) throw std::runtime_error("Invalid file");
        magic.assign(magic_buf, 4);

        if (magic != "GGUF") {
            // try GGML older format
            if (magic == "GGML") {
                valid = parse_ggml_header(ifs);
                return;
            }
            throw std::runtime_error("Unknown magic: " + magic);
        }

        // read version
        ifs.read(reinterpret_cast<char*>(&version), sizeof(uint32_t));

        uint64_t n_tensors = 0;
        uint64_t n_kv = 0;
        ifs.read(reinterpret_cast<char*>(&n_tensors), 8);
        ifs.read(reinterpret_cast<char*>(&n_kv), 8);

        for (uint64_t i = 0; i < n_kv; i++) {
            std::string key = read_string(ifs);
            uint32_t vtype;
            ifs.read(reinterpret_cast<char*>(&vtype), 4);

            std::string val = read_value(ifs, vtype);
            fields.push_back({key, val});

            if (key.find("general.architecture") != std::string::npos) arch = val;
            if (key.find("general.quantization_version") != std::string::npos) quantization = val;
            if (key.find("embedding_length") != std::string::npos) embedding_size = std::stoul(val);
        }

        valid = true;
    }

    bool parse_ggml_header(std::ifstream &ifs) {
        uint32_t version, n_vocab, n_embd, n_layer, f16;
        ifs.read((char*)&version, 4);
        ifs.read((char*)&n_vocab, 4);
        ifs.read((char*)&n_embd, 4);
        ifs.read((char*)&n_layer, 4);
        ifs.read((char*)&f16, 4);

        arch = "ggml";
        quantization = ggml_quantization_name(f16);
        embedding_size = n_embd;
        version = version;
        valid = true;
        return true;
    }

    std::string read_string(std::ifstream &ifs) {
        uint64_t len;
        ifs.read(reinterpret_cast<char*>(&len), 8);
        std::string s(len, '\0');
        ifs.read(&s[0], len);
        return s;
    }

    std::string read_value(std::ifstream &ifs, uint32_t vtype) {
        switch (vtype) {
            case 0: { // uint8
                uint8_t v; ifs.read((char*)&v, 1);
                return std::to_string(v);
            }
            case 1: { // int8
                int8_t v; ifs.read((char*)&v, 1);
                return std::to_string(v);
            }
            case 2: { // uint16
                uint16_t v; ifs.read((char*)&v, 2);
                return std::to_string(v);
            }
            case 3: { // int16
                int16_t v; ifs.read((char*)&v, 2);
                return std::to_string(v);
            }
            case 4: { // uint32
                uint32_t v; ifs.read((char*)&v, 4);
                return std::to_string(v);
            }
            case 5: { // int32
                int32_t v; ifs.read((char*)&v, 4);
                return std::to_string(v);
            }
            case 6: { // float32
                float v; ifs.read((char*)&v, 4);
                return std::to_string(v);
            }
            case 7: { // bool
                uint8_t v; ifs.read((char*)&v, 1);
                return v ? "true" : "false";
            }
            case 8: { // string
                return read_string(ifs);
            }
            default:
                return "[unknown-type-" + std::to_string(vtype) + "]";
        }
    }

    std::string ggml_quantization_name(uint32_t f16) const {
        static std::map<uint32_t, std::string> table = {
            {0, "F32"}, {1, "F16"}, {2, "Q4_0"}, {3, "Q4_1"},
            {4, "Q5_0"}, {5, "Q5_1"}, {6, "Q8_0"}, {7, "Q8_1"},
            {8, "Q2_K"}, {9, "Q3_K"}, {10, "Q4_K"}, {11, "Q5_K"}, {12, "Q6_K"}
        };
        auto it = table.find(f16);
        return (it != table.end()) ? it->second : "Unknown(" + std::to_string(f16) + ")";
    }
};

