/* re-Isearch <--> Schmate Bridge. This module provides append/delete/search */

#pragma once
#ifndef EMBEDDING_H
# define EMBEDDING_H

// re-Isearch
class IDBOBJ;
class STRING;

// Schmate
class BertIndexManager;
class EmbedderFactory;
class SBertGGML;
class SearchResult;
namespace hnswlib { class HnswConfig; }

// We support either bert.cpp directly or via a factory both
#ifdef USE_EMBEDDER_FACTORY
//# define USE_EMBEDDER_FACTORY /* Use the factory to handle both bert.cpp and llama.cpp */
#endif

#ifdef USE_EMBEDDER_FACTORY
# include "EmbedderFactory.hpp"
#else
# include <memory>
#endif


/* This class is the glue interface between re-Isearch and Schmate */

class EmbeddingIndexer {
public:
  EmbeddingIndexer(IDBOBJ *Parent, bool searchOnly = false);
  ~EmbeddingIndexer();

  // We generally call this with buffer, fieldname, GPStart and GPEnd
  bool Append(const STRING& buffer, const STRING &fieldname, const FC& fc);

  bool Ok() const;

  bool Clear(const STRING &Fieldname);

  // When the re-Isearch index has a number of deleted elements we should call this
  // as with K-ANN we get K elements but some (or all) of these may have been deleted
  // which would reduce the number of returned elements.
  size_t deleteDeleted(const STRING &filename);

  PIRSET  search(const STRING &fieldname, const STRING &query);

private:
  IDBOBJ*  Parent;
  bool     unified = false;
  std::unique_ptr<hnswlib::HnswConfig> cfg;
  std::unique_ptr<BertIndexManager> manager;
#ifdef USE_EMBEDDER_FACTORY
  std::unique_ptr<BaseEmbedder> embedder; 
#else
  std::unique_ptr<SBertGGML> embedder;
#endif
//  std::vector<SearchResult> search(const std::string &filename, const std::string &query);
} ;


bool RemoveEmbeddingIndexFile(const STRING& path);


#endif
