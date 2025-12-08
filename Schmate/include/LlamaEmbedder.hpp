#pragma once
#include <string>
#include <vector>
#include <memory>
#include "llama.h"

class LlamaEmbedder {
public:
    explicit LlamaEmbedder(const std::string &model_path, int n_threads = 4)
        : threads(n_threads) {
        llama_backend_init(false);
        
        llama_model_params model_params = llama_model_default_params();
        model_params.use_mmap = true;
        model_params.use_mlock = false;

        model.reset(llama_load_model_from_file(model_path.c_str(), model_params));
        if (!model) throw std::runtime_error("Failed to load llama model: " + model_path);

        llama_context_params ctx_params = llama_context_default_params();
        ctx_params.n_threads = threads;
        ctx_params.seed = 0;
        ctx_params.embedding = true;

        ctx.reset(llama_new_context_with_model(model.get(), ctx_params));
        if (!ctx) throw std::runtime_error("Failed to create llama context");

        dim = llama_n_embd(model.get());
    }

    std::vector<float> encode_text(const std::string &text, bool debug = false) {
        std::vector<llama_token> tokens(text_to_tokens(text));
        if (tokens.empty()) throw std::runtime_error("Tokenization failed for text");

        if (llama_encode(ctx.get(), tokens.data(), tokens.size()) != 0)
            throw std::runtime_error("Llama embedding failed");

        const float *embd = llama_get_embeddings(ctx.get());
        std::vector<float> result(embd, embd + dim);

        if (debug) {
            float norm = 0.0f;
            for (float v : result) norm += v * v;
            printf("[DEBUG] embedding norm=%f\n", sqrt(norm));
        }

        return result;
    }

    size_t embedding_dim() const { return dim; }

private:
    std::vector<llama_token> text_to_tokens(const std::string &text) {
        std::vector<llama_token> tokens(text.size() + 10);
        int n = llama_tokenize(model.get(), text.c_str(), tokens.data(), tokens.size(), true);
        tokens.resize(n > 0 ? n : 0);
        return tokens;
    }

    struct ModelDeleter { void operator()(llama_model *p) const { llama_free_model(p); } };
    struct CtxDeleter   { void operator()(llama_context *p) const { llama_free(p); } };

    std::unique_ptr<llama_model, ModelDeleter> model;
    std::unique_ptr<llama_context, CtxDeleter> ctx;
    int threads;
    size_t dim;
};

