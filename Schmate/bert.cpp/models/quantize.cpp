/* NOT CONVERTED TO NEW GGML */


#include "ggml.h"
#include "bert.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <regex>
#include <set>

// quantize a model
bool bert_model_quantize(const std::string & fname_inp, const std::string & fname_out, ggml_type qtype) {
    static const std::set<ggml_type> valid_qtypes = {
        GGML_TYPE_Q4_0, GGML_TYPE_Q4_1, GGML_TYPE_Q5_0, GGML_TYPE_Q5_1, GGML_TYPE_Q8_0
    };

    // ensure supported quantization type
    if (valid_qtypes.count(qtype) == 0) {
        fprintf(stderr, "%s: invalid quantization type %d\n", __func__, qtype);
        return false;
    }

    // get quantization type name
    const char * qname = ggml_type_name(qtype);

    // load model on cpu but don't allocate compute buffers
    printf("%s: loading model from '%s'\n", __func__, fname_inp.c_str());
    bert_ctx * ctx = bert_load_from_file(fname_inp.c_str(), true);
    if (!ctx) {
        fprintf(stderr, "%s: failed to open '%s' for reading\n", __func__, fname_inp.c_str());
        return false;
    }

    // unpack data
    bert_model model = ctx->model;
    bert_vocab vocab = ctx->vocab;
    bert_hparams hparams = model.hparams;

    // set up ggml context
    size_t buffer_size = ggml_backend_buffer_get_size(ctx->weights_buffer);
    struct ggml_init_params ggml_params = {
        /*.mem_size   =*/ buffer_size,
        /*.mem_buffer =*/ NULL,
        /*.no_alloc   =*/ false,
    };
    struct ggml_context * ggml = ggml_init(ggml_params);

    // set up gguf output
    struct gguf_context * gguf = gguf_init_empty();

    // set general info
    gguf_set_val_str(gguf, "general.architecture", "bert");
    gguf_set_val_str(gguf, "general.name", "BERT");
    gguf_set_val_str(gguf, "general.description", "GGML BERT model");
    gguf_set_val_u32(gguf, "general.file_type", qtype);

    // set model params
    gguf_set_val_u32(gguf, "vocab_size", hparams.n_vocab);
    gguf_set_val_u32(gguf, "max_position_embedding", hparams.n_max_tokens);
    gguf_set_val_u32(gguf, "hidden_size", hparams.n_embd);
    gguf_set_val_u32(gguf, "intermediate_size", hparams.n_intermediate);
    gguf_set_val_u32(gguf, "num_attention_heads", hparams.n_head);
    gguf_set_val_u32(gguf, "num_hidden_layers", hparams.n_layer);
    gguf_set_val_f32(gguf, "layer_norm_eps", hparams.layer_norm_eps);

    // write vocab
    std::vector<const char*> tokens;
    for (const std::string & s : vocab.tokens) {
        tokens.push_back(const_cast<const char*>(s.c_str()));
    }
    gguf_set_arr_str(gguf, "tokenizer.ggml.tokens", tokens.data(), tokens.size());

    // output buffer for quants
    int tot_size_old = 0;
    int tot_size_new = 0;

    // loop over tensors
    struct ggml_tensor * tensor = ggml_get_first_tensor(ctx->ctx_data);
    while (tensor != NULL) {
        // get tensor info
        const char* name = ggml_get_name(tensor);
        const char* tname = ggml_type_name(tensor->type);
        const int64_t * ne = tensor->ne;
        const int64_t n_dims = ggml_n_dims(tensor);
        const int64_t n_elem = ggml_nelements(tensor);
        const int64_t n_cols = tensor->ne[0];

        // select desired weighs by name
        bool quantize = (
            std::regex_match(name, std::regex(".*weight")) &&
            !std::regex_match(name, std::regex(".*LayerNorm.*"))
        );

        // make buffer for quantization
        float* data = reinterpret_cast<float *>(tensor->data);
        size_t old_size = ggml_nbytes(tensor);
        size_t cur_size = 0;

        if (quantize) {
            struct ggml_tensor * cur = ggml_new_tensor(ggml, qtype, n_dims, ne);
            ggml_set_name(cur, name);

            // what is this?
            std::vector<int64_t> hist_cur(1 << 4, 0);

            switch (qtype) {
                case GGML_TYPE_Q4_0: { cur_size = ggml_quantize_q4_0(data, cur->data, n_elem, n_cols, hist_cur.data()); break; }
                case GGML_TYPE_Q4_1: { cur_size = ggml_quantize_q4_1(data, cur->data, n_elem, n_cols, hist_cur.data()); break; }
                case GGML_TYPE_Q5_0: { cur_size = ggml_quantize_q5_0(data, cur->data, n_elem, n_cols, hist_cur.data()); break; }
                case GGML_TYPE_Q5_1: { cur_size = ggml_quantize_q5_1(data, cur->data, n_elem, n_cols, hist_cur.data()); break; }
                case GGML_TYPE_Q8_0: { cur_size = ggml_quantize_q8_0(data, cur->data, n_elem, n_cols, hist_cur.data()); break; }
                default: {
                    fprintf(stderr, "%s: invalid quantization type %d\n", __func__, qtype);
                    return false;
                }
            }

            // print stats
            printf("[%5ld, %5ld] (%3s) -> (%4s) = %s\n", ne[0], ne[1], tname, qname, name);

            // add quantized tensor
            gguf_add_tensor(gguf, cur);
        } else {
            cur_size = old_size;
            gguf_add_tensor(gguf, tensor);
        }

        // increment total size
        tot_size_old += old_size;
        tot_size_new += cur_size;

        // increment to next tensor
        tensor = ggml_get_next_tensor(ctx->ctx_data, tensor);
    }

    // print stats
    printf("\n");
    printf("model size  = %8.2f MB\n", tot_size_old / 1024.0f / 1024.0f);
    printf("quant size  = %8.2f MB\n", tot_size_new / 1024.0f / 1024.0f);

    // write ggml file to disk
    gguf_write_to_file(gguf, fname_out.c_str(), false);

    // free memory
    gguf_free(gguf);
    ggml_free(ggml);
    bert_free(ctx);

    return true;
}

ggml_type ggml_type_from_str(const char * str) {
    for (int i = GGML_TYPE_F32; i < GGML_TYPE_COUNT; i++) {
        const ggml_type t = static_cast<ggml_type>(i);
        const char * n = ggml_type_name(t);
        if (strcmp(str, n) == 0) {
            return t;
        }
    }
    throw printf("ggml_type_from_str: invalid type '%s'\n", str);
}

// main entry point
int main(int argc, char ** argv) {
    if (argc != 4) {
        fprintf(stderr, "usage: quantize model-f32.bin model-quant.bin qtype\n");
        return 1;
    }

    const std::string fname_inp = argv[1];
    const std::string fname_out = argv[2];
    const ggml_type itype = ggml_type_from_str(argv[3]);

    const int64_t t_start_us = ggml_time_us();

    if (!bert_model_quantize(fname_inp, fname_out, itype)) {
        fprintf(stderr, "%s: failed to quantize model from '%s'\n", __func__, fname_inp.c_str());
        return 1;
    }

    const int64_t t_end_us = ggml_time_us();

    printf("quant time = %9.2f ms\n", (t_end_us - t_start_us) / 1000.0f);

    return 0;
}
