import os
import sys
import ctypes
import numpy as np
from tqdm import tqdm

LIB_DIR = os.path.dirname(__file__)
LIB_PATH = os.path.join(LIB_DIR, 'build/libbert.so')

# Avoid "LookupError: unknown encoding: ascii" when open() called in a destructor
outnull_file = open(os.devnull, "w")
errnull_file = open(os.devnull, "w")

class suppress_stdout_stderr():
    # NOTE: these must be "saved" here to avoid exceptions when using
    #       this context manager inside of a __del__ method
    sys = sys
    os = os

    def __init__(self, disable=True):
        self.disable = disable

    # Oddly enough this works better than the contextlib version
    def __enter__(self):
        if self.disable:
            return self

        # Check if sys.stdout and sys.stderr have fileno method
        if not hasattr(self.sys.stdout, 'fileno') or not hasattr(self.sys.stderr, 'fileno'):
            return self  # Return the instance without making changes

        self.old_stdout_fileno_undup = self.sys.stdout.fileno()
        self.old_stderr_fileno_undup = self.sys.stderr.fileno()

        self.old_stdout_fileno = self.os.dup(self.old_stdout_fileno_undup)
        self.old_stderr_fileno = self.os.dup(self.old_stderr_fileno_undup)

        self.old_stdout = self.sys.stdout
        self.old_stderr = self.sys.stderr

        self.os.dup2(outnull_file.fileno(), self.old_stdout_fileno_undup)
        self.os.dup2(errnull_file.fileno(), self.old_stderr_fileno_undup)

        self.sys.stdout = outnull_file
        self.sys.stderr = errnull_file
        return self

    def __exit__(self, *_):
        if self.disable:
            return

        # Check if sys.stdout and sys.stderr have fileno method
        if hasattr(self.sys.stdout, 'fileno') and hasattr(self.sys.stderr, 'fileno'):
            self.sys.stdout = self.old_stdout
            self.sys.stderr = self.old_stderr

            self.os.dup2(self.old_stdout_fileno, self.old_stdout_fileno_undup)
            self.os.dup2(self.old_stderr_fileno, self.old_stderr_fileno_undup)

            self.os.close(self.old_stdout_fileno)
            self.os.close(self.old_stderr_fileno)

def increment_pointer(p, d):
    t = type(p)._type_
    v = ctypes.cast(p, ctypes.c_void_p)
    v.value += d * ctypes.sizeof(t)
    return ctypes.cast(v, ctypes.POINTER(t))

class BertModel:
    def __init__(self, fname, batch_size=32, use_cpu=False, verbose=False):
        # set up ctypes for library
        self.lib = ctypes.cdll.LoadLibrary(LIB_PATH)

        self.lib.bert_load_from_file.restype = ctypes.c_void_p
        self.lib.bert_load_from_file.argtypes = [
            ctypes.c_char_p, # const char * fname
            ctypes.c_bool,   # bool use_cpu
        ]

        self.lib.bert_allocate_buffers.restype = ctypes.c_void_p
        self.lib.bert_allocate_buffers.argtypes = [
            ctypes.c_void_p, # bert_ctx * ctx
            ctypes.c_int32,  # int32_t n_max_tokens
            ctypes.c_int32,  # int32_t batch_size
        ]

        self.lib.bert_n_embd.restype = ctypes.c_int32
        self.lib.bert_n_embd.argtypes = [ctypes.c_void_p]

        self.lib.bert_n_max_tokens.restype = ctypes.c_int32
        self.lib.bert_n_max_tokens.argtypes = [ctypes.c_void_p]

        self.lib.bert_free.argtypes = [ctypes.c_void_p]

        self.lib.bert_tokenize_c.restype = ctypes.c_uint64
        self.lib.bert_tokenize_c.argtypes = [
            ctypes.c_void_p,                 # struct bert_ctx * ctx
            ctypes.c_char_p,                 # const char * text
            ctypes.POINTER(ctypes.c_int32),  # int32_t * output
            ctypes.c_uint64,                 # uint64_t n_max_tokens
        ]

        self.lib.bert_encode_batch_c.argtypes = [
            ctypes.c_void_p,                 # struct bert_ctx * ctx
            ctypes.POINTER(ctypes.c_char_p), # const char ** texts
            ctypes.POINTER(ctypes.c_float),  # float * embeddings
            ctypes.c_int32,                  # int32_t n_inputs
            ctypes.c_int32,                  # int32_t n_threads
        ]

        # load model from file
        with suppress_stdout_stderr(disable=verbose):
            self.ctx = self.lib.bert_load_from_file(fname.encode('utf-8'), use_cpu)
        if not self.ctx:
            raise ValueError(f'Failed to load model from file: {fname}')

        # get model dimensions
        self.n_embd = self.lib.bert_n_embd(self.ctx)
        self.n_max_tokens = self.lib.bert_n_max_tokens(self.ctx)
        self.batch_size = batch_size

        # allocate compute buffers
        with suppress_stdout_stderr(disable=verbose):
            self.lib.bert_allocate_buffers(self.ctx, self.n_max_tokens, self.batch_size)

    def __del__(self):
        self.lib.bert_free(self.ctx)

    def tokenize(self, text, n_max_tokens=None):
        if n_max_tokens is None:
            n_max_tokens = self.lib.bert_n_max_tokens(self.ctx)
        tokens = np.zeros(n_max_tokens, dtype=np.int32)
        tokens_p = tokens.ctypes.data_as(ctypes.POINTER(ctypes.c_int32))
        n_tokens = self.lib.bert_tokenize_c(self.ctx, text.encode('utf-8'), tokens_p, n_max_tokens)
        return tokens[:n_tokens]

    def embed_batch(self, batch, embed_p=None, n_threads=8):
        # create embedding memory
        n_input = len(batch)
        if embed_p is None:
            embed = np.zeros((n_input, self.n_embd), dtype=np.float32)
            embed_p = embed.ctypes.data_as(ctypes.POINTER(ctypes.c_float))
        else:
            embed = None

        # create batch strings
        strings = (ctypes.c_char_p * n_input)()
        for j, s in enumerate(batch):
            strings[j] = s.encode('utf-8')

        # call bert.cpp function
        self.lib.bert_encode_batch_c(self.ctx, strings, embed_p, n_input, n_threads)

        # return if it wasn't inplace
        if embed is not None:
            return embed

    def embed(self, text, progress=False):
        # handle singleton case
        if isinstance(text, str):
            text = [text]
            squeeze = True
        else:
            squeeze = False
        n_input = len(text)

        # create embedding memory
        embed = np.zeros((n_input, self.n_embd), dtype=np.float32)
        embed_p = embed.ctypes.data_as(ctypes.POINTER(ctypes.c_float))

        # loop over batches
        indices = range(0, n_input, self.batch_size)
        if progress:
            indices = tqdm(list(indices))
        for i in indices:
            j = min(i + self.batch_size, n_input)
            batch = text[i:j]
            batch_p = increment_pointer(embed_p, i * self.n_embd)
            self.embed_batch(batch, embed_p=batch_p)

        # return squeezed maybe
        return embed[0] if squeeze else embed
