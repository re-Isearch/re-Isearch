/* NOT CONVERTED TO NEW GGML */

#include "ggml.h"
#include "ggml-backend.h"

#ifdef GGML_USE_CUDA
#include "ggml-cuda.h"
#endif

#include <string>
#include <vector>
#include <iostream>


#define ggml_allocr_  ggml_gallocr
#define ggml_backend_t ggml_backend_buffer_t

//
// definitions
//

#define BASIC_MAX_NODES 128

typedef std::vector<float> basic_input;
typedef std::vector<basic_input> basic_batch;

//
// data structures
//

struct basic_hparams {
    int32_t n_size = 256;
};

struct basic_model {
    basic_hparams hparams;
    struct ggml_tensor *weights;
};

struct basic_ctx {
    basic_model model;

    struct ggml_context * ctx_data;
    std::vector<uint8_t> buf_compute_meta;

    // memory buffers to evaluate the model
    ggml_backend_buffer_t params_buffer = NULL;
    ggml_backend_buffer_t compute_buffer = NULL;
    ggml_backend_t backend = NULL;
    ggml_allocr * compute_alloc = NULL;
};

//
// helper functions

static struct ggml_tensor * get_tensor(struct ggml_context * ctx, const std::string & name) {
    struct ggml_tensor * cur = ggml_get_tensor(ctx, name.c_str());
    if (!cur) {
        throw printf("%s: unable to find tensor %s\n", __func__, name.c_str());
    }

    return cur;
}

static void tensor_stats(ggml_tensor * t) {
    int32_t src0 = t->src[0] ? t->src[0]->backend : -1;
    int32_t src1 = t->src[1] ? t->src[1]->backend : -1;
    printf(
        "type = %s, dims = %d, shape = (%d, %d, %d, %d), backend = %d, src0 = %d, src1 = %d\n",
        ggml_type_name(t->type), ggml_n_dims(t), t->ne[0], t->ne[1], t->ne[2], t->ne[3], t->backend, src0, src1
    );
}

//
// model definition
//

ggml_cgraph * basic_build_graph(basic_ctx * ctx, basic_batch batch) {
    const basic_model & model = ctx->model;
    const basic_hparams & hparams = model.hparams;

    // extract model params
    const int n_size = hparams.n_size;
    const int n_batch_size = batch.size();

    // params for graph data
    struct ggml_init_params params = {
        /*.mem_size   =*/ ctx->buf_compute_meta.size(),
        /*.mem_buffer =*/ ctx->buf_compute_meta.data(),
        /*.no_alloc   =*/ true,
    };

    // initialze computational graph
    struct ggml_context * ctx_compute = ggml_init(params);
    struct ggml_cgraph * gf = ggml_new_graph_custom(ctx_compute, BASIC_MAX_NODES, false);

    // construct input tensors
    struct ggml_tensor *input = ggml_new_tensor_2d(ctx_compute, GGML_TYPE_F32, n_size, n_batch_size);
    ggml_set_name(input, "input");
    ggml_allocr_alloc(ctx->compute_alloc, input);

    // avoid writing input embeddings in memory measure mode
    if (!ggml_allocr_is_measure(ctx->compute_alloc)) {
        float * input_data = (float*)malloc(ggml_nbytes(input));
        for (int ba = 0; ba < n_batch_size; ba++) {
            for (int i = 0; i < n_size; i++) {
                input_data[ba * n_size + i] = batch[ba][i];
            }
        }
        ggml_backend_tensor_set(input, input_data, 0, ggml_nbytes(input));
        free(input_data);
    }

    // the only computation
    ggml_tensor * output = ggml_mul_mat(ctx_compute, input, model.weights); // [bs, ns] * [ns] -> [bs]

    // build the graph
    ggml_build_forward_expand(gf, output);

    // free context
    ggml_free(ctx_compute);

    // return complete graph
    return gf;
}

//
// loading and setup
//

struct basic_ctx * basic_create_model() {
    printf("%s: creating model\n", __func__);

    // create context
    basic_ctx * new_basic = new basic_ctx;
    basic_model & model = new_basic->model;
    basic_hparams & hparams = model.hparams;

    // get hparams
    const int32_t n_size = hparams.n_size;

    // initialize advanced backend
#ifdef GGML_USE_CUBLAS
    new_basic->backend = ggml_backend_cuda_init(0);
    if (!new_basic->backend) {
        printf("%s: ggml_backend_cuda_init() failed\n", __func__);
    } else {
        printf("%s: BERT using CUDA backend\n", __func__);
    }
#endif

    // fall back to CPU backend
    if (!new_basic->backend) {
        new_basic->backend = ggml_backend_cpu_init();
        printf("%s: BERT using CPU backend\n", __func__);
    }

    // load tensors
    {
        struct ggml_init_params params = {
            /*.mem_size =*/ 2 * ggml_tensor_overhead(),
            /*.mem_buffer =*/ NULL,
            /*.no_alloc =*/ true,
        };

        new_basic->ctx_data = ggml_init(params);
        if (!new_basic->ctx_data) {
            fprintf(stderr, "%s: ggml_init() failed\n", __func__);
            free(new_basic);
            return nullptr;
        }

        // add tensors to context
        const char * name = "weights";
        struct ggml_tensor * weights = ggml_new_tensor_1d(new_basic->ctx_data, GGML_TYPE_F32, n_size);
        ggml_set_name(weights, name);
        size_t weights_size = ggml_nbytes(weights);

        // alloc memory and offload data
        new_basic->params_buffer = ggml_backend_alloc_buffer(new_basic->backend, weights_size);
        ggml_allocr* alloc = ggml_allocr_new_from_buffer(new_basic->params_buffer);
        ggml_allocr_alloc(alloc, weights);

        // get local buffer
        float * data;
        std::vector<float> read_buf;
        if (ggml_backend_buffer_is_host(new_basic->params_buffer)) {
            data = (float*)weights->data;
        } else {
            read_buf.resize(weights_size);
            data = reinterpret_cast<float*>(read_buf.data());
        }

        // fill in on host
        for (int i = 0; i < n_size; i++) {
            data[i] = 1.0;
        }

        // copy to device
        if (!ggml_backend_buffer_is_host(new_basic->params_buffer)) {
            ggml_backend_tensor_set(weights, data, 0, weights_size);
        }

        // free memory
        ggml_allocr_free(alloc);
    }

    // use get_tensors to populate basic_model
    model.weights = get_tensor(new_basic->ctx_data, "weights");

    // allocate space for graph
    {
        // measure compute graph memory usage
        new_basic->buf_compute_meta.resize(GGML_DEFAULT_GRAPH_SIZE * ggml_tensor_overhead() + ggml_graph_overhead());
        new_basic->compute_alloc = ggml_allocr_new_measure_from_backend(new_basic->backend);

        // construct batch and compute graph
        basic_input input(hparams.n_size);
        basic_batch batch = {input, input};
        ggml_cgraph * gf = basic_build_graph(new_basic, batch);

        // get measurement results
        size_t compute_memory_buffer_size = ggml_allocr_alloc_graph(new_basic->compute_alloc, gf);
        ggml_allocr_free(new_basic->compute_alloc);

        // create real compute buffer and allocator
        new_basic->compute_buffer = ggml_backend_alloc_buffer(new_basic->backend, compute_memory_buffer_size);
        new_basic->compute_alloc = ggml_allocr_new_from_buffer(new_basic->compute_buffer);

        printf("%s: compute allocated memory: %.2f MB\n", __func__, compute_memory_buffer_size / 1024.0 / 1024.0);
    }

    return new_basic;
}

void basic_free(basic_ctx * ctx) {
    ggml_free(ctx->ctx_data);
    delete ctx;
}

//
// model execution
//

void basic_forward_batch(basic_ctx * ctx, basic_batch batch, float * output) {
    // reset alloc buffer to clean the memory from previous invocations
    ggml_allocr_reset(ctx->compute_alloc);

    // build the inference graph
    ggml_cgraph * gf = basic_build_graph(ctx, batch);
    ggml_allocr_alloc_graph(ctx->compute_alloc, gf);

    // compute the graph
    ggml_backend_graph_compute(ctx->backend, gf);

    // print graph info
    printf("%s: compute done\n", __func__);
    ggml_graph_print(gf);

    // the last node is the embedding tensor
    struct ggml_tensor * final = gf->nodes[gf->n_nodes - 1];
    printf(
        "%s: type = %s, ndim = %d, nelem = %d, nrows = %d\n",
        __func__, ggml_type_name(final->type), ggml_n_dims(final), ggml_nelements(final), ggml_nrows(final)
    );

    // copy the embeddings to the location passed by the user
    ggml_backend_tensor_get(final, output, 0, ggml_nbytes(final));
}

float basic_forward_one(struct basic_ctx * ctx, basic_input input) {
    basic_batch batch = {input};
    float output;
    basic_forward_batch(ctx, batch, &output);
    return output;
}

int main(int argc, char ** argv) {
    basic_input input(256);
    for (int i = 0; i < 256; i++) {
        input[i] = (float)i;
    }
    basic_batch batch = {input, input};
    float output[2];

    basic_ctx * ctx = basic_create_model();
    basic_forward_batch(ctx, batch, output);

    printf("output = %f %f\n", output[0], output[1]);
    return 0;
}
