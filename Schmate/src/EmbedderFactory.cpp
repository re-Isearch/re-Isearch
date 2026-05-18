bool is_bert_cpp_compatible(const std::string & fname) {
    struct gguf_init_params params = { .no_alloc = true, .ctx = NULL };
    struct gguf_context * ctx = gguf_init_from_file(fname.c_str(), params);
    
    if (!ctx) return false;

    // 1. Check Architecture
    std::string arch = gguf_get_val_str(ctx, gguf_find_key(ctx, "general.architecture"));
    
    // 2. Check for BERT-specific pooling (the hallmark of a bert.cpp model)
    int pooling_key = gguf_find_key(ctx, "bert.pooling_type");

    gguf_free(ctx);

    // If it's a 'bert' architecture and has defined pooling, bert.cpp will love it
    return (arch == "bert" && pooling_key != -1);
}
