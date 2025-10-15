#include "SBertGGML.hpp"
#include "HnswConfig.hpp"
#include "BertIndex.hpp"
#include "ShardedIndex.hpp"

#include <iostream>
#include <cassert>

int main() {
    HnswConfig cfg;
    cfg.max_elements = 100;
    cfg.max_tokens_per_chunk = 5;  // force small chunks
    cfg.overlap_percent = 0.2f;    // 20% overlap
    cfg.debug = true;

    SBertGGML embedder("../lib/sbert.ggml");  // replace with your model path
    ShardedIndex sindex(embedder, cfg, "test_index");

    std::vector<std::string> sentences = {
        "Artificial intelligence is transforming the world",
        "He develops in C++",
        "She researches AI",
        "AI is a hype market"
    };

    // Assign stable IDs
    for (size_t i = 0; i < sentences.size(); ++i) {
        sindex.append(sentences[i], static_cast<int64_t>(1000 + i));
    }

    // Expected results (rough reconstruction, may ignore punctuation)
    std::vector<std::string> expected = {
        "Artificial intelligence is transforming the world",
        "He develops in C++",
        "She researches AI",
        "AI is a hype market"
    };

    // --- Check before merge ---
    for (size_t i = 0; i < sentences.size(); ++i) {
        int64_t sid = 1000 + i;
        std::string recon = sindex.reconstruct_sid(sid);
        std::cout << "SID=" << sid << " -> '" << recon << "'\n";
        assert(!recon.empty());
        // normalize by removing trailing whitespace
        if (recon.back() == ' ') recon.pop_back();
        assert(recon.find(expected[i]) != std::string::npos);
    }

    // --- Merge shards ---
    if (sindex.shard_count() > 1) {
        sindex.merge_last_two();
    }

    // --- Check after merge ---
    for (size_t i = 0; i < sentences.size(); ++i) {
        int64_t sid = 1000 + i;
        std::string recon = sindex.reconstruct_sid(sid);
        std::cout << "[After merge] SID=" << sid << " -> '" << recon << "'\n";
        assert(!recon.empty());
        if (recon.back() == ' ') recon.pop_back();
        assert(recon.find(expected[i]) != std::string::npos);
    }

    std::cout << "✅ Reconstruction test passed.\n";
    return 0;
}

