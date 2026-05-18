#pragma once
#include <string>
#include <vector>
#include <memory>
#include "llama.h"


// TODO: Support for Matryoshka embeddings 
// Need to 1) define a new distance metric for HNSW
//         2) figure out how to read the dimension from llama.cpp
// 


class LlamaEmbedder : public BaseEmbedder {
public:
    explicit LlamaEmbedder(const std::string &model_path, int n_threads = 4)
        : threads(n_threads) {

        // Note: llama_backend_init is global; calling it multiple times is okay,
        // but llama_backend_free should ideally be called at program exit.
        llama_backend_init(false);
        
        llama_model_params model_params = llama_model_default_params();
        model_params.use_mmap = true;
        model_params.use_mlock = false;

        model.reset(llama_load_model_from_file(model_path.c_str(), model_params));
        if (!model) throw std::runtime_error("Failed to load llama model: " + model_path);

        llama_context_params ctx_params = llama_context_default_params();
        ctx_params.n_threads = threads;
        ctx_params.seed = 0;
        ctx_params.embedding = true; // Required for encoder/embedding models

        ctx.reset(llama_new_context_with_model(model.get(), ctx_params));
        if (!ctx) throw std::runtime_error("Failed to create llama context");

	// --- Extract GGUF Metadata directly using llama.cpp API ---
        char buffer[256];
        
        // 1. Get Architecture (e.g., "bert", "nomic-bert-moe")
        if (llama_model_meta_val_str(model, "general.architecture", buffer, sizeof(buffer)) > 0) {
            arch = buffer;
	} else {
            arch = "unknown";
	}
	// 2. Get Model Family Name
	if (llama_model_meta_val_str(model, "general.name", buffer, sizeof(buffer)) > 0) {
            name = buffer;
	} else {
            name = model_path; // Fallback to filename if property missing
	}
        dim = llama_n_embd(model.get());
    }

    ~LlamaEmbedder() override {
	if (ctx) llama_free(ctx);
	if (model) llama_free_model(model);
    }

    std::string_view model_architecture() const override { return arch; } 
    std::string_view model_name() const override { return name; }

    std::vector<float> encode_text(const std::string &text, bool debug = false) override {
#if 1
    // 1. Tokenize
    std::vector<llama_token> tokens(text.size() * 2 + 16);
    int n_tokens = llama_tokenize(model.get(), text.c_str(), (int)text.length(), 
                                  tokens.data(), (int)tokens.size(), true, true);
    if (n_tokens < 0) {
        tokens.resize(-n_tokens);
        n_tokens = llama_tokenize(model.get(), text.c_str(), (int)text.length(), 
                                  tokens.data(), (int)tokens.size(), true, true);
    }
    tokens.resize(n_tokens);

    if (tokens.empty()) return {};

    // 2. Batching: We send the whole string as one sequence (ID 0)
    llama_batch batch = llama_batch_get_one(tokens.data(), (int)tokens.size(), 0, 0);

    // 3. Decode 
    if (llama_decode(ctx.get(), batch) != 0) {
        throw std::runtime_error("llama_decode failed");
    }

    // 4. Retrieve Embeddings
    // Note: llama_get_embeddings_seq is the modern way to get the pooled output for sequence 0
    const float* embd = llama_get_embeddings_seq(ctx.get(), 0);
    
    // Fallback for older versions of the new API
    if (embd == nullptr) {
        embd = llama_get_embeddings(ctx.get());
    }

    if (embd == nullptr) {
        throw std::runtime_error("Model did not return embeddings (check if pooling is enabled)");
    }

    return std::vector<float>(embd, embd + dim);
#elif  1 /* Code for the latest version of Llama.cpp */
        auto tokens = text_to_tokens(text);
        if (tokens.empty()) throw std::runtime_error("No tokens produced");

        // 1. Create a batch for a single sequence
        // Arguments: (tokens_ptr, n_tokens, sequence_id, last_token_pos)
        llama_batch batch = llama_batch_get_one(tokens.data(), (int)tokens.size(), 0, 0);

        // 2. Compute the embeddings
        if (llama_decode(ctx.get(), batch) != 0) {
            throw std::runtime_error("llama_decode failed");
        }

        // 3. Extract the result
        // For embeddings, we usually want the pooled result or the last token's output
        const float *embd = llama_get_embeddings(ctx.get());
        if (!embd) {
            // Some newer models require getting embeddings by sequence ID
            embd = llama_get_embeddings_seq(ctx.get(), 0);
        }

        if (!embd) throw std::runtime_error("Failed to retrieve embeddings from llama context");

        return std::vector<float>(embd, embd + dim);
#else
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
#endif
    }

    size_t embedding_dim() const override { return dim; }
    size_t Matryoshka_dim() const override { return 0; } // For NOW!

private:

    std::vector<llama_token> text_to_tokens(const std::string &text) {
#if 1 /* NEW CODE (for latest Llama.cpp */
        // Over-allocate slightly for safety
        std::vector<llama_token> tokens(text.size() * 2 + 8);
    
        // Modern signature: note the extra 'false' at the end for "parse special"
        int n_tokens = llama_tokenize(model.get(), text.c_str(), (int)text.length(), 
                                  tokens.data(), (int)tokens.size(), true, true);
    
        if (n_tokens < 0) {
            // If buffer was too small, resize and try once more
            tokens.resize(-n_tokens);
            n_tokens = llama_tokenize(model.get(), text.c_str(), (int)text.length(), 
                                  tokens.data(), (int)tokens.size(), true, true);
        }
        tokens.resize(std::max(0, n_tokens));
#else
        std::vector<llama_token> tokens(text.size()*2 + 5);
        int n = llama_tokenize(model.get(), text.c_str(), tokens.data(), tokens.size(), true);
        tokens.resize(n > 0 ? n : 0);
#endif
        return tokens;
    }

    struct ModelDeleter { void operator()(llama_model *p) const { llama_free_model(p); } };
    struct CtxDeleter   { void operator()(llama_context *p) const { llama_free(p); } };

    std::unique_ptr<llama_model, ModelDeleter> model;
    std::unique_ptr<llama_context, CtxDeleter> ctx;
    int threads;
    size_t dim;
    std::string arch;
    std::string name;
};

