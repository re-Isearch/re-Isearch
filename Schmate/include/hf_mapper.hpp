#pragma once
#include <iostream>
#include <fstream>      // std::ifstream
#include <string_view>
#include <cstdint>
#include <optional>
#include <utility>

#include "hf_mapper_format.hpp"


class HFModelMapper {
friend class HFModelMap;
public:

    static std::optional<HFModelMapper>
    load(const void* data, size_t size);

    void dump() const;

    std::string_view lookup(std::string_view key) const;

    // Returns {repo, filename}
    // bert-base=bert-base-uncased|tokenizer.gmml
    // NOTE: We don't ever want to see | (pipe) in repo or filename. While Unix can
    // allow '|' it is disallowed in a number of other filesystems including Windows. Because
    // of this it is considered very bad design to use '|' in a URL.  It gets generally
    // encoded as %7C
    // This simplifies things since we don't need to worry about escaping any characters (as
    // escaping | would demand we also support escaping \ and probably also =!
    std::optional<std::pair<std::string_view, std::string_view>> getModel(std::string_view key) const {
        auto model = lookup(key);
        if (!model.empty()) {
            const size_t pos = model.find('|');
            if (pos != std::string_view::npos) {
                return std::make_optional(
                    std::pair{ std::string_view{model.substr(0, pos)},
                    std::string_view{model.substr(pos + 1)} });
            }
        }
        return std::nullopt;
    }

private:
    const hfmap::Header* header;
    const hfmap::Entry*  table;
    const char*   pool;
    size_t        mask;

};


class HFModelMap {
private:
    std::optional<HFModelMapper> mapper;
    void *data            = NULL;
    size_t size           = 0;
public:
    // Filenames are 255 bytes on most modern filesystems (ext4, NTFS, FAT32, NFS, CIFS)
    // but in practice, HuggingFace model filenames are typically quite short
   inline static constexpr int MAX_MODEL_ID_LENGTH  = 64;

    HFModelMap(const std::string& filepath) {
      open(filepath);
    }
    bool open(const std::string& filepath);

    std::string_view lookup(std::string_view key) const {
       if (mapper) return mapper->lookup(key);
       return key;
    }

    bool Ok() const { return mapper ? true : false; }

    std::optional<std::pair<std::string_view, std::string_view>> getModel(std::string_view key) const {
       if (mapper) return mapper->getModel(key);
       return std::nullopt;
    }

   // decompile to standard out stream
    void dump() const {
      if (mapper) mapper->dump();

    }

    // Compile from source (text) to dest (binary)
    static bool compile(const std::string& source, const std::string& dest);
        
    ~HFModelMap();
};


std::optional<HFModelMap> ModelMap();


