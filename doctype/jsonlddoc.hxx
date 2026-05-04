/*-@@@
File:           jsonlddoc.hxx
Version:        1.00
Description:    Class JSONLDDOC - JSON-LD (Linked Data) Document Type
Author:         Based on JSONDOC by Edward C. Zimmermann
Copyright:      Copyright (c) 2026 re-Isearch Project
                Licensed under the Apache 2.0 license

Notes:
  Extends JSONDOC to handle JSON-LD (https://www.w3.org/TR/json-ld/).

  JSON-LD adds a set of '@' keywords to plain JSON:

    @context   Term-to-IRI map; resolved into the UnifiedNames table.
               URL-based contexts are looked up from a local cache
               (option ContextCacheDir=) or ignored with a warning.
    @id        Node identifier → stored as record key and as field _id
    @type      RDF type(s)    → indexed as field _type
    @value     Literal value  → unwrapped and indexed under the parent key
    @language  Language tag   → stored as field _language on the same node
    @graph     Named graph    → each element treated as a top-level record
    @set       Explicit set   → treated identically to a JSON array
    @list      Ordered list   → treated identically to a JSON array
    @nest      Nesting alias  → transparent; recursed with same prefix

  Term expansion from @context
  -----------------------------
  Inline @context objects are parsed into a local term map.  If a term
  expands to a full IRI (http://...) the local name (last path/hash
  segment) is used as the field name unless the option
  ExpandIRIs=true is set, in which case the full IRI is used (with
  '/' and '#' replaced by the path separator).

  Doctype options (in .ini / IDB option string):
    PathSep=<char>              inherited from JSONDOC (default '|')
    IndexArrayElements=True     inherited from JSONDOC
    AutodetectFieldtypes=True   inherited from JSONDOC
    ExpandIRIs=False            default: use local name only
    ContextCacheDir=<path>      directory for cached remote @context files
@@@-*/

#ifndef JSONLDDOC_HXX
#define JSONLDDOC_HXX

#include "jsondoc.hxx"

// Built-in JSON-LD keyword mappings.
// Empty target string = skip (do not index).
// These are checked before the UnifiedNames tagRegistry / ini lookup.
struct JsonLdKeyword {
  const char *keyword;   // e.g. "@type"
  const char *mapped;    // e.g. "_type"  (empty = skip)
};


class JSONLDDOC : public JSONDOC {
public:
  JSONLDDOC(PIDBOBJ DbParent, const STRING& Name);

  const char *Description(PSTRLIST List) const;
  void SourceMIMEContent(PSTRING StringPtr) const;

  // UnifiedNames: resolves raw JSON-LD terms (including @keywords and
  // context-mapped terms) to canonical re-Isearch field names.
  // Returns 0 if the field should be skipped (mapped to "").
  INT UnifiedNames(const STRING& Tag, PSTRLIST Value) const;

  void ParseFields(PRECORD NewRecord);

  ~JSONLDDOC();

protected:
  // Called by ParseFields before the main field walk.
  // Parses an inline @context object or string and populates
  // m_ContextTerms / m_ContextValues for this document.
  void ParseContext(const char *json, size_t& pos);

  // Resolve a single context value (IRI or prefix:local) to a field name.
  STRING ResolveIRI(const STRING& iri) const;

  // Override ParseObject to handle @value unwrapping, @id, @type, @graph.
  // Declared protected so JSONLDDOC can call JSONDOC's version via
  // the explicit base-class call when needed.
  void ParseObject(const char *json, size_t& pos,
                   const STRING& prefix, int depth,
                   PRECORD record, GPTYPE base);

private:
  // Per-document term map populated from inline @context.
  // Parallel arrays: m_ContextTerms[i] maps to m_ContextValues[i].
  STRLIST m_ContextTerms;
  STRLIST m_ContextValues;

  // Prefix map from @context  ("schema" -> "http://schema.org/")
  STRLIST m_PrefixTerms;
  STRLIST m_PrefixValues;

  bool    m_ExpandIRIs;
  STRING  m_ContextCacheDir;

  // Detect whether an object is a JSON-LD value object
  // { "@value": ..., "@language": ... } or { "@value": ..., "@type": ... }
  bool IsValueObject(const char *json, size_t pos) const;

  // Load a cached remote context file; returns false if not available.
  bool LoadContextFromCache(const STRING& url);
};

typedef JSONLDDOC* PJSONLDDOC;

#endif /* JSONLDDOC_HXX */
