/* This class is the glue interface between re-Isearch and Schmate */
#pragma once
#ifndef EMBEDDING_H
# define EMBEDDING_H

// From re-Isearch
#include "common.hxx"
#include "idb.hxx"

// From Schmate
#include "BertIndexManager.hpp"
#include "ConfigBuilder.hpp"

// We support either bert.cpp directly or via a factory both
#ifdef USE_EMBEDDER_FACTORY
//# define USE_EMBEDDER_FACTORY /* Use the factory to handle both bert.cpp and llama.cpp */
#endif

#ifdef USE_EMBEDDER_FACTORY
# include "EmbedderFactory.hpp"
#endif

class EmbeddingIndexer {
  EmbeddingIndexer(IDB *Parent);

  // We generally call this with buffer, fieldname, GPStart and GPEnd
  inline bool Append(const STRING& buffer, const STRING &fieldname, const FC& fc) {
    if (manager) {
       // Schmate uses std::string so need to convert, the buffer is string_view so gets casted
       manager->append(fieldname.toStdString(), buffer, fc.GetFieldStart(), (uint32_t)fc.Span());
    }
    else return false;
    return true;
  }

  bool Ok() const;

  // When the re-Isearch index has a number of deleted elements we should call this
  // as with K-ANN we get K elements but some (or all) of these may have been deleted
  // which would reduce the number of returned elements.
  size_t deleteDeleted(const STRING &fieldname);

  std::vector<SearchResult> search(const std::string &fieldname, const std::string &query) {
    if (manager) return manager->search(fieldname, query);
    return {};
  }
  PIRSET  search(const STRING &fieldname, const STRING &query);
private:
  IDB*  Parent;
  hnswlib::HnswConfig cfg;
  std::unique_ptr<BertIndexManager> manager;
#ifdef USE_EMBEDDER_FACTORY
  std::unique_ptr<EmbedderFactory> embedder;
#else
  std::unique_ptr<SBertGGML> embedder;
#endif
} ;


#endif
