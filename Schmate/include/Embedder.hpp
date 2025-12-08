#pragma once
#include <string>
#include <vector>

class BaseEmbedder {
public:
    virtual ~BaseEmbedder() = default;
    virtual std::vector<float> encode_text(const std::string &text, bool debug=false) = 0;
    virtual size_t embedding_dim() const = 0;
};


