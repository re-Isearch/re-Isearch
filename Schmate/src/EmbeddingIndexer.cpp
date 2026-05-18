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

#define ENABLE_PASSTHROUGH 1

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
#include "unified_hnsw.hpp"

#ifdef MT_DELETE
#include <future>
#endif

#ifdef BERT_API_VERSION 
static const char default_model[] = "sbert.gguf";
#else
# define BERT_API_VERSION 0
static const char default_model[] = "sbert.ggml";
#endif


static const char search_path[] = "/opt/nonmonotonic/schmate/etc:/usr/local/ib/etc:~/.ib/models:../lib:.";


#ifdef DEBUG

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
#endif

// To handle multiple embedding models we'll use the Virtual DBs.. each
// DB in the ensemble can have its own model.. Eg. a virtual DB with two
// DBs: A and B. DbA for modelA and DbB for modelB.
// A search of the ensembed A+B would search both each with their own model..

EmbeddingIndexer::EmbeddingIndexer(IDBOBJ *Parent_, bool searchOnly) : Parent(Parent_) {

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
#if ENABLE_PASSTHROUGH
   auto q = get_model_quant(model);
   hnswlib::StorageType storage = q.first;
   if (storage != hnswlib::StorageType::FLOAT32) {
     message_log (LOG_INFO, "MODEL is %s (%s)", q.second.c_str(), storage_type_to_string(storage).c_str() );
     cfg->set_storage_type(storage);
     cfg->set_quantization( hnswlib::QuantMode::NONE);
   }
#endif
#ifdef USE_EMBEDDER_FACTORY
   // Use the factory to handle both bert.cpp and llama.cpp
   embedder = EmbedderFactory(model);
#else
   // Need to search since this logic is part of the factory above..
   auto found = find_model(model, search_path);
   if (found.second == GGML_TYPE::UNKNOWN) {
     message_log (LOG_ERROR, "GGML model '%s' not resolved", model.c_str()); 
     return;
   } 
#if BERT_API_VERSION == 2
   if (found.second == GGML_TYPE::GGML) {
     message_log (LOG_ERROR, "Model '%s' uses obsolete format (GGML). Use GGUF.", model.c_str());
   }
#endif

   model = found.first;
   embedder = std::make_unique<SBertGGML>(model);
#endif
   cfg->model_name = embedder->model_name();

   Parent->RevalidateFileCache(); // Work around should the libs have messed with handles!

   // manager uses references to embedder? our manager takes embedder ref in constructor earlier.
   size_t cache_size = embedder ? determine_optimal_hnsw_cache_size(*cfg, embedder->n_embd) : 0;
   if (cfg->debug) LOG_DEBUG_S() << "Optimal Index Cache Size: " << cache_size;
   if (embedder-> ctx) {

#if USE_LRUCACHE
     manager = std::make_unique<BertIndexManager>(*embedder, *cfg, cache_size, searchOnly, Parent);
#else
     manager = std::make_unique<BertIndexManager>(*embedder, *cfg, searchOnly, Parent);
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
size_t EmbeddingIndexer::deleteDeleted(const STRING &filename)
{
    size_t deleted_count = 0;
    if (manager) {
        const std::string name        = filename.toStdString();

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
size_t  EmbeddingIndexer::deleteDeleted(const STRING &filename)
{
  size_t deleted_count = 0;
  if (manager) {
    const std::string  name = filename.toStdString();
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


// TODO: Implement the unified code (use TargetName) in the rest of
// the library

PIRSET EmbeddingIndexer::search(const STRING &fieldname, const STRING &query)
{
    if (!manager) return nullptr;

    STRING fileStem;
    int field_id = Parent->GetMainDfdt()->GetFileNumber(fieldname);
    if (unified) {
       fileStem = Parent->GetDbFileStem();
    } else if (field_id > 0) {
       fileStem = Parent->ComposeDbFn(field_id);
    } else {
       message_log (LOG_ERROR, "[EmbeddingIndex] Could not get a filename for '%s'. DFD Defect?", fieldname.c_str() );
       Parent->SetErrorCode( 2 ); // "Temporary system error"
       return NULL;
    }
    const hnswlib::TargetName name (fileStem.toStdString(), field_id);
    // We now have both field_id and fileStem (which HNSW to search)
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
        }
        return deleted_count;
    };

#ifdef GC_DEL_UPDATE
   const size_t deletion_threshold = std::max(size_t(1),
                static_cast<size_t>(results.size() * cfg->deletion_threshold_pc));
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


// We generally call this with buffer, fieldname, GPStart and GPEnd
bool EmbeddingIndexer::Append(const STRING& buffer, const STRING &fieldname, const FC& fc) {
    if (manager) {
      STRING fileStem;
      int field_id = Parent->GetMainDfdt()->GetFileNumber(fieldname);
      if (unified) {
        fileStem = Parent->GetDbFileStem();
      } else if (field_id > 0) {
        fileStem = Parent->ComposeDbFn(field_id);
      } else {
        message_log (LOG_ERROR, "[EmbeddingIndex] Could not get a filename for '%s'. DFD Defect?", fieldname.c_str() );
        Parent->SetErrorCode( 109 ); // "Database unavailable"
        return false;
       }
       const hnswlib::TargetName  name (fileStem.toStdString(), field_id);
       // We now have both field_id and fileStem (which HNSW to search)
       manager->append(name, buffer, fc.GetFieldStart(), (uint32_t)fc.Span());
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

/*
std::vector<SearchResult> EmbeddingIndexer::search(const std::string &filename, const std::string &query) {
    if (manager) return manager->search(filename, query);
    return {};
}
*/


EmbeddingIndexer::~EmbeddingIndexer() = default;


class ReIsearchSentenceStore : public SentenceStore {
private:
    IDBOBJ* parent;

public:
    ReIsearchSentenceStore(IDBOBJ* p) : parent(p) {}

    bool open(const std::string& /*path*/) override {
        // We ignore the path. The engine is already open.
        return parent && parent->Ok();
    }

    // No-ops: The engine manages its own lifecycle and writes
    void close() override {}
    void flush() override {}
    size_t size() const override { return 0; } 

    // This is the "Safety Valve": We don't append via the bridge
    int64_t append(std::string_view /*text*/) override { return 0; }
    
    // Likely a No-op or throws an error in bridge mode
    int64_t append_from(SentenceStore& /*source*/) override { return 0; }

    // THE ONLY ACTIVE GEAR:
    std::string get(const OffsetEntry& e) override {
        if (!parent) return "";
        
        // Construct the FC (Field Coordinates) from sid and span
        // sid is our GPTYPE (Global Pointer)
        FC hit(e.sid, e.sid + e.span);
        
        // The engine fetches the content (tags and all)
        STRING res = parent->GetPeerContent(hit);
        return std::string(res.c_str(), res.GetLength());
    }
};


std::unique_ptr<SentenceStore> SentenceStoreFactory::CreateBridgeStore(void* ptr) {
    if (!ptr) return nullptr;
    return std::make_unique<ReIsearchSentenceStore>((IDBOBJ*)ptr);
}


#if BERT_API_VERSION > 1

// Custom message handler
// Install as     ggml_log_set(schmate_message_router, nullptr);
static void schmate_message_router(enum ggml_log_level level, const char * text, void * user_data) {
    // user_data can point to an instance of your logger class if needed
    // e.g., MyLogger* logger = static_cast<MyLogger*>(user_data);

    switch (level) {
        case GGML_LOG_LEVEL_ERROR:
	    message_log (LOG_ERROR, text);,
            break;
        case GGML_LOG_LEVEL_WARN:
            message_log (LOG_WARN, text);,
            break;
        case GGML_LOG_LEVEL_INFO:
        default:
            message_log (LOG_INFO, text);,
            break;
    }
}

#endif




#endif // VECTOR_INDEX
