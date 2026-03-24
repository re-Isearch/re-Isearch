/* This class is the glue interface between re-Isearch and Schmate */

#include "string.hxx"
#include "idb.hxx"

class EmbeddingIndexer {
   EmbeddingIndex(const char *model, bool debug = false) {
     ConfigLoader loader;
     HnswConfig cfg = loader.load(debug);

     // create embedder first
#if 0 // Use the factory to handle both bert.cpp and llama.cpp
     embedder = std::make_unique<EmbedderFactory>(model);
#else
     embedder = std::make_unique<SBertGGML>(model);
#endif

     // manager uses references to embedder? our manager takes embedder ref in constructor earlier.
     size_t cache_size = determine_optimal_hnsw_cache_size(cfg, embedder.n_embd);
     if (cfg.debug) LOG_DEBUG_S() << "Optimal Index Cache Size: " << cache_size;
     if (embedder-> bert_ctx)
       manager = std::make_unique<BertIndexManager>(embedder, cfg, cache_size);
  }

  // We generally call this with buffer, fieldname, GPStart and GPEnd
  inline bool Append(const STRING& buffer, const STRING &fieldname, const const FC& fc) {
    if (manager) {
       // Schmate uses std::string so need to convert, the buffer is string_view so gets casted
       manager->append(fieldname.toStdString(), buffer, fc.FieldStart, (uint32_t)fc.Span());
    }
    else return false;
    return true;
  }

  std::vector<SearchResult> search(const std::string &fieldname, const std::string &query) {
    if (manager) return manager->search(fieldname, querry);
    return {};
  }
  PIRSET  search(const STRING &fieldname, const STRING &query, IDB *Parent);
private:
  BertIndexManager *manager = NULL;
  SBertGGML        *embedder;
}

