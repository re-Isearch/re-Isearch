#include <fstream>
#include <vector>
#include <string>
#include <string_view>
#include <cstdint>
#include <iostream>

#include "hf_model_mapper_format.hpp"
using namespace hfmap;

/*
# hf_models.txt
llama2_7b=TheBloke/Llama-2-7B-GGUF|llama-2-7b.Q4_K_M.gguf
llama2_13b=TheBloke/Llama-2-13B-GGUF|llama-2-13b.Q4_K_M.gguf
mistral_7b=TheBloke/Mistral-7B-Instruct-v0.2-GGUF|mistral-7b-instruct-v0.2.Q4_K_M.gguf
codellama=TheBloke/CodeLlama-13B-GGUF|codellama-13b.Q4_K_M.gguf
mixtral=TheBloke/Mixtral-8x7B-Instruct-v0.1-GGUF|mixtral-8x7b-instruct-v0.1.Q4_K_M.gguf
*/



static size_t next_pow2(size_t v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    return ++v;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "HuggingFace Map Compiler:\n\
  Usage: " << argv[0] << " input.txt output.bin\n\
  Format: key=repro|filename with # as comment line\n\
  Valid keys may not contain the '=' character." << std::endl;

        return 1;
    }

    std::ifstream in(argv[1]);
    if (!in) {
        std::cerr << "Failed to open input\n";
        return 1;
    }

    std::vector<std::pair<std::string, std::string>> items;
    std::string line;

    size_t lineno = 1;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#')
            continue;

        auto pos = line.find('=');
        if (pos == std::string::npos) {
            std::cerr << "Line#" << lineno << " does not contain a = character, skipping.\n";
            continue;
        }

        if (line.find('|') == std::string::npos) {
            std::cerr << "Line#" << lineno << " does not contain a | character, skipping.\n";
            continue;
        }


        items.emplace_back(
            line.substr(0, pos),
            line.substr(pos + 1)
        );
        lineno++;
    }

    size_t table_size = next_pow2(items.size() * 2);
    std::vector<Entry> table(table_size);
    for (auto& e : table)
        e.key_offset = 0xFFFFFFFF;

    std::vector<char> pool;
    pool.reserve(1024);

    auto add_string = [&](const std::string& s) {
        uint32_t off = pool.size();
        pool.insert(pool.end(), s.begin(), s.end());
        pool.push_back('\0');
        return off;
    };

    for (auto& [k, v] : items) {
        uint32_t h = fnv1a(k);
        size_t i = h & (table_size - 1);

        uint32_t k_off = add_string(k);
        uint32_t v_off = add_string(v);

        while (table[i].key_offset != 0xFFFFFFFF)
            i = (i + 1) & (table_size - 1);

        table[i] = {k_off, v_off, h, 0};
    }

    Header hdr {
        kMagic,
        kVersion,
        static_cast<uint32_t>(items.size()),
        static_cast<uint32_t>(table_size),
        static_cast<uint32_t>(pool.size())
    };

    std::ofstream out(argv[2], std::ios::binary);
    out.write(reinterpret_cast<char*>(&hdr), sizeof(hdr));
    out.write(reinterpret_cast<char*>(table.data()), table.size() * sizeof(Entry));
    out.write(pool.data(), pool.size());

    std::cout << "Processed " << lineno-1 << " lines. " << items.size() << " items.\n";
}

