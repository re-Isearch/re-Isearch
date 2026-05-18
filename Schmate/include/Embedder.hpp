#pragma once
#include <string>
#include <vector>

class BaseEmbedder {
public:
    virtual ~BaseEmbedder() = default;
    virtual std::vector<float> encode_text(const std::string &text, bool debug=false) = 0;
    virtual size_t embedding_dim() const = 0;
    virtual size_t Matryoshka_dim() const = 0;

    // Returns the exact GGUF architecture string (e.g., "bert", "nomic-bert-moe")
    virtual std::string_view model_architecture() const = 0;
    
    // Returns the model name or family identifier if needed for finer rules (e.g., "bge-large")
    virtual std::string_view model_name() const = 0;
};


