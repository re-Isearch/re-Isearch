#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
#include "Util.hpp"
#include "Logger.hpp"
#include "Embedder.hpp"

// Forward declaration of bert C API
extern "C" {
#include "bert.h"
}

struct SBertGGML : public BaseEmbedder {
    bert_ctx * ctx = nullptr;
    int n_embd = 0;
    std::string model_name;

    SBertGGML(const std::string & model_path, size_t threads = 4) : _threads(threads) {
#ifdef __APPLE__
       relax_macos_malloc_zones();
#endif
	clock_t start = clock();
        ctx = bert_load_from_file(model_path.c_str());
        if (!ctx) throw std::runtime_error("Failed to load model " + model_path);
        model_name = basename(model_path); // Get the name
        n_embd = bert_n_embd(ctx);
	clock_t end = clock();
	const double factor = 1000.0/CLOCKS_PER_SEC;
        const double cpu_total = end > start ? (end - start)*factor : 0.0;

        LOG_INFO_S() << "Loaded SBERT GGML model. dim=" << n_embd << " (" << cpu_total << "ms)" ;
#if 0

// The bert_ctx structure contains the model
// Access the tensors through ctx->model
// For example, check the embedding weights:
if (ctx && ctx->model.word_embeddings) {
    enum ggml_type type = ctx->model.word_embeddings->type;
    printf("Word embeddings type: %d\n", type);
    
    if (type == GGML_TYPE_Q4_0 || type == GGML_TYPE_Q4_1) {
        printf("Model uses 4-bit quantization\n");
    }
}
#endif

    }

    size_t embedding_dim() const override { return (size_t)n_embd; }

    ~SBertGGML() {
        if (ctx) bert_free(ctx);
    }

    std::vector<float> encode_text(const std::string & text, bool debug=false) override {
        const int MAX_TOKENS = 512;
        std::vector<bert_vocab_id> tokens(MAX_TOKENS);
        int32_t n_tokens = 0; // was 512;
	// std::cerr << "Encoding: \"" << text << "\"\n";
        bert_tokenize(ctx, text.c_str(), tokens.data(), &n_tokens, (int)tokens.size());

	// new: check error
        if (n_tokens <= 0) return std::vector<float>(n_embd, 0.0f);

        std::vector<float> emb(n_embd);
        this->eval(tokens.data(), n_tokens, emb.data());
        return emb;
    }

   void encode(const char ** texts, float ** embeddings, int n_inputs, int batch_size = 1);
   void encode( const char * texts, float * embeddings, int batch_size = 1);
   void eval (bert_vocab_id * tokens, int32_t n_tokens, float * embeddings);

   void reset_context() {
     bert_free(ctx);
   }

private:
   size_t _threads;
};


