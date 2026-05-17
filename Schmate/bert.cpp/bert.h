#ifndef BERT_H
#define BERT_H

// Modern GGML includes (post-GGUF era, ggml-org/ggml / llama.cpp)
#include "ggml.h"
#include "ggml-alloc.h"      // ggml_gallocr_t
#include "ggml-backend.h"    // ggml_backend_t, ggml_backend_buffer_t
#include "ggml-cpu.h"        // ggml_backend_cpu_init, ggml_backend_cpu_set_n_threads
#include "gguf.h"            // gguf_context

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// ---------------------------------------------------------------------------
// C++ structs and type aliases — must be outside extern "C"
// ---------------------------------------------------------------------------
#ifdef __cplusplus
#include <string>
#include <vector>
#include <map>

typedef int32_t                  bert_token;
typedef std::vector<bert_token>  bert_tokens;
typedef std::vector<bert_tokens> bert_batch;
typedef std::string              bert_string;
typedef std::vector<bert_string> bert_strings;

struct bert_hparams {
    int32_t n_vocab        = 30522;
    int32_t n_max_tokens   = 512;
    int32_t n_embd         = 256;
    int32_t n_intermediate = 1536;
    int32_t n_head         = 12;
    int32_t n_layer        = 6;
    float   layer_norm_eps = 1e-12f;
};

struct bert_layer {
    struct ggml_tensor * ln_att_w;
    struct ggml_tensor * ln_att_b;
    struct ggml_tensor * ln_out_w;
    struct ggml_tensor * ln_out_b;

    struct ggml_tensor * q_w;
    struct ggml_tensor * q_b;
    struct ggml_tensor * k_w;
    struct ggml_tensor * k_b;
    struct ggml_tensor * v_w;
    struct ggml_tensor * v_b;
    struct ggml_tensor * o_w;
    struct ggml_tensor * o_b;

    struct ggml_tensor * ff_i_w;
    struct ggml_tensor * ff_i_b;
    struct ggml_tensor * ff_o_w;
    struct ggml_tensor * ff_o_b;
};

struct bert_vocab {
    std::vector<std::string> tokens;
    std::map<std::string, bert_token> token_to_id;
    std::map<std::string, bert_token> subword_token_to_id;
    std::map<bert_token, std::string> _id_to_token;
    std::map<bert_token, std::string> _id_to_subword_token;
};

struct bert_model {
    bert_hparams hparams;
    struct ggml_tensor * word_embeddings;
    struct ggml_tensor * token_type_embeddings;
    struct ggml_tensor * position_embeddings;
    struct ggml_tensor * ln_e_w;
    struct ggml_tensor * ln_e_b;
    std::vector<bert_layer> layers;
};

struct bert_ctx {
    bert_model model;
    bert_vocab vocab;

    struct ggml_context * ctx_data = nullptr;
    struct gguf_context * ctx_meta = nullptr;
    std::vector<uint8_t>  buf_compute_meta;

    ggml_backend_t        backend        = nullptr;
    ggml_backend_buffer_t weights_buffer = nullptr;
    ggml_backend_buffer_t compute_buffer = nullptr;
    ggml_gallocr_t        compute_alloc  = nullptr;
};

#endif // __cplusplus

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

#define BERT_API __attribute__((visibility("default")))
#define BERT_API_VERSION 2

#ifdef __cplusplus
extern "C" {
#endif

// -- lifecycle --
BERT_API struct bert_ctx * bert_load_from_file(const char * fname, bool use_cpu);
BERT_API void bert_allocate_buffers(struct bert_ctx * ctx, int32_t n_max_tokens, int32_t batch_size);
BERT_API void bert_deallocate_buffers(struct bert_ctx * ctx);
BERT_API void bert_free(struct bert_ctx * ctx);

// -- accessors --
BERT_API int32_t     bert_n_embd(struct bert_ctx * ctx);
BERT_API int32_t     bert_n_max_tokens(struct bert_ctx * ctx);
BERT_API const char* bert_vocab_id_to_token(struct bert_ctx * ctx, int32_t id);

// Used to get model and arch
BERT_API const char * bert_get_model_name(const bert_ctx * ctx);
BERT_API const char * bert_get_architecture(const bert_ctx * ctx);


// -- C-compatible tokenise / encode --
BERT_API uint64_t bert_tokenize_c(
    struct bert_ctx * ctx,
    const char *      text,
    int32_t *         output,
    uint64_t          n_max_tokens
);

BERT_API void bert_encode_batch_c(
    struct bert_ctx * ctx,
    const char **     texts,
    float *           embeddings,
    int32_t           n_input,
    int32_t           n_threads
);

#ifdef __cplusplus
} // extern "C"

// -- C++-only API (uses bert_tokens / bert_batch / bert_string / bert_strings) --

BERT_API struct ggml_cgraph * bert_build_graph(bert_ctx * ctx, bert_batch batch);

BERT_API void bert_forward_batch(bert_ctx * ctx, bert_batch batch,
                                 float * embeddings, int32_t n_threads);

BERT_API void bert_encode_batch(bert_ctx * ctx, bert_strings texts,
                                float * embeddings, int32_t n_threads);

BERT_API bert_tokens bert_tokenize(bert_ctx * ctx, bert_string text,
                                   uint64_t n_max_tokens);

BERT_API void bert_forward(bert_ctx * ctx, bert_tokens tokens,
                           float * embeddings, int32_t n_threads);

BERT_API void bert_encode(bert_ctx * ctx, bert_string text,
                          float * embeddings, int32_t n_threads);

#endif // __cplusplus
#endif // BERT_H
