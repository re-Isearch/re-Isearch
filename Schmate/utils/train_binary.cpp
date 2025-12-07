// Utility: Train Better Binary Quantization for BERT Index
// Usage: ./train_better_binary <corpus_file> <output_index> [num_training_samples]

#include "unified_hnsw.hpp"
#include "bert_index.hpp" // Your actual BERT encoder
#include <iostream>
#include <fstream>
#include <chrono>

// Simple helper to load texts from file (one per line)
std::vector<std::string> load_texts(const std::string& filename, size_t max_lines = 0) {
    std::vector<std::string> texts;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            texts.push_back(line);
            if (max_lines > 0 && texts.size() >= max_lines) {
                break;
            }
        }
    }
    
    return texts;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <corpus_file> <output_index> [num_training_samples]\n";
        std::cerr << "Example: " << argv[0] << " corpus.txt bert_index.bin 1000\n";
        return 1;
    }
    
    std::string corpus_file = argv[1];
    std::string output_index = argv[2];
    size_t num_training_samples = 1000;

    SBertGGML emb(model);
    HnswConfig cfg;
    const std::string name = "default";
    
    if (argc > 3) {
        num_training_samples = std::stoull(argv[3]);
    }
    
    std::cout << "Better Binary Quantization Training\n";
    std::cout << "====================================\n\n";
    
    // Step 1: Initialize BERT encoder
    std::cout << "[1/6] Initializing BERT encoder...\n";
    BertIndex bert_encoder(emb, cfg, name);

    
    // Get embedding dimension from a test encoding
    auto test_emb = bert_encoder.encode_text("test");
    size_t dim = test_emb.size();
    std::cout << "  sBert dimentions: " << emb.n_embd << "\n";
    std::cout << "  Embedding dimension: " << dim << "\n\n";
    
    // Step 2: Load corpus
    std::cout << "[2/6] Loading corpus from " << corpus_file << "...\n";
    auto all_texts = load_texts(corpus_file);
    std::cout << "  Loaded " << all_texts.size() << " texts\n\n";
    
    if (all_texts.empty()) {
        std::cerr << "Error: No texts loaded from corpus file\n";
        return 1;
    }
    
    // Step 3: Prepare training samples
    std::cout << "[3/6] Preparing training samples...\n";
    size_t fit_size = std::min(num_training_samples, all_texts.size());
    std::vector<std::vector<float>> training_embeddings;
    training_embeddings.reserve(fit_size);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < fit_size; i++) {
        auto emb = bert_encoder.encode_text(all_texts[i]);
        training_embeddings.push_back(std::move(emb));
        
        if ((i + 1) % 100 == 0) {
            std::cout << "  Encoded " << (i + 1) << "/" << fit_size << " samples\r";
            std::cout.flush();
        }
    }
    
    auto encode_time = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::high_resolution_clock::now() - start).count();
    
    std::cout << "  Encoded " << fit_size << " samples in " << encode_time << "s\n\n";
    
    // Step 4: Create and fit index
    std::cout << "[4/6] Creating index and fitting quantizer...\n";
    std::cout << "  Mode: BETTER (correlation-optimized thresholds)\n";
    std::cout << "  Metric: Cosine similarity\n";
    std::cout << "  Rescoring: Enabled\n\n";
    
    hnswlib::UnifiedIndex index(
        dim,
        all_texts.size(),
        hnswlib::Metric::Cosine,
        hnswlib::QuantizationType::BINARY,
        hnswlib::BinMode::BETTER,
        true,  // enable rescoring
        16,    // M
        200    // ef_construction
    );
    
    start = std::chrono::high_resolution_clock::now();
    index.fit(training_embeddings);
    auto fit_time = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::high_resolution_clock::now() - start).count();
    
    std::cout << "  Quantizer fitted in " << fit_time << "s\n";
    std::cout << "  (Computed optimal threshold for each of " << dim << " dimensions)\n\n";
    
    // Step 5: Index all documents
    std::cout << "[5/6] Indexing all documents...\n";
    start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < all_texts.size(); i++) {
        auto emb = bert_encoder.encode_text(all_texts[i]);
        index.addPoint(emb.data(), (hnswlib::labeltype)i);
        
        if ((i + 1) % 100 == 0) {
            std::cout << "  Indexed " << (i + 1) << "/" << all_texts.size() << " documents\r";
            std::cout.flush();
        }
    }
    
    auto index_time = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::high_resolution_clock::now() - start).count();
    
    std::cout << "  Indexed " << all_texts.size() << " documents in " << index_time << "s\n";
    std::cout << "  Rate: " << (all_texts.size() / (float)index_time) << " docs/sec\n\n";
    
    // Step 6: Save index
    std::cout << "[6/6] Saving index to " << output_index << "...\n";
    index.saveIndex(output_index);
    
    // Calculate file size
    std::ifstream file(output_index, std::ios::binary | std::ios::ate);
    size_t file_size = file.tellg();
    file.close();
    
    std::cout << "  Index saved (" << (file_size / 1024.0 / 1024.0) << " MB)\n\n";
    
    // Summary
    std::cout << "Training Complete!\n";
    std::cout << "==================\n";
    std::cout << "Training samples:   " << fit_size << "\n";
    std::cout << "Total documents:    " << all_texts.size() << "\n";
    std::cout << "Embedding dim:      " << dim << "\n";
    std::cout << "Quantization:       Better Binary (correlation-optimized)\n";
    std::cout << "Rescoring:          Enabled\n";
    std::cout << "Index file:         " << output_index << "\n";
    std::cout << "File size:          " << (file_size / 1024.0 / 1024.0) << " MB\n\n";
    
    // Memory savings
    size_t float_size = all_texts.size() * dim * sizeof(float);
    size_t binary_size = all_texts.size() * ((dim + 7) / 8);
    std::cout << "Memory comparison:\n";
    std::cout << "  Float vectors:    " << (float_size / 1024.0 / 1024.0) << " MB\n";
    std::cout << "  Binary vectors:   " << (binary_size / 1024.0 / 1024.0) << " MB\n";
    std::cout << "  Compression:      " << (float_size / (float)binary_size) << "x\n\n";
    
    std::cout << "To use this index:\n";
    std::cout << "  1. hnswlib::UnifiedIndex index(dim, max_elements, ...);\n";
    std::cout << "  2. index.loadIndex(\"" << output_index << "\");\n";
    std::cout << "  3. auto emb = bert_encoder.encode_text(query);\n";
    std::cout << "  4. auto results = index.searchKnn(emb.data(), k, true);\n";
    
    return 0;
}
