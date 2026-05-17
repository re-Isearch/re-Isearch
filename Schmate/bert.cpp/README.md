# bert.cpp

This is a *heavily refactored* version of [ggml](https://github.com/ggerganov/ggml) to support the newest ggml tensor library (0.11.1 May 2026). It has been designed for re-Isearch's Schmate project [schmate](https://nlnet.nl/project/Re-Isearch-Vector/).  It may be a drop-in replacement for the last bert.cpp.

Within this bert.cpp we support CUDA (NVIDIA), Metal (Apple hardware), Vulkan and fallback CPU. Support for other accelerators (as supported by the ggml tensor library) can be added as needed.

Because of the shift we no longer support GGML model files but just the newer GGUF format which replaced it. We have tried to support the various metadata names in GGUF files for BERT models but the implmentation does NOT support all GGUF, in particular EuroBERT, models. On failure we dump a copy of the metadata so one can decide to either Update the internal list of names or use llama.cpp instead. 

A simple test is:
   if (key == "general.architecture")
     architecture = value;
   ...
   if (key == "bert.pooling_type" || key == "pooling_type")
     pooling_type = value;
   ...
   bert_cpp_compatible = (architecture == "bert" || architecture == "nomic-bert");
   if (is_bert_cpp_compatible && pooling_type == -1) {
      // Might NOT be OK.
    }

Why bert.cpp instead of llama.cpp? Size and performance.

On an Apple M1:
model:   sentence-transformers_all-MiniLM-L12-v2
main:    token time =     0.03 ms / 0.01 ms per token
main:     eval time =    15.07 ms / 3.77 ms per token

model:   bge-large-en-v1.5-q4_k_m.gguf
main:    token time =     0.03 ms / 0.01 ms per token
main:     eval time =    30.91 ms / 7.73 ms per token


===

This is a [ggml](https://github.com/ggerganov/ggml) implementation of the BERT embedding architecture. It supports inference on both CPU and CUDA in floating point and a wide variety of quantization schemes. Includes Python bindings for batched inference.

This repo is a fork of original [bert.cpp](https://github.com/skeskinen/bert.cpp) as well as [embeddings.cpp](https://github.com/xyzhang626/embeddings.cpp). Thanks to both of you!

### Install

Fetch this respository then download submodules and install packages with
```sh
git submodule update --init --recursive
pip install -r requirements.txt
```

To fetch models from `huggingface`  and convert them to `gguf` format run the following
```sh
cd models
python download-repo.py BAAI/bge-base-en-v1.5 # or any other model
python convert-to-ggml.py BAAI/bge-base-en-v1.5 f16
python convert-to-ggml.py BAAI/bge-base-en-v1.5 f32
```

### Build

To build the dynamic library for usage from Python
```sh
cmake -B build .
make -C build -j
```

If you're compiling for GPU, you should run
```sh
cmake -DGGML_CUBLAS=ON -B build .
make -C build -j
```
On some distros, you also need to specifiy the host C++ compiler. To do this, I suggest setting the `CUDAHOSTCXX` environment variable to your C++ bindir.

And for Apple Metal, you should run
```sh
cmake -DGGML_METAL=ON -B build .
make -C build -j
```

### Excecute

All executables are placed in `build/bin`. To run inference on a given text, run
```sh
build/bin/main -m models/bge-base-en-v1.5/ggml-model-f16.gguf -p "Hello world"
```
To force CPU usage, add the flag `-c`.

### Python

You can also run everything through Python, which is particularly useful for batch inference. For instance,
```python
import bert
mod = bert.BertModel('models/bge-base-en-v1.5/ggml-model-f16.gguf')
emb = mod.embed(batch)
```
where `batch` is a list of strings and `emb` is a `numpy` array of embedding vectors.

### Quantize

You can quantize models with the command
```sh
build/bin/quantize models/bge-base-en-v1.5/ggml-model-f32.gguf models/bge-base-en-v1.5/ggml-model-q8_0.gguf q8_0
```
or whatever your desired quantization level is. Currently supported values are: `q8_0`, `q5_0`, `q5_1`, `q4_0`, and `q4_1`. You can then pass these model files directly to `main` as above.
