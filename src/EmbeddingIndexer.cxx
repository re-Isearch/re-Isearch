/*
This is the bridge re-Isearch <--> Schmate. This is the basis for DeepQuarry.
*/

#define VECTOR_INDEX

#define GC_DEL /* Mark deleted re_Isearch to Vector DB */
#define GC_DEL_UPDATE


/* Can't have DEL_UPDATE without the DEL */
#ifdef GC_DEL_UPDATE
# define GC_DEL
#endif

#define MT_DELETE 


// Define VECTOR_INDEX is the code is to be used as part of re-Isearch (for coreQuarry). 
#ifdef VECTOR_INDEX

// From re-Isearch
#include "common.hxx"
#include "idb.hxx"

// From Schmate
#include "Logger.hpp"
#include "BertIndexManager.hpp"
#include "ConfigBuilder.hpp"

// Bridge
#include "EmbeddingIndexer.hpp"

#ifdef MT_DELETE
#include <future>
#endif

static const char default_model[] = "sbert.ggml";
static const char search_path[] = "/opt/nonmonotonic/schmate/etc:/usr/local/ib/etc:~/.ib/models:../lib:.";


// Unified printResults() for all search modes
template <typename ResultVec>
inline void printResults(const ResultVec &results, bool debug = false) {
    using std::cout;
    using std::endl;

#ifdef NO_COLOR 
    bool use_color = false;
#else
    bool use_color = isatty(STDOUT_FILENO);
#endif

// --- Optional ANSI terminal colors  ---
    static const char *COLOR_RESET = "\033[0m";
    static const char *COLOR_ERROR = "\033[31;1;4m";
    static const char *COLOR_SCORE = "\033[38;5;39m";  // blue
    static const char *COLOR_LABEL = "\033[38;5;208m"; // orange
    static const char *COLOR_TEXT  = "\033[38;5;250m"; // gray
    static const char *COLOR_SID   = "\033[38;5;82m";  // green
    static const char *COLOR_NUM   = "\033[38;5;57m";    // strong purple


    if (!use_color)
        COLOR_RESET = COLOR_ERROR = COLOR_SCORE = COLOR_LABEL = COLOR_TEXT = COLOR_SID = "";

    if (results.empty()) {
        cout << " - " << COLOR_ERROR <<  "(no results)" << COLOR_RESET << " -" << endl;
        return;
    }

    cout << "# Got " << COLOR_NUM << results.size() << COLOR_RESET << " hits" << endl;
    for (const auto &r : results) {
        cout << " - [score=" << COLOR_SCORE << std::fixed << std::setprecision(6)
             << r.score << COLOR_RESET
             << ", sid=" << COLOR_SID << r.sentence_id << COLOR_RESET
             << ", label=" << COLOR_LABEL << r.label << COLOR_RESET
             << ", tokens=[" << r.token_start << "," << r.token_end << "]] ";

        cout << COLOR_TEXT << r.text << COLOR_RESET << endl;

        if (debug) {
            cout << "   file=[" << r.file_start << "," << r.file_end << "]";
            // if (r.address) cout << " addr=" << r.address;
            cout << endl;
        }
    }
}


// To handle multiple embedding models we'll use the Virtual DBs.. each
// DB in the ensemble can have its own model.. Eg. a virtual DB with two
// DBs: A and B. DbA for modelA and DbB for modelB.
// A search of the ensembed A+B would search both each with their own model..

EmbeddingIndexer::EmbeddingIndexer(IDBOBJ *Parent_) : Parent(Parent_) {

  Logger::instance().setPrefix( _globalMessageLogger.get_prefix()); 

  if (Parent) {
    const char section[] = "Embedding";
    STRING project_ = Parent->ProfileGetString(section, "project");

    ConfigLoader loader;
    cfg = std::make_unique<hnswlib::HnswConfig>( loader.load_with_project(project_.toStdString()));

    std::string  model = cfg->model_name;
    if (model.empty()) {
        STRING model_   = Parent->ProfileGetString(section, "model");
        if (model_.IsEmpty())
           model = default_model; // Default Model
        else model = model_.toStdString();
    }

   // create embedder first
#ifdef USE_EMBEDDER_FACTORY
   // Use the factory to handle both bert.cpp and llama.cpp
   embedder = std::make_unique<EmbedderFactory>(model);
#else
   // Need to search since this logic is part of the factory above..
   auto found = find_model(model, search_path);
   if (found.second == GGML_TYPE::UNKNOWN) {
     message_log (LOG_ERROR, "GGML model '%s' not resolved", model.c_str()); 
     return;
   } 
   model = found.first;
   embedder = std::make_unique<SBertGGML>(model);
#endif
   cfg->model_name = embedder->model_name;

   Parent->RevalidateFileCache(); // Work around should the libs have messed with handles!

   // manager uses references to embedder? our manager takes embedder ref in constructor earlier.
   size_t cache_size = embedder ? determine_optimal_hnsw_cache_size(*cfg, embedder->n_embd) : 0;
   if (cfg->debug) LOG_DEBUG_S() << "Optimal Index Cache Size: " << cache_size;
   if (embedder-> ctx) {
#if USE_LRUCACHE
     manager = std::make_unique<BertIndexManager>(*embedder, *cfg, cache_size);
#else
     manager = std::make_unique<BertIndexManager>(*embedder, *cfg);
#endif
  }
 }
}

bool EmbeddingIndexer::Ok() const
{
   volatile const void* self = this;
   if (self == nullptr) {
     message_log (LOG_ERROR,"EmbeddingIndexer:OK() called when self is null!");
     return false;
  }
  // if Parent we should have an embedder.. if it was OK.. then manager
  return  (Parent && embedder && manager);
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
    auto               index       = manager->get(name);

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
       message_log (LOG_ERROR, "EmbeddingIndex::search: index '%s' not found", fieldname.c_str()); 
       Parent->SetErrorCode(114); // "Unsupported Use attribute"
       return new IRSET (Parent); // No index -> Nothing to search -> Empty set
    }

    IRESULT iresult;
#if 1
    INDEX_ID  idx;
    idx.SetVirtualIndex((UCHR)( Parent->GetVolume(NULL) ) );
#else
    iresult.SetVirtualIndex((UCHR)(Parent->GetVolume(NULL)));
#endif
    iresult.SetMdt(Parent->GetMainMdt());
    iresult.SetHitCount(1);
    iresult.SetAuxCount(1);

#ifdef GC_DEL_UPDATE
    // constexpr float DELETION_THRESHOLD_PCT = cfg.deletion_threshold_pc;  // retry if >% deleted
#endif

    auto results = index->search(query.toStdString());

#ifdef DEBUG
    std::cerr << "GOT " << results.size() << " hits" << std::endl;
    printResults(results, true);
#endif
    PIRSET pirset = new IRSET(Parent, results.size() + 1); // results.size()+1 we use as the increment.

    if (pirset == nullptr) return nullptr; // Make sure allocated.. 

    auto process_results = [&](const auto& results) -> size_t {
        MDTREC mdtrec;
        FC     fc;
        size_t deleted_count = 0;

        pirset->Clear();  // reuse the allocation
        // Since we are adding by a sorted results.. can inform the irset..
        pirset->setSortedByScore();

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

#if 1
            idx.SetMdtIndex(w);
            iresult.SetIndex(idx);
#else
            iresult.SetMdtIndex(w);
#endif
            iresult.SetScore(r.score * boost);
            fc.SetFieldStart(gp);
            fc.SetFieldEnd(gp + r.span);
            iresult.SetHitTable(fc);
            pirset->FastAddEntry(iresult); // Can add fast since we are building from scratch
// std::cerr << "Added to priset" << std::endl;
        }
        return deleted_count;
    };

#ifdef GC_DEL_UPDATE
   const size_t deletion_threshold = std::max(size_t(1),
                static_cast<size_t>(results.size() * cfg->deletion_threshold_pc));
    while (true) {
        size_t deleted = process_results(results);
//std::cerr << "DELETED = " << deleted << std::endl;
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

#if 1
    INDEX_ID  idx;
    idx.SetVirtualIndex((UCHR)( Parent->GetVolume(NULL) ) );
#else
    iresult.SetVirtualIndex( (UCHR)( Parent->GetVolume(NULL) ) );
#endif
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

//std::cerr << "search returned " << results.size() << " elements" << std:endl;

    // NOTE: IRSET will expand itself when needed!
    if (pirset == NULL) pirset = new IRSET ( Parent, results.size() + 1);


    for (const auto &r : results) {
     const GPTYPE gp = r.sentence_id;
     float mult = boost; //  + (r.token_end - r.token_start)/50.0; 
cerr << "LOOKUP GP address " << endl;
     size_t w = Parent->GetMainMdt ()->LookupByGp (gp);
cerr << "GOT it: " << w << endl;
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

//std::cerr << "Adding Index " << s << " GP= (" << gp << ", " << gp + r.span << ")" << std:endl;
#if 1
	idx.SetMdtIndex(w);
	iresult.SetIndex(idx);
#else
        iresult.SetMdtIndex (w);
#endif
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



// We generally call this with buffer, fieldname, GPStart and GPEnd
bool EmbeddingIndexer::Append(const STRING& buffer, const STRING &fieldname, const FC& fc) {
    if (manager) {
//std::cerr << "DEBUG: Appending: " << buffer << std::endl;
       // Schmate uses std::string so need to convert, the buffer is string_view so gets casted
       manager->append(fieldname.toStdString(), buffer, fc.GetFieldStart(), (uint32_t)fc.Span());
    }
    else return false;
    return true;
}


bool EmbeddingIndexer::Clear(const STRING &Fieldname)
{
  if (manager) {
      manager->clear(Fieldname.toStdString());
      return true;
  }
  return false;
}

bool RemoveEmbeddingIndexFile(const STRING& path)
{
  return ShardedIndex::unlink(path.toStdString()) ;
}

std::vector<SearchResult> EmbeddingIndexer::search(const std::string &fieldname, const std::string &query) {
    if (manager) return manager->search(fieldname, query);
    return {};
}


EmbeddingIndexer::~EmbeddingIndexer() = default;



#endif // VECTOR_INDEX
