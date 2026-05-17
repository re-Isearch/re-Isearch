#include "bert.h"
#include "ggml.h"

#include <unistd.h>
#include <stdio.h>
#include <vector>

struct bert_params
{
    int32_t n_threads = 6;
    const char* model = "models/all-MiniLM-L6-v2/ggml-model-q4_0.bin";
    const char* prompt = "test prompt";
    int32_t batch_size = 32;
    bool use_cpu = false;
};

void bert_print_usage(char **argv, const bert_params &params) {
    fprintf(stderr, "usage: %s [options]\n", argv[0]);
    fprintf(stderr, "\n");
    fprintf(stderr, "options:\n");
    fprintf(stderr, "  -h, --help            show this help message and exit\n");
    fprintf(stderr, "  -s SEED, --seed SEED  RNG seed (default: -1)\n");
    fprintf(stderr, "  -t N, --threads N     number of threads to use during computation (default: %d)\n", params.n_threads);
    fprintf(stderr, "  -p PROMPT, --prompt PROMPT\n");
    fprintf(stderr, "                        prompt to start generation with (default: random)\n");
    fprintf(stderr, "  -m FNAME, --model FNAME\n");
    fprintf(stderr, "                        model path (default: %s)\n", params.model);
    fprintf(stderr, "  -b BATCH_SIZE, --batch-size BATCH_SIZE\n");
    fprintf(stderr, "                        batch size to use when executing model\n");
    fprintf(stderr, "  -c, --cpu             use CPU backend (default: use CUDA if available)\n");
    fprintf(stderr, "\n");
}

bool bert_params_parse(int argc, char **argv, bert_params &params) {
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];

        if (arg == "-t" || arg == "--threads") {
            params.n_threads = std::stoi(argv[++i]);
        } else if (arg == "-p" || arg == "--prompt") {
            params.prompt = argv[++i];
        } else if (arg == "-m" || arg == "--model") {
            params.model = argv[++i];
        } else if (arg == "-c" || arg == "--cpu") {
            params.use_cpu = true;
        } else if (arg == "-h" || arg == "--help") {
            bert_print_usage(argv, params);
            exit(0);
        } else {
            fprintf(stderr, "error: unknown argument: %s\n", arg.c_str());
            bert_print_usage(argv, params);
            exit(0);
        }
    }

    return true;
}

int main(int argc, char ** argv) {
    ggml_time_init();
    const int64_t t_main_start_us = ggml_time_us();

    bert_params params;
    if (bert_params_parse(argc, argv, params) == false) {
        return 1;
    }

    int64_t t_load_us = 0;

    bert_ctx * bctx;

    // load the model
    {
        const int64_t t_start_us = ggml_time_us();

        bctx = bert_load_from_file(params.model, params.use_cpu);
        if (bctx == nullptr) {
            fprintf(stderr, "%s: failed to load model from '%s'\n", __func__, params.model);
            return 1;
        }

        const int32_t n_max_tokens = bert_n_max_tokens(bctx);
        bert_allocate_buffers(bctx, n_max_tokens, params.batch_size);

        t_load_us = ggml_time_us() - t_start_us;
    }

    int64_t t_start_us = ggml_time_us();

    // tokenize the prompt
    int N = bert_n_max_tokens(bctx);
    bert_tokens tokens = bert_tokenize(bctx, params.prompt, N);

    int64_t t_mid_us = ggml_time_us();
    int64_t t_token_us = t_mid_us - t_start_us;

    // print the tokens
    for (auto & tok : tokens) {
        fprintf(stderr, "%d -> %s\n", tok, bert_vocab_id_to_token(bctx, tok));
    }
    fprintf(stderr, "\n");

    // create a batch
    const int n_embd = bert_n_embd(bctx);
    bert_batch batch = { tokens };

    // run the embedding
    std::vector<float> embed(batch.size()*n_embd);
    bert_forward_batch(bctx, batch, embed.data(), params.n_threads);

    int64_t t_end_us = ggml_time_us();
    int64_t t_eval_us = t_end_us - t_mid_us;
    
    printf("[ ");
    for (int i = 0; i < n_embd; i++) {
        const char * sep = (i == n_embd - 1) ? "" : ",";
        printf("%1.4f%s ",embed[i], sep);
    }
    printf("]\n");

    // report timing
    {
        const int64_t t_main_end_us = ggml_time_us();

        fprintf(stderr, "\n");
        fprintf(stderr, "%s:     load time = %8.2f ms\n", __func__, t_load_us/1000.0f);
        fprintf(stderr, "%s:    token time = %8.2f ms / %.2f ms per token\n", __func__, t_token_us/1000.0f, t_token_us/1000.0f/tokens.size());
        fprintf(stderr, "%s:     eval time = %8.2f ms / %.2f ms per token\n", __func__, t_eval_us/1000.0f, t_eval_us/1000.0f/tokens.size());
        fprintf(stderr, "%s:    total time = %8.2f ms\n", __func__, (t_main_end_us - t_main_start_us)/1000.0f);
    }

    bert_free(bctx);

    return 0;
}
