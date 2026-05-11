/*-@@@
File:           jsonlddoc.cxx
Version:        1.00
Description:    Class JSONLDDOC - JSON-LD (Linked Data) Document Type
Author:         Based on JSONDOC by Edward C. Zimmermann
Copyright:      Copyright (c) 2026 re-Isearch Project
                Licensed under the Apache 2.0 license
@@@-*/

#include "jsonlddoc.hxx"
#include "common.hxx"
#include "doc_conf.hxx"

// ---------------------------------------------------------------------------
// Built-in JSON-LD keyword table (sorted for bsearch)
// Empty mapped name = skip entirely (do not index as a field)
// ---------------------------------------------------------------------------

static const JsonLdKeyword JsonLdKeywords[] = {
  /* SORTED */
  { "@base",      ""          },  // base IRI – affects context, not indexed
  { "@container", ""          },  // container type hint – not indexed
  { "@context",   ""          },  // handled specially in ParseFields
  { "@direction", "_direction"},  // text direction
  { "@graph",     "_graph"    },  // named graph – handled specially
  { "@id",        "_id"       },  // node identifier → record key
  { "@import",    ""          },  // context import – not yet supported
  { "@included",  ""          },  // included block – recurse as records
  { "@index",     ""          },  // index map key – not indexed
  { "@json",      ""          },  // JSON literal – not indexed
  { "@language",  "_language" },  // language tag
  { "@list",      ""          },  // ordered list – treated as array
  { "@nest",      ""          },  // nesting alias – transparent
  { "@none",      ""          },  // catch-all – skip
  { "@prefix",    ""          },  // prefix flag – not indexed
  { "@propagate", ""          },  // propagation flag – not indexed
  { "@protected", ""          },  // protected flag – not indexed
  { "@reverse",   "_reverse"  },  // reverse property
  { "@set",       ""          },  // explicit set – treated as array
  { "@type",      "_type"     },  // RDF type
  { "@value",     "_value"    },  // literal value – unwrapped to parent
  { "@version",   ""          },  // processing mode – not indexed
  { "@vocab",     ""          },  // default vocabulary – not indexed
};
static const int JsonLdKeywordsCount =
  (int)(sizeof(JsonLdKeywords) / sizeof(JsonLdKeywords[0]));

static int CmpKeyword(const void *a, const void *b)
{
  return strcmp((const char *)a,
                ((const JsonLdKeyword *)b)->keyword);
}

// Look up a @keyword.  Returns the mapped name, or nullptr if not found.
// Returns "" (empty string) if the keyword should be skipped.
static const char *LookupKeyword(const char *kw)
{
  const JsonLdKeyword *hit = (const JsonLdKeyword *)bsearch(
    kw, JsonLdKeywords, JsonLdKeywordsCount,
    sizeof(JsonLdKeyword), CmpKeyword);
  return hit ? hit->mapped : nullptr;
}

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

JSONLDDOC::JSONLDDOC(PIDBOBJ DbParent, const STRING& Name)
  : JSONDOC(DbParent, Name),
    m_ExpandIRIs(false)
{
  m_ExpandIRIs      = Getoption("ExpandIRIs",      "false").GetBool();
  m_ContextCacheDir = Getoption("ContextCacheDir", "");
}

JSONLDDOC::~JSONLDDOC() {}

// ---------------------------------------------------------------------------
// Description / MIME
// ---------------------------------------------------------------------------

const char *JSONLDDOC::Description(PSTRLIST List) const
{
  if (List) {
    const STRING ThisDoctype("JSON-LD");
    if (Doctype != ThisDoctype && List->IsEmpty())
      List->AddEntry(Doctype);
    List->AddEntry(ThisDoctype);
    JSONDOC::Description(List);
  }
  return "JSON-LD Document Type (JSON Linked Data, RFC 8259 + W3C JSON-LD 1.1).\n\
Extends JSONDOC with @context term mapping, @id record keys,\n\
@value unwrapping, @type/@language fields, and @graph multi-record support.\n\
Options (in .ini):\n\
  ExpandIRIs=False           use local IRI name only (default)\n\
  ContextCacheDir=<path>     directory for cached remote @context files";
}

void JSONLDDOC::SourceMIMEContent(PSTRING StringPtr) const
{
  if (StringPtr)
    *StringPtr = "application/ld+json";
}

// ---------------------------------------------------------------------------
// UnifiedNames
//
// Resolution order:
//   1. Built-in JSON-LD @keyword table
//   2. Per-document @context term map (m_ContextTerms/Values)
//   3. tagRegistry / ini [Fields] section (base class)
// ---------------------------------------------------------------------------

INT JSONLDDOC::UnifiedNames(const STRING& Tag, PSTRLIST Value) const
{
  const char *tagStr = Tag.c_str();

  // 1. Built-in @keyword?
  if (tagStr[0] == '@') {
    const char *mapped = LookupKeyword(tagStr);
    if (mapped != nullptr) {
      if (Value) {
        Value->Clear();
        if (mapped[0]) Value->AddEntry(STRING(mapped));
      }
      return mapped[0] ? 1 : 0;  // 0 = skip
    }
  }

  // 2. Per-document @context term map
  // m_ContextTerms[i] -> m_ContextValues[i]
  {
    const size_t n = m_ContextTerms.GetTotalEntries();
    for (size_t i = 1; i <= n; ++i) {
      if (m_ContextTerms.GetEntry(i) == Tag) {
        STRING mapped = m_ContextValues.GetEntry(i);
        if (Value) {
          Value->Clear();
          if (mapped.GetLength()) Value->AddEntry(mapped);
        }
        return mapped.GetLength() ? 1 : 0;
      }
    }
  }

  // 3. Fall through to base class (tagRegistry / ini)
  return JSONDOC::UnifiedNames(Tag, Value);
}

// ---------------------------------------------------------------------------
// ResolveIRI
//
// Given an IRI or prefixed name from @context, return the field name
// to use in the index.
//
// Examples:
//   "http://schema.org/name"  -> "name"         (ExpandIRIs=false)
//   "http://schema.org/name"  -> "http|schema.org|name" (ExpandIRIs=true)
//   "schema:name"             -> "name"  (prefix resolved if known)
//   "name"                    -> "name"  (plain term, returned as-is)
// ---------------------------------------------------------------------------

STRING JSONLDDOC::ResolveIRI(const STRING& iri) const
{
  const char *s = iri.c_str();

  // Full IRI: http:// or https:// or urn: etc.
  if (strncmp(s, "http://", 7)  == 0 ||
      strncmp(s, "https://", 8) == 0 ||
      strncmp(s, "urn:",  4)    == 0)
    {
      if (m_ExpandIRIs)
        {
          // Replace / and # with path separator
          STRING result(iri);
          // Strip scheme
          size_t schemeEnd = iri.Find("//");
          if (schemeEnd != (size_t)NOT_FOUND)
            result = iri.After(schemeEnd + 1);
          result.Replace('/', m_PathSep);
          result.Replace('#', m_PathSep);
          return result;
        }
      // Local name: last segment after / or #
      const char *lastSlash = strrchr(s, '/');
      const char *lastHash  = strrchr(s, '#');
      const char *local     = lastHash  ? lastHash  + 1 :
                              lastSlash ? lastSlash + 1 : s;
      return STRING(local);
    }

  // Prefixed name: "schema:name" -> resolve prefix if known
  const char *colon = strchr(s, ':');
  if (colon && colon != s)
    {
      STRING prefix(s, (size_t)(colon - s));
      STRING local(colon + 1);
      const size_t n = m_PrefixTerms.GetTotalEntries();
      for (size_t i = 1; i <= n; ++i)
        {
          if (m_PrefixTerms.GetEntry(i) == prefix)
            {
              STRING expanded = m_PrefixValues.GetEntry(i);
              expanded += local;
              return ResolveIRI(expanded);   // recurse to handle full IRI
            }
        }
      // Prefix not found — return local part only
      return local;
    }

  // Plain term or keyword — return as-is
  return iri;
}

// ---------------------------------------------------------------------------
// ParseContext
//
// Called when "@context" is encountered.  pos must be on the value
// (string URL, inline object, or array) that follows the "@context" key.
//
// Populates m_ContextTerms/Values and m_PrefixTerms/Values.
// ---------------------------------------------------------------------------

void JSONLDDOC::ParseContext(const char *json, size_t& pos)
{
  SkipWhitespace(json, pos);

  if (json[pos] == '"')
    {
      // URL reference — try to load from cache
      size_t uStart, uEnd;
      SkipString(json, pos, uStart, uEnd);
      if (uEnd >= uStart)
        {
          STRING url;
          for (size_t i = uStart; i <= uEnd; ++i) url += json[i];
          if (!LoadContextFromCache(url))
            message_log(LOG_WARN,
                        "JSONLDDOC: remote @context '%s' not in cache — "
                        "set ContextCacheDir= to enable",
                        url.c_str());
        }
    }
  else if (json[pos] == '[')
    {
      // Array of contexts — process each element
      ++pos; // consume '['
      while (true)
        {
          SkipWhitespace(json, pos);
          if (!json[pos] || json[pos] == ']') { if (json[pos]) ++pos; break; }
          ParseContext(json, pos);   // recurse for each element
          SkipWhitespace(json, pos);
          if (json[pos] == ',') ++pos;
        }
    }
  else if (json[pos] == '{')
    {
      ++pos; // consume '{'
      while (true)
        {
          SkipWhitespace(json, pos);
          if (!json[pos] || json[pos] == '}') { if (json[pos]) ++pos; break; }
          if (json[pos] != '"')
            {
              while (json[pos] && json[pos] != ',' && json[pos] != '}') ++pos;
              if (json[pos] == ',') ++pos;
              continue;
            }

          // Read the term name
          size_t tStart, tEnd;
          SkipString(json, pos, tStart, tEnd);
          STRING term;
          for (size_t i = tStart; i <= tEnd; ++i) term += json[i];

          SkipWhitespace(json, pos);
          if (json[pos] == ':') ++pos;
          SkipWhitespace(json, pos);

          if (term == "@base" || term == "@vocab")
            {
              // @base / @vocab set the default IRI prefix
              if (json[pos] == '"')
                {
                  size_t vStart, vEnd;
                  SkipString(json, pos, vStart, vEnd);
                  STRING iri;
                  for (size_t i = vStart; i <= vEnd; ++i) iri += json[i];
                  if (term == "@vocab")
                    {
                      m_PrefixTerms.AddEntry(STRING("@vocab"));
                      m_PrefixValues.AddEntry(iri);
                    }
                }
              else
                {
                  // null or other — skip value
                  while (json[pos] && json[pos] != ',' && json[pos] != '}')
                    ++pos;
                }
            }
          else if (json[pos] == '"')
            {
              // Simple term: "name": "http://schema.org/name"
              size_t vStart, vEnd;
              SkipString(json, pos, vStart, vEnd);
              STRING iri;
              for (size_t i = vStart; i <= vEnd; ++i) iri += json[i];

              // Check if it's a prefix definition ("schema": "http://schema.org/")
              size_t iriLen = iri.GetLength();
              const char *iriStr = iri.c_str();
              bool isPrefix = (iriLen > 0 &&
                               (iriStr[iriLen-1] == '/' || iriStr[iriLen-1] == '#'));
              if (isPrefix)
                {
                  m_PrefixTerms.AddEntry(term);
                  m_PrefixValues.AddEntry(iri);
                }
              else
                {
                  STRING fieldName = ResolveIRI(iri);
                  m_ContextTerms.AddEntry(term);
                  m_ContextValues.AddEntry(fieldName);
                }
            }
          else if (json[pos] == '{')
            {
              // Expanded term definition: "name": { "@id": "schema:name", ... }
              // Find the @id within it
              size_t savedPos = pos;
              ++pos; // consume '{'
              STRING mappedId;
              while (true)
                {
                  SkipWhitespace(json, pos);
                  if (!json[pos] || json[pos] == '}') { if (json[pos]) ++pos; break; }
                  if (json[pos] != '"')
                    {
                      while (json[pos] && json[pos] != ',' && json[pos] != '}') ++pos;
                      if (json[pos] == ',') ++pos;
                      continue;
                    }
                  size_t kStart, kEnd;
                  SkipString(json, pos, kStart, kEnd);
                  STRING innerKey;
                  for (size_t i = kStart; i <= kEnd; ++i) innerKey += json[i];
                  SkipWhitespace(json, pos);
                  if (json[pos] == ':') ++pos;
                  SkipWhitespace(json, pos);
                  if (innerKey == "@id" && json[pos] == '"')
                    {
                      size_t vStart, vEnd;
                      SkipString(json, pos, vStart, vEnd);
                      for (size_t i = vStart; i <= vEnd; ++i) mappedId += json[i];
                    }
                  else
                    {
                      // skip value
                      while (json[pos] && json[pos] != ',' && json[pos] != '}') ++pos;
                    }
                  SkipWhitespace(json, pos);
                  if (json[pos] == ',') ++pos;
                }
              if (mappedId.GetLength() > 0)
                {
                  STRING fieldName = ResolveIRI(mappedId);
                  m_ContextTerms.AddEntry(term);
                  m_ContextValues.AddEntry(fieldName);
                }
            }
          else
            {
              // null or unhandled — skip
              while (json[pos] && json[pos] != ',' && json[pos] != '}') ++pos;
            }

          SkipWhitespace(json, pos);
          if (json[pos] == ',') ++pos;
        }
    }
  else
    {
      // null or unknown — skip
      while (json[pos] && json[pos] != ',' && json[pos] != '}') ++pos;
    }
}

// ---------------------------------------------------------------------------
// LoadContextFromCache
//
// Looks for a cached copy of a remote @context URL in ContextCacheDir.
// The filename is derived by replacing '/' and ':' with '_'.
// Returns false if not found / not configured.
// ---------------------------------------------------------------------------

bool JSONLDDOC::LoadContextFromCache(const STRING& url)
{
  if (m_ContextCacheDir.IsEmpty())
    return false;

  // Derive cache filename from URL
  STRING cacheFile(m_ContextCacheDir);
  cacheFile += "/";
  STRING safeName(url);
  safeName.Replace('/', '_');
  safeName.Replace(':', '_');
  safeName.Replace('.', '_');
  cacheFile += safeName;
  cacheFile += ".jsonld";

  PFILE fp = fopen(cacheFile.c_str(), "rb");
  if (!fp)
    return false;

  fseek(fp, 0, SEEK_END);
  long len = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  if (len <= 0) { fclose(fp); return false; }

  char *buf = new char[len + 1];
  size_t nRead = fread(buf, 1, len, fp);
  fclose(fp);
  buf[nRead] = '\0';

  // The cache file should contain the @context value (the object, not
  // the full document), or a full JSON-LD document with a @context key.
  size_t pos = 0;
  SkipWhitespace(buf, pos);
  if (buf[pos] == '{')
    {
      // Full document — scan for @context key
      ++pos;
      while (buf[pos])
        {
          SkipWhitespace(buf, pos);
          if (buf[pos] == '}') break;
          if (buf[pos] != '"')
            {
              while (buf[pos] && buf[pos] != ',' && buf[pos] != '}') ++pos;
              if (buf[pos] == ',') ++pos;
              continue;
            }
          size_t kStart, kEnd;
          SkipString(buf, pos, kStart, kEnd);
          STRING key;
          for (size_t i = kStart; i <= kEnd; ++i) key += buf[i];
          SkipWhitespace(buf, pos);
          if (buf[pos] == ':') ++pos;
          SkipWhitespace(buf, pos);
          if (key == "@context")
            {
              ParseContext(buf, pos);
              break;
            }
          // skip other keys
          while (buf[pos] && buf[pos] != ',' && buf[pos] != '}') ++pos;
          if (buf[pos] == ',') ++pos;
        }
    }
  else
    {
      // Bare context object or value
      ParseContext(buf, pos);
    }

  delete[] buf;
  message_log(LOG_INFO, "JSONLDDOC: loaded @context from cache '%s'",
              cacheFile.c_str());
  return true;
}

// ---------------------------------------------------------------------------
// IsValueObject
//
// Returns true if the object starting at pos (which must be on '{') is a
// JSON-LD value object: { "@value": ... } possibly with @language or @type.
// Does a lightweight lookahead without advancing pos.
// ---------------------------------------------------------------------------

bool JSONLDDOC::IsValueObject(const char *json, size_t pos) const
{
  if (json[pos] != '{') return false;
  ++pos; // skip '{'
  // scan keys of this object looking for "@value" as first non-trivial key
  int keysChecked = 0;
  while (json[pos] && keysChecked < 4)
    {
      SkipWhitespace(json, pos);
      if (json[pos] == '}') break;
      if (json[pos] != '"') break;
      size_t kStart = ++pos;
      while (json[pos] && json[pos] != '"')
        { if (json[pos] == '\\') ++pos; ++pos; }
      size_t kEnd = pos;
      if (json[pos] == '"') ++pos;
      // check if this key is "@value"
      if (kEnd - kStart == 6 &&
          strncmp(json + kStart, "@value", 6) == 0)
        return true;
      // skip the value
      SkipWhitespace(json, pos);
      if (json[pos] == ':') ++pos;
      SkipWhitespace(json, pos);
      // rudimentary skip of the value
      if (json[pos] == '"')
        { ++pos; while (json[pos] && json[pos] != '"') { if (json[pos]=='\\') ++pos; ++pos; } if (json[pos]) ++pos; }
      else if (json[pos] == '{' || json[pos] == '[')
        { int d=1; char o=json[pos],c=(o=='{'?'}':']'); ++pos; while (json[pos]&&d>0){if(json[pos]==o)d++;else if(json[pos]==c)d--;++pos;} }
      else
        { while (json[pos] && json[pos]!=',' && json[pos]!='}') ++pos; }
      SkipWhitespace(json, pos);
      if (json[pos] == ',') ++pos;
      ++keysChecked;
    }
  return false;
}

// ---------------------------------------------------------------------------
// ParseObject  (override)
//
// Handles JSON-LD semantics on top of the base JSON object parsing:
//
//   @context  → ParseContext (term map); do not index
//   @id       → record->SetKey(); also index as _id field
//   @type     → index as _type field
//   @value    → if this whole object is a value object, unwrap and index
//               the @value content under the PARENT key (prefix)
//   @language → index as _language alongside @value
//   @graph    → recurse into each element as if it were a top-level object
//   @set/@list→ treat as array (pass prefix unchanged)
//   @nest     → recurse transparently with same prefix
//   other @kw → skip (empty mapped name)
// ---------------------------------------------------------------------------

void JSONLDDOC::ParseObject(const char *json, size_t& pos,
                            const STRING& prefix, int depth,
                            PRECORD record, GPTYPE base)
{
  if (depth > JSON_MAX_DEPTH)
    {
      message_log(LOG_WARN, "JSONLDDOC: max nesting depth exceeded");
      int b=1; ++pos;
      while (json[pos] && b>0)
        { if(json[pos]=='{')b++; else if(json[pos]=='}')b--; ++pos; }
      return;
    }

  // ---- lookahead: is this a value object { "@value": ... }? ----
  // If so, unwrap it and index the content under the parent key (prefix).
  if (IsValueObject(json, pos))
    {
      // Scan the object for @value and @language/@type
      ++pos; // consume '{'
      STRING valueContents;
      size_t valueStart = (size_t)-1, valueEnd = 0;
      STRING language;

      while (true)
        {
          SkipWhitespace(json, pos);
          if (!json[pos] || json[pos] == '}') { if (json[pos]) ++pos; break; }
          if (json[pos] != '"')
            {
              while (json[pos] && json[pos] != ',' && json[pos] != '}') ++pos;
              if (json[pos] == ',') ++pos;
              continue;
            }
          size_t kStart, kEnd;
          SkipString(json, pos, kStart, kEnd);
          STRING key;
          for (size_t i = kStart; i <= kEnd; ++i) key += json[i];

          SkipWhitespace(json, pos);
          if (json[pos] == ':') ++pos;
          SkipWhitespace(json, pos);

          if (key == "@value" && json[pos] == '"')
            {
              SkipString(json, pos, valueStart, valueEnd);
              for (size_t i = valueStart; i <= valueEnd; ++i)
                valueContents += json[i];
            }
          else if (key == "@language" && json[pos] == '"')
            {
              size_t lStart, lEnd;
              SkipString(json, pos, lStart, lEnd);
              for (size_t i = lStart; i <= lEnd; ++i) language += json[i];
            }
          else
            {
              // @type or other — skip
              while (json[pos] && json[pos] != ',' && json[pos] != '}') ++pos;
            }
          SkipWhitespace(json, pos);
          if (json[pos] == ',') ++pos;
        }

      // Index @value content under the parent field name
      if (valueStart != (size_t)-1 && valueEnd >= valueStart)
        {
          AddField(record,
                   SanitiseFieldName(prefix),
                   (GPTYPE)(base + valueStart),
                   (GPTYPE)(base + valueEnd),
                   valueContents);
        }

      // Index @language as a sibling field (prefix + path-sep + "_language")
      if (language.GetLength() > 0 && prefix.GetLength() > 0)
        {
          // We don't have a meaningful FC for the language tag itself
          // beyond what we parsed — store it as a small inline field.
          // Since language tags are short and re-Isearch needs an FC,
          // we'd need the byte range of the language value.  Skip for now
          // — this would need a second pass or refactored SkipString call
          // that returns the range.  Log it instead:
          message_log(LOG_DEBUG, "JSONLDDOC: %s @language=%s",
                      (const char*)prefix, (const char*)language);
        }
      return;
    }

  // ---- normal object processing ----
  ++pos; // consume '{'

  while (true)
    {
      SkipWhitespace(json, pos);
      if (!json[pos] || json[pos] == '}') { if (json[pos]) ++pos; break; }
      if (json[pos] != '"')
        {
          while (json[pos] && json[pos] != ',' && json[pos] != '}') ++pos;
          if (json[pos] == ',') ++pos;
          continue;
        }

      // Read the key
      size_t kStart, kEnd;
      SkipString(json, pos, kStart, kEnd);
      STRING rawKey;
      for (size_t i = kStart; i <= kEnd; ++i) rawKey += json[i];

      SkipWhitespace(json, pos);
      if (json[pos] == ':') ++pos;
      SkipWhitespace(json, pos);

      // ---- JSON-LD special key handling ----

      if (rawKey == "@context")
        {
          ParseContext(json, pos);
          SkipWhitespace(json, pos);
          if (json[pos] == ',') ++pos;
          continue;
        }

      if (rawKey == "@id")
        {
          // Store as record key AND index as _id field
          if (json[pos] == '"')
            {
              size_t vStart, vEnd;
              SkipString(json, pos, vStart, vEnd);
              STRING idVal;
              for (size_t i = vStart; i <= vEnd; ++i) idVal += json[i];
              if (idVal.GetLength() > 0)
                {
                  record->SetKey(idVal);
                  AddField(record, STRING("_id"),
                           (GPTYPE)(base + vStart),
                           (GPTYPE)(base + vEnd),
                           idVal);
                }
            }
          else
            {
              while (json[pos] && json[pos] != ',' && json[pos] != '}') ++pos;
            }
          SkipWhitespace(json, pos);
          if (json[pos] == ',') ++pos;
          continue;
        }

      if (rawKey == "@graph")
        {
          // Each element is a top-level node — recurse with empty prefix
          if (json[pos] == '[')
            {
              ++pos; // consume '['
              while (true)
                {
                  SkipWhitespace(json, pos);
                  if (!json[pos] || json[pos] == ']') { if (json[pos]) ++pos; break; }
                  if (json[pos] == '{')
                    ParseObject(json, pos, STRING(), depth + 1, record, base);
                  else
                    while (json[pos] && json[pos] != ',' && json[pos] != ']') ++pos;
                  SkipWhitespace(json, pos);
                  if (json[pos] == ',') ++pos;
                }
            }
          else if (json[pos] == '{')
            {
              ParseObject(json, pos, STRING(), depth + 1, record, base);
            }
          SkipWhitespace(json, pos);
          if (json[pos] == ',') ++pos;
          continue;
        }

      if (rawKey == "@nest")
        {
          // Transparent nesting — recurse with same prefix
          if (json[pos] == '{')
            ParseObject(json, pos, prefix, depth + 1, record, base);
          else
            while (json[pos] && json[pos] != ',' && json[pos] != '}') ++pos;
          SkipWhitespace(json, pos);
          if (json[pos] == ',') ++pos;
          continue;
        }

      if (rawKey == "@set" || rawKey == "@list")
        {
          // Treat as array under the current prefix
          if (json[pos] == '[')
            ParseArray(json, pos, prefix, depth + 1, record, base);
          else
            while (json[pos] && json[pos] != ',' && json[pos] != '}') ++pos;
          SkipWhitespace(json, pos);
          if (json[pos] == ',') ++pos;
          continue;
        }

      // Other @keywords — look up in table; skip if mapped to ""
      if (rawKey.GetChr(1) == '@')
        {
          const char *mapped = LookupKeyword(rawKey.c_str());
          if (mapped != nullptr && mapped[0] == '\0')
            {
              // explicitly skip
              while (json[pos] && json[pos] != ',' && json[pos] != '}') ++pos;
              SkipWhitespace(json, pos);
              if (json[pos] == ',') ++pos;
              continue;
            }
          // mapped to a real name — fall through with remapped key
          if (mapped && mapped[0])
            rawKey = STRING(mapped);
        }

      // ---- normal term: resolve via UnifiedNames then parse value ----

      // Apply UnifiedNames mapping
      STRING mappedKey;
      {
        STRLIST mappedList;
        INT res = UnifiedNames(rawKey, &mappedList);
        if (res == 0 || mappedList.IsEmpty())
          {
            // Skip this field
            while (json[pos] && json[pos] != ',' && json[pos] != '}') ++pos;
            SkipWhitespace(json, pos);
            if (json[pos] == ',') ++pos;
            continue;
          }
        mappedKey = mappedList.GetEntry(1);
      }

      // Build full path
      STRING fullKey;
      if (prefix.GetLength() > 0)
        {
          fullKey  = prefix;
          fullKey += m_PathSep;
          fullKey += mappedKey;
        }
      else
        {
          fullKey = mappedKey;
        }

      ParseValue(json, pos, fullKey, depth + 1, record, base);

      SkipWhitespace(json, pos);
      if (json[pos] == ',') ++pos;
    }
}

// ---------------------------------------------------------------------------
// ParseFields
//
// Pre-clears the per-document context map, then delegates to the base
// class ParseFields which will call our overridden ParseObject.
// ---------------------------------------------------------------------------

void JSONLDDOC::ParseFields(PRECORD NewRecord)
{
  // Clear per-document context maps — each document has its own @context
  m_ContextTerms.Clear();
  m_ContextValues.Clear();
  m_PrefixTerms.Clear();
  m_PrefixValues.Clear();

  // The base class opens the file, reads the buffer, and calls ParseObject
  // (which we've overridden) for the top-level value.
  JSONDOC::ParseFields(NewRecord);
}
