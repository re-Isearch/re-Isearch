/*
Go from SearchResult to IRSET .....
Needs IDB Parent
*/

//#define VECTOR_INDEX

#define GC_DEL /* Mark deleted re_Isearch to Vector DB */
#define GC_DEL_UPDATE


/* Can't have DEL_UPDATE without the DEL */
#ifdef GC_DEL_UPDATE
# define GC_DEL
#endif

#define MT_DELETE 

#ifdef VECTOR_INDEX

#include "EmbeddingIndexer.hpp"
#include "Logger.hpp"

#ifdef MT_DELETE
#include <future>
#endif

static const char default_model[] = "sbert.ggml";
static const char search_path[] = "/opt/nonmonotonic/schmate/etc:/usr/local/ib/etc:~/.ib/models:.";

// To handle multiple embedding models we'll use the Virtual DBs.. each
// DB in the ensemble can have its own model.. Eg. a virtual DB with two
// DBs: A and B. DbA for modelA and DbB for modelB.
// A search of the ensembed A+B would search both each with their own model..

EmbeddingIndexer::EmbeddingIndexer(IDB *Parent_) : Parent(Parent_) {
  if (Parent) {
    const char section[] = "Embedding";
    STRING project_ = Parent->ProfileGetString(section, "project");

    ConfigLoader loader;
    cfg = loader.load_with_project(project_.toStdString());

    std::string  model = cfg.model_name;
    if (model.empty()) {
        STRING model_   = Parent->ProfileGetString(section, "model");
        if (model_.IsEmpty()) {
           // Default Model
           model = find_ggml_model(default_model, search_path).first;
        } else model = model_.toStdString();
        cfg.model_name = model;
    }

   // create embedder first
#ifdef USE_EMBEDDER_FACTORY
   // Use the factory to handle both bert.cpp and llama.cpp
   embedder = std::make_unique<EmbedderFactory>(model);
#else
   embedder = std::make_unique<SBertGGML>(model);
#endif

   // manager uses references to embedder? our manager takes embedder ref in constructor earlier.
   size_t cache_size = embedder ? determine_optimal_hnsw_cache_size(cfg, embedder->n_embd) : 0;
   if (cfg.debug) LOG_DEBUG_S() << "Optimal Index Cache Size: " << cache_size;
   if (embedder-> ctx) {
#if USE_LRUCACHE
     manager = std::make_unique<BertIndexManager>(*embedder, cfg, cache_size);
#else
     manager = std::make_unique<BertIndexManager>(*embedder, cfg);
#endif
  }
 }
}

bool EmbeddingIndexer::Ok() const
{
 return (Parent && embedder && manager);
}



// Make deleted in the Vector DB what is deleted in the re-Isearch index
#ifdef MT_DELETE /* Do this parallel across the shards! */
size_t EmbeddingIndexer::deleteDeleted(const STRING &fieldname)
{
    size_t deleted_count = 0;
    if (manager) {
        const std::string name        = fieldname.toStdString();

        // Get the index once, single-threaded, before launching futures
        auto              index       = manager->get(name);

        if (!index) return 0;
        const size_t      shard_count = index->shard_count();

        std::vector<std::future<size_t>> futures;
        futures.reserve(shard_count + 1);

        for (size_t i = 0; i <= shard_count; i++) {
            futures.push_back(std::async(std::launch::async,
                [this, index, i]() {
                    return index->removeDeletedElements(
                        [this, index, i](size_t label) {
                            const GPTYPE gp = index->get_sentence_id(label, i);
                            return Parent->GetDocumentDeletedByGP(gp);
                        }, i);
                }));
        }

        for (auto& f : futures) {
            deleted_count += f.get();
        }
    }
    return deleted_count;
}

#else // Serial 
size_t  EmbeddingIndexer::deleteDeleted(const STRING &fieldname)
{
  size_t deleted_count = 0;
  if (manager) {
    const std::string  name = fieldname.toStdString();
    auto               index       = &(manager->get(name));

    if (!index) return 0;
    const size_t       shard_count = index->shard_count ();
    // We need to handle all the shards!
    for (size_t i = 0; i<= shard_count; i++) {
     deleted_count += index->removeDeletedElements(
      [this, index, i](size_t label) { 
        const GPTYPE gp = index->get_sentence_id(label, i);
	return Parent->GetDocumentDeletedByGP(gp); }, i);
     }
  }
  return deleted_count;
}
#endif // MT



#if 1

PIRSET EmbeddingIndexer::search(const STRING &fieldname, const STRING &query)
{
    if (!manager) return nullptr;

    const std::string name  = fieldname.toStdString();
    const float       boost = 1.0f;

    auto              index       = manager->get(name);
    if (!index) {
       Parent->SetErrorCode(114); // "Unsupported Use attribute"
       return new IRSET (Parent); // No index -> Nothing to search -> Empty set
    }

    IRESULT iresult;
    iresult.SetVirtualIndex((UCHR)(Parent->GetVolume(NULL)));
    iresult.SetMdt(Parent->GetMainMdt());
    iresult.SetHitCount(1);
    iresult.SetAuxCount(1);

#ifdef GC_DEL_UPDATE
    // constexpr float DELETION_THRESHOLD_PCT = cfg.deletion_threshold_pc;  // retry if >% deleted
#endif

    auto results = index->search(query.toStdString());
    PIRSET pirset = new IRSET(Parent, results.size() + 1); // results.size()+1 we use as the increment.

    auto process_results = [&](const auto& results) -> size_t {
        MDTREC mdtrec;
        FC     fc;
        size_t deleted_count = 0;

        pirset->Clear();  // reuse the allocation

        for (const auto& r : results) {
            const GPTYPE gp = r.sentence_id;
            size_t w = Parent->GetMainMdt()->LookupByGp(gp);
            if (w == 0) continue;

            if (!Parent->GetMainMdt()->GetEntry(w, &mdtrec)) continue;

            if (mdtrec.GetDeleted()) {
#ifdef GC_DEL
                index->remove(r.label, r.shard);
#endif
                deleted_count++;
                continue;
            }

            iresult.SetMdtIndex(w);
            iresult.SetScore(r.score * boost);
#if 1 /* Include the whole range.. Let IRSET fix it */
            fc.SetFieldStart(gp);
            fc.SetFieldEnd(gp + r.span);
#else /* include only the bits */
            fc.SetFieldStart(gp + r.start_tok);
            fc.SetFieldEnd(gp + r.end_tok);
#endif
            iresult.SetHitTable(fc);
            pirset->AddEntry(iresult, true);
        }
        return deleted_count;
    };

#ifdef GC_DEL_UPDATE
   const size_t deletion_threshold = std::max(size_t(1),
                static_cast<size_t>(results.size() * cfg.deletion_threshold_pc));
    while (true) {
        size_t deleted = process_results(results);
        if (deleted < deletion_threshold) return pirset;
        results = index->search(query.toStdString());
    }
#else
    process_results(results);
    return pirset;
#endif
}





#else


PIRSET  EmbeddingIndexer::search(const STRING &fieldname, const STRING &query) {
  const float           boost = 1.0;

  if (manager) {
    IRESULT         iresult;
    MDTREC          mdtrec;
    FC              fc;
    PIRSET          pirset = NULL;

    iresult.SetVirtualIndex( (UCHR)( Parent->GetVolume(NULL) ) );
    iresult.SetMdt (Parent->GetMainMdt() );
    iresult.SetHitCount (1);
    iresult.SetAuxCount (1);
    const std::string  name = fieldname.toStdString();

#ifdef GC_DEL_UPDATE
    size_t   deleted;
search_again:
    deleted = 0;
#endif
    // Here gplist from SearchResult sentence_id and a loop 
    auto results = manager->search(name, query.toStdString() );

    // NOTE: IRSET will expand itself when needed!
    if (pirset == NULL) pirset = new IRSET ( Parent, results.size() + 1);


    for (const auto &r : results) {
     const GPTYPE gp = r.sentence_id;
     float mult = boost; //  + (r.token_end - r.token_start)/50.0; 
     size_t w = Parent->GetMainMdt ()->LookupByGp (gp);
     if (w == 0) continue; // Could not find GP
     if (Parent->GetMainMdt ()->GetEntry (w, &mdtrec)) {
        if (mdtrec.GetDeleted()) {
#ifdef GC_DEL
          // Should delete  r.label from the index
          manager->remove(name, r.label, r.shard);
#ifdef GC_DEL_UPDATE
          deleted++;
#endif
#endif
          continue; // Deleted entry
        }
#ifdef GC_DEL_UPDATE
        if (deleted) goto search_again; 
#endif
        iresult.SetMdtIndex (w);
        iresult.SetScore(r.score * mult);
#if 1 /* Include the whole range.. Let IRSET fix it */
        fc.SetFieldStart(gp);
        fc.SetFieldEnd(gp + r.span);
#else /* include only the bits */
        fc.SetFieldStart(gp + r.start_tok);
        fc.SetFieldEnd(gp + r.end_tok); 
#endif
        iresult.SetHitTable (fc);
        // Add entry
        pirset->AddEntry (iresult, true);
     }
    }
    return pirset;
  }
  return NULL;
}
#endif


#endif
