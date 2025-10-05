#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>

// Forward declaration of bert C API
extern "C" {
#include "bert.h"
}

struct SBertGGML {
    bert_ctx * ctx = nullptr;
    int n_embd = 0;

    SBertGGML(const std::string & model_path) {
        ctx = bert_load_from_file(model_path.c_str());
        if (!ctx) throw std::runtime_error("Failed to load model " + model_path);
        n_embd = bert_n_embd(ctx);
        std::cerr << "Loaded SBERT GGML model. dim=" << n_embd << std::endl;
    }

    ~SBertGGML() {
        if (ctx) bert_free(ctx);
    }

    std::vector<float> encode_text(const std::string & text, bool debug=false) {
        std::vector<bert_vocab_id> tokens(512);
        int32_t n_tokens = 512;
        bert_tokenize(ctx, text.c_str(), tokens.data(), &n_tokens, tokens.size());

        std::vector<float> emb(n_embd);
        bert_eval(ctx, 4, tokens.data(), n_tokens, emb.data());

        if (debug) {
            float norm=0;
            for (float v : emb) norm += v*v;
            std::cerr << "[DEBUG] embedding norm=" << sqrt(norm) << std::endl;
        }
        return emb;
    }
};


