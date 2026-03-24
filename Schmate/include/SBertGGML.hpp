#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
#include "Util.hpp"
#include "Logger.hpp"

// Forward declaration of bert C API
extern "C" {
#include "bert.h"
}

struct SBertGGML {
    bert_ctx * ctx = nullptr;
    int n_embd = 0;
    std::string model_name;

    SBertGGML(const std::string & model_path) {
#ifdef __APPLE__
       relax_macos_malloc_zones();
#endif
        ctx = bert_load_from_file(model_path.c_str());
        if (!ctx) throw std::runtime_error("Failed to load model " + model_path);
        model_name = basename(model_path); // Get the name
        n_embd = bert_n_embd(ctx);
        LOG_INFO_S() << "Loaded SBERT GGML model. dim=" << n_embd;
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

    int dim() const { return n_embd; }

    ~SBertGGML() {
        if (ctx) bert_free(ctx);
    }

    std::vector<float> encode_text(const std::string & text, bool debug=false) {
        const int MAX_TOKENS = 512;
        std::vector<bert_vocab_id> tokens(MAX_TOKENS);
        int32_t n_tokens = 0; // was 512;
	// std::cerr << "Encoding: \"" << text << "\"\n";
        bert_tokenize(ctx, text.c_str(), tokens.data(), &n_tokens, tokens.size());

        std::vector<float> emb(n_embd);
#if 1
        eval(tokens.data(), n_tokens, emb.data());
#else
        // 4 = number of threads (can make configurable)
        bert_eval(ctx, 4, tokens.data(), n_tokens, emb.data());
#endif
        return emb;
    }

   void encode( const char * texts, float * embeddings, int batch_size = 1);
   void eval (bert_vocab_id * tokens, int32_t n_tokens, float * embeddings);

   void reset_context() {
     bert_free(ctx);
   }

private:
};


