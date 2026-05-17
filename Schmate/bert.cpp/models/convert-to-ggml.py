import sys
import json
import torch

from pathlib import Path
from gguf import GGUFWriter, GGMLQuantizationType
from transformers import AutoModel, AutoTokenizer

# primay usage
if len(sys.argv) < 2:
    print('Usage: convert-to-ggml.py model_dir [float-type=f16,f32]\n')
    sys.exit(1)

# output in the same directory as the model
model_dir = Path(sys.argv[1])
float_type = sys.argv[2].lower() if len(sys.argv) > 2 else 'f16'

# check model dir exists
if not model_dir.exists():
    print(f'Directory {model_dir} does not exist.')
    sys.exit(1)

# convert to ggml quantization type
if float_type not in ['f16', 'f32']:
    print(f'Float type must be f16 or f32, got: {float_type}')
    sys.exit(1)
else:
    qtype = GGMLQuantizationType[float_type.upper()]
    dtype0 = {'f16': torch.float16, 'f32': torch.float32}[float_type]

# get output file name
fname_out = model_dir / f'ggml-model-{float_type}.gguf'

# heck if the directory existsc
if not model_dir.exists():
    print(f'Directory {model_dir} does not exist.')

# load hf modle data
with open(model_dir / 'tokenizer.json', 'r', encoding='utf-8') as f:
    encoder = json.load(f)

with open(model_dir / 'config.json', 'r', encoding='utf-8') as f:
    hparams = json.load(f)

with open(model_dir / 'vocab.txt', 'r', encoding='utf-8') as f:
    vocab = f.read().splitlines()

# load tokenizer and model
tokenizer = AutoTokenizer.from_pretrained(model_dir)
model = AutoModel.from_pretrained(model_dir, low_cpu_mem_usage=True)

# print model
hparam_keys = [
    'vocab_size', 'max_position_embeddings', 'hidden_size', 'intermediate_size',
    'num_attention_heads', 'num_hidden_layers', 'layer_norm_eps'
]
print('PARAMS')
for k in hparam_keys:
    print(f'{k:<24s} = {hparams[k]}')
print()

# start to write GGUF file
gguf_writer = GGUFWriter(fname_out, 'bert')

# write metadata
gguf_writer.add_name('BERT')
gguf_writer.add_description('GGML BERT model')
gguf_writer.add_file_type(qtype)

# write kv flags
gguf_writer.add_uint32('vocab_size', hparams['vocab_size'])
gguf_writer.add_uint32('max_position_embedding', hparams['max_position_embeddings'])
gguf_writer.add_uint32('hidden_size', hparams['hidden_size'])
gguf_writer.add_uint32('intermediate_size', hparams['intermediate_size'])
gguf_writer.add_uint32('num_attention_heads', hparams['num_attention_heads'])
gguf_writer.add_uint32('num_hidden_layers', hparams['num_hidden_layers'])
gguf_writer.add_float32('layer_norm_eps', hparams['layer_norm_eps'])

# write vocab
gguf_writer.add_token_list(vocab)

# write tensors
print('TENSORS')
for name, data in model.state_dict().items():
    # get correct dtype
    if 'LayerNorm' in name or 'bias' in name:
        dtype = torch.float32
    else:
        dtype = dtype0

    # print info
    shape_str = str(list(data.shape))
    print(f'{name:64s} = {shape_str:16s} {data.dtype} → {dtype}')

    # do conversion
    data = data.to(dtype)

    # add to gguf output
    gguf_writer.add_tensor(name, data.numpy())

# execute and close writer
gguf_writer.write_header_to_file()
gguf_writer.write_kv_data_to_file()
gguf_writer.write_tensors_to_file()
gguf_writer.close()

# print success
print(f'GGML model written to {fname_out}')
