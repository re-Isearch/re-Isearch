/*-@@@
File:           ejsondoc.cxx
Version:        1.01
Description:    Class EJSONDOC - MongoDB Extended JSON / Extended JSON Document Type
Author:         Based on JSONDOC by Edward C. Zimmermann
Copyright:      Copyright (c) 2026 re-Isearch Project
                Licensed under the Apache 2.0 license
@@@-*/

#include "ejsondoc.hxx"
#include "common.hxx"
#include "doc_conf.hxx"

// ---------------------------------------------------------------------------
// MongoDB-specific alias table
//
// Maps MongoDB Extended JSON '$' type names that don't directly match any
// re-Isearch FIELDTYPE name or alias to the correct re-Isearch name.
// Sorted for bsearch.  Only names NOT already handled by FIELDTYPE(name)
// need to appear here.
// ---------------------------------------------------------------------------

// MongoDB OID is:
// 507f1f77bcf86cd799439011
//└──────┘└──────┘└──┘└──┘
// 4B time  5B rand 3B inc
//
// This is quite different from re-Isearch's dotnumber which are derived from
// IP addresses. We also have the OIDs in ISO23950/Z39.50.
// 
// Since Mongo's OIDs won't fit in a long double and we really don't have
// any use beyond matching we map this to casehash. The 4 bytes for
// time are also not interesting to search independently.

struct MongoAlias { const char *mongo; const char *reisearch; };
static const MongoAlias MongoAliases[] = {
  /* SORTED */
  { "bool",          "boolean"   },
  { "code",          "string"    },  // JavaScript code — index as text
  { "numberDecimal", "numerical" },
  { "numberDouble",  "numerical" },
  { "numberInt",     "numerical" },
  { "numberLong",    "numerical" },
  { "oid",           "casehash"  },  // 24-hex ObjectId string
  { "regex",         "string"    },  // pattern — index as text
  { "symbol",        "string"    },
  { "timestamp",     "time"      },  // { "t": sec, "i": inc } → use seconds
};
static const int MongoAliasCount =
  (int)(sizeof(MongoAliases) / sizeof(MongoAliases[0]));

// Known types that carry no indexable text — silently skip them.
static const char * const SkipTypes[] = {
  /* SORTED */
  "binary", "dbPointer", "maxKey", "minKey", "undefined",
};
static const int SkipTypeCount =
  (int)(sizeof(SkipTypes) / sizeof(SkipTypes[0]));

static int CmpCStr(const void *a, const void *b)
{
  return strcmp((const char *)a, *(const char * const *)b);
}

static int CmpMongoAlias(const void *a, const void *b)
{
  return strcmp((const char *)a,
                ((const MongoAlias *)b)->mongo);
}

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

EJSONDOC::EJSONDOC(PIDBOBJ DbParent, const STRING& Name)
  : JSONDOC(DbParent, Name)
{
  // All options inherited from JSONDOC
}

EJSONDOC::~EJSONDOC() {}

// ---------------------------------------------------------------------------
// Description / MIME
// ---------------------------------------------------------------------------

const char *EJSONDOC::Description(PSTRLIST List) const
{
  if (List) {
    const STRING ThisDoctype("EJSON");
    if (Doctype != ThisDoctype && List->IsEmpty())
      List->AddEntry(Doctype);
    List->AddEntry(ThisDoctype);
    JSONDOC::Description(List);
  }

#ifdef VECTOR_INDEX
  return "Extended JSON Document Type (MongoDB EJSON v1/v2 + re-Isearch $type convention).\n\
{ \"$typename\": value } wrappers set the field type explicitly.\n\
$typename is looked up as a re-Isearch FIELDTYPE name (all aliases supported).\n\
MongoDB-specific names ($numberLong/$oid/$bool/$timestamp etc.) also handled.\n\
MongoDB Examples:\n\
  { \"box\":  { \"$box\":       \"51.5,-0.1,52,0.1\" } }  -> box\n\
  { \"loc\":  { \"$box\":       \"[-74.0,40.7],[-73.9,40.8]\" } } -> box (re-written)\n\
  { \"name\": { \"$metaphone\": \"Smith\"            } }  -> metaphone\n\
  { \"id\":   { \"$oid\":       \"507f1f77...\"      } }  -> casehash\n\
Native examples:\n\
  { \"date\":  { \"$date\":     \"Mon May  4 10:33:00 CEST 2026\" } } -> date\n\
  { \"contents\": { \"$hnsw\":  \"Caesar, and companion me with my mistress.\" } } -> hnsw\n\
We also support hex, base64 and JSON vectors for hnsw but their dimension must match model.\n\
For a list of native built-in datatypes: quarry index -help types  (command line indexer)\n\
Options: see JSON.";
#else
  return "Extended JSON Document Type (MongoDB EJSON v1/v2 + re-Isearch $type convention).\n\
{ \"$typename\": value } wrappers set the field type explicitly.\n\
$typename is looked up as a re-Isearch FIELDTYPE name (all aliases supported).\n\
MongoDB-specific names ($numberLong/$oid/$bool/$timestamp etc.) also handled.\n\
MongoDB Examples:\n\
  { \"box\":  { \"$box\":       \"51.5,-0.1,52,0.1\" } }  -> box\n\
  { \"loc\":  { \"$box\":       \"[-74.0,40.7],[-73.9,40.8]\" } } -> box (re-written)\n\
  { \"name\": { \"$metaphone\": \"Smith\"            } }  -> metaphone\n\
  { \"id\":   { \"$oid\":       \"507f1f77...\"      } }  -> casehash\n\
Native examples:\n\
  { \"date\":  { \"$date\":     \"Mon May  4 10:33:00 CEST 2026\" } } -> date\n\
For a list of native built-in datatypes: quarry index -help types  (command line indexer)\n\
Options: see JSON.";
#endif
}

void EJSONDOC::SourceMIMEContent(PSTRING StringPtr) const
{
  if (StringPtr)
    *StringPtr = "application/json";  // no dedicated MIME type for EJSON
}

// ---------------------------------------------------------------------------
// IsEJsonWrapper
//
// Lightweight lookahead — does NOT advance pos.
//
// Returns true if the object at pos is a single-key wrapper { "$x": v }.
// Resolution order:
//   1. Strip '$'; call FIELDTYPE(name) — covers all re-Isearch types + aliases
//   2. MongoAliases table  — MongoDB-specific names
//   3. SkipTypes list      — known but non-indexable; sets skip=true
//   4. Unknown '$' key     — return false (treat as normal nested object)
// ---------------------------------------------------------------------------

bool EJSONDOC::IsEJsonWrapper(const char *json, size_t pos,
                               FIELDTYPE& ft, bool& skip) const
{
  skip = false;
  ft   = FIELDTYPE();   // undefined

  if (json[pos] != '{') return false;

  // Scan for the first '"' key
  size_t p = pos + 1;
  while (json[p] && isspace((unsigned char)json[p])) ++p;
  if (json[p] != '"') return false;

  ++p; // skip opening '"'
  size_t kStart = p;
  while (json[p] && json[p] != '"')
    { if (json[p] == '\\') ++p; ++p; }
  size_t kLen = p - kStart;
  if (json[p] == '"') ++p;

  // Must start with '$'
  if (kLen < 2 || json[kStart] != '$') return false;

  // Copy name without '$'
  char nameBuf[128];
  size_t nameLen = kLen - 1;
  if (nameLen >= sizeof(nameBuf)) return false;
  memcpy(nameBuf, json + kStart + 1, nameLen);
  nameBuf[nameLen] = '\0';

  // 1. Direct FIELDTYPE lookup via GetType() — handles all re-Isearch
  //    types and aliases with no side effects and no error logging.
  //    Returns FIELDTYPE::unknown for unrecognised names.
  //    Covers: $date, $box, $iban, $metaphone, $xs:dateTime, etc.

  if ( FIELDTYPE::GetType(STRING(nameBuf)) != FIELDTYPE::unknown)
    return true;

  // 2. MongoDB-specific alias table — names that don't match any
  //    re-Isearch FIELDTYPE name ($numberLong, $oid, $bool, etc.)
  {
    const MongoAlias *alias =
      (const MongoAlias *)bsearch(
        nameBuf, MongoAliases, MongoAliasCount,
        sizeof(MongoAliases[0]), CmpMongoAlias);
    if (alias)
      {
	if ( FIELDTYPE::GetType(STRING(alias->reisearch)) !=  FIELDTYPE::unknown)
	  return true;
      }
  }

  // 3. Skip types — known MongoDB types with no indexable text content
  if (bsearch(nameBuf, SkipTypes, SkipTypeCount, sizeof(SkipTypes[0]), CmpCStr))
    {
      skip = true;
      return true;
    }

  // Unknown '$' key — not an EJSON wrapper we recognise.
  // Fall through to normal nested object parsing so no data is lost.
  return false;
}

// ---------------------------------------------------------------------------
// ForceFieldType
//
// Writes an explicit FIELDTYPE for fieldname into the DFDT.
// Called after AddField has registered the FC, to override whatever
// GuessFieldType may have set.
//
// Note: GuessFieldType is already reliable for many cases ($date with ISO
// string, $numberLong with digit string).  ForceFieldType is most important
// for types that look like plain text to autodetection: $oid (hex string →
// would be 'text', should be 'dotnumber'), $bool (true/false → no bool path
// in GuessFieldType), $timestamp (integer → 'numerical', should be 'time'),
// $metaphone, $box, $iban, etc.  Calling it unconditionally is correct and
// harmless for all cases.
// ---------------------------------------------------------------------------

void EJSONDOC::ForceFieldType(const STRING& fieldname, const FIELDTYPE& ft)
{
  if (!Db || !ft.Defined()) return;
  DFD dfd;
  dfd.SetFieldName(fieldname);
  dfd.SetFieldType(ft);
  Db->DfdtAddEntry(dfd);
}

// ---------------------------------------------------------------------------
// HandleEJsonWrapper
//
// pos is on the opening '{' of the wrapper; advances past closing '}'.
//
// Structural special cases:
//   time (from $timestamp): { "t": <sec>, "i": <inc> } — extract "t"
//   nested object value:    { "$numberLong": "<ms>" }  — extract inner value
//   string value:           "$date" v2, $oid, $regex, $code, …
//   primitive value:        $numberInt, $bool true/false, …
// ---------------------------------------------------------------------------

void EJSONDOC::HandleEJsonWrapper(const char *json, size_t& pos,
                                   const STRING& fieldname,
                                   const FIELDTYPE& ft,
                                   PRECORD record, GPTYPE base)
{
  ++pos; // consume '{'
  SkipWhitespace(json, pos);

  // Skip the $key — type already resolved
  if (json[pos] == '"')
    {
      size_t kStart, kEnd;
      SkipString(json, pos, kStart, kEnd);
      (void)kStart; (void)kEnd;
    }

  SkipWhitespace(json, pos);
  if (json[pos] == ':') ++pos;
  SkipWhitespace(json, pos);

  if (ft == FIELDTYPE::time && json[pos] == '{')
    {
      // $timestamp: { "t": <seconds>, "i": <ordinal> } — index "t" only
      ++pos;
      size_t tStart = (size_t)-1, tEnd = 0;
      STRING tContents;
      while (true)
        {
          SkipWhitespace(json, pos);
          if (!json[pos] || json[pos] == '}') { if (json[pos]) ++pos; break; }
          if (json[pos] != '"')
            {
              while (json[pos] && json[pos]!=',' && json[pos]!='}') ++pos;
              if (json[pos]==',') ++pos;
              continue;
            }
          size_t kStart, kEnd;
          SkipString(json, pos, kStart, kEnd);
          STRING key;
          for (size_t i = kStart; i <= kEnd; ++i) key += json[i];
          SkipWhitespace(json, pos);
          if (json[pos] == ':') ++pos;
          SkipWhitespace(json, pos);
          if (key == "t")
            {
              SkipPrimitive(json, pos, tStart, tEnd);
              for (size_t i = tStart; i <= tEnd; ++i) tContents += json[i];
            }
          else
            { while (json[pos] && json[pos]!=',' && json[pos]!='}') ++pos; }
          SkipWhitespace(json, pos);
          if (json[pos] == ',') ++pos;
        }
      if (tStart != (size_t)-1 && !fieldname.IsEmpty())
        {
          AddField(record, fieldname,
                   (GPTYPE)(base + tStart), (GPTYPE)(base + tEnd),
                   tContents);
          ForceFieldType(fieldname, ft);
        }
    }
  else if (json[pos] == '{')
    {
      // Nested object value — e.g. EJSON v1 $date: { "$numberLong": "<ms>" }
      // Extract the inner string or primitive.
      ++pos; // consume inner '{'
      SkipWhitespace(json, pos);
      // skip inner key
      if (json[pos] == '"')
        {
          size_t kStart, kEnd;
          SkipString(json, pos, kStart, kEnd);
          SkipWhitespace(json, pos);
          if (json[pos] == ':') ++pos;
          SkipWhitespace(json, pos);
        }
      size_t vStart = (size_t)-1, vEnd = 0;
      STRING contents;
      if (json[pos] == '"')
        {
          SkipString(json, pos, vStart, vEnd);
          for (size_t i = vStart; i <= vEnd; ++i) contents += json[i];
        }
      else
        {
          SkipPrimitive(json, pos, vStart, vEnd);
          for (size_t i = vStart; i <= vEnd; ++i) contents += json[i];
        }
      // skip to and consume inner '}'
      while (json[pos] && json[pos] != '}') ++pos;
      if (json[pos] == '}') ++pos;

      if (vStart != (size_t)-1 && !fieldname.IsEmpty())
        {
          AddField(record, fieldname,
                   (GPTYPE)(base + vStart), (GPTYPE)(base + vEnd),
                   contents);
          ForceFieldType(fieldname, ft);
        }
    }
  else if (json[pos] == '"')
    {
      // String value: ISO date, OID hex, regex pattern, code, symbol, …
      size_t vStart, vEnd;
      SkipString(json, pos, vStart, vEnd);
      if (vEnd >= vStart && !fieldname.IsEmpty())
        {
          STRING contents;
          for (size_t i = vStart; i <= vEnd; ++i) contents += json[i];
          AddField(record, fieldname,
                   (GPTYPE)(base + vStart), (GPTYPE)(base + vEnd),
                   contents);
          ForceFieldType(fieldname, ft);
        }
    }
  else
    {
      // Primitive: $numberInt, $numberLong, $numberDouble, $bool true/false
      size_t vStart, vEnd;
      SkipPrimitive(json, pos, vStart, vEnd);
      if (vEnd >= vStart && !fieldname.IsEmpty())
        {
          STRING contents;
          for (size_t i = vStart; i <= vEnd; ++i) contents += json[i];
          AddField(record, fieldname,
                   (GPTYPE)(base + vStart), (GPTYPE)(base + vEnd),
                   contents);
          ForceFieldType(fieldname, ft);
        }
    }

  // Consume the closing '}' of the wrapper object
  SkipWhitespace(json, pos);
  if (json[pos] == '}') ++pos;
}

// ---------------------------------------------------------------------------
// ParseObject  (override)
//
// For each key-value pair: if the value is a { "$type": ... } wrapper,
// resolve the type and call HandleEJsonWrapper.
// Otherwise fall through to the inherited ParseValue.
// ---------------------------------------------------------------------------

void EJSONDOC::ParseObject(const char *json, size_t& pos,
                            const STRING& prefix, int depth,
                            PRECORD record, GPTYPE base)
{
  if (depth > JSON_MAX_DEPTH)
    {
      message_log(LOG_WARN, "EJSONDOC: max nesting depth exceeded");
      int b = 1; ++pos;
      while (json[pos] && b > 0)
        { if (json[pos]=='{') b++; else if (json[pos]=='}') b--; ++pos; }
      return;
    }

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

      // Read key
      size_t kStart, kEnd;
      SkipString(json, pos, kStart, kEnd);
      STRING key;
      for (size_t i = kStart; i <= kEnd; ++i) key += json[i];

      // Build full field path
      STRING fullKey;
      if (prefix.GetLength() > 0)
        { fullKey = prefix; fullKey += m_PathSep; fullKey += key; }
      else
        fullKey = key;

      SkipWhitespace(json, pos);
      if (json[pos] == ':') ++pos;
      SkipWhitespace(json, pos);

      // Check if value is a { "$type": ... } wrapper
      if (json[pos] == '{')
        {
          FIELDTYPE ft;
          bool skip = false;
          if (IsEJsonWrapper(json, pos, ft, skip))
            {
              if (skip)
                {
                  // Non-indexable type — skip the entire wrapper object
                  int d = 1; ++pos;
                  while (json[pos] && d > 0)
                    {
                      if      (json[pos]=='{' || json[pos]=='[') d++;
                      else if (json[pos]=='}' || json[pos]==']') d--;
                      ++pos;
                    }
                }
              else
                {
                  HandleEJsonWrapper(json, pos,
                                     SanitiseFieldName(fullKey),
                                     ft, record, base);
                }
              SkipWhitespace(json, pos);
              if (json[pos] == ',') ++pos;
              continue;
            }
          // Unrecognised '$' key or normal nested object — fall through
        }

      // Normal value — parse via inherited ParseValue
      ParseValue(json, pos, fullKey, depth + 1, record, base);

      SkipWhitespace(json, pos);
      if (json[pos] == ',') ++pos;
    }
}


// Parse Buffer, fieldname, fld

#if 1

int EJSONDOC::ParseBBox(const STRING& Buffer, const STRING& FieldName, BBOXFLD *fld) const
{
  STRING s( Buffer);
  s.Trim(STRING::both);

  if (s.GetChr(1) == '[')
    {
      // MongoDB format: [[west,south],[east,north]]
      // Parse robustly — extract exactly 4 numbers in order,
      // ignoring all brackets, commas and whitespace.
      double coords[4];
      int    found = 0;
      const char *p = s.c_str();

      while (*p && found < 4)
        {
          // Skip anything that isn't the start of a number
          while (*p && !isdigit((unsigned char)*p) &&
                 *p != '-' && *p != '+' && *p != '.')
            ++p;
          if (!*p) break;

          char *end = nullptr;
          double v = strtod(p, &end);
          if (end == p) break;  // no progress — malformed
          coords[found++] = v;
          p = end;
        }

      if (found == 4)
        {
          // MongoDB: [x1,y1],[x2,y2] = [lon,lat],[lon,lat]
          // Normalise to N W S E regardless of corner order
          double north = std::max(coords[1], coords[3]);
          double south = std::min(coords[1], coords[3]);
          double west  = std::min(coords[0], coords[2]);
          double east  = std::max(coords[0], coords[2]);

          s.sprintf("%.10g %.10g %.10g %.10g", north, west, south, east);
        }
       else // Malformed — fall through
        message_log(LOG_WARN, "EJSONDOC::ParseBBox: could not parse '%s' as MongoDB box",
                  (const char *)s);
    }

  // re-Isearch native formats — base class handles all variants
  return DOCTYPE::ParseBBox(s, FieldName, fld);
}

#else
int EJSONDOC::ParseBBox(const STRING& Buffer, const STRING& FieldName, BBOXFLD *fld) const
{
  STRING s( Buffer );

  // Remove leading and trailing space..
  s.Trim(STRING::both);

  // Rwrite the buffer from Mongo's box to our box
  // Mongo: [west, south], [east, north]]
  // Ours:  [North, South, West, East]
  
  if (s.GetChr(1) == '[')
    {
      // MongoDB format: [[west,south],[east,north]] or [[x1,y1],[x2,y2]]
      double x1=0, y1=0, x2=0, y2=0;
      
      // Strip brackets and parse two coordinate pairs
      // Formats seen in the wild:
      //   [[W,S],[E,N]]
      //   [ [W, S], [E, N] ]
      //   [[x1,y1],[x2,y2]]  (min corner, max corner)
      if (sscanf(s.c_str(), "[[%lf,%lf],[%lf,%lf]]", &x1,&y1,&x2,&y2) == 4 ||
          sscanf(s.c_str(), "[ [%lf , %lf] , [%lf , %lf] ]", &x1,&y1,&x2,&y2) == 4)
        {
          // MongoDB uses [longitude, latitude] = [x, y] = [West/East, South/North]
          // Normalise to min/max regardless of which corner was passed first
          const double north = std::max(y1, y2);
          const double south = std::min(y1, y2);
          const double west  = std::min(x1, x2);
          const double east  = std::max(x1, x2);

	  // Rewrite as re-Isearch "N W S E" canonical string
	  s.sprintf( "%.10g %.10g %.10g %.10g", north, west, south, east);
        }
      // Malformed MongoDB box — fall through to base class which will fail gracefully
    }

  // re-Isearch native formats:
  //   "N W S E"          flat space-separated
  //   "N,W,S,E"          comma-separated  
  //   "N S W E"          alternate order (handled by base class)
  //   "51.5,-0.1,52,0.1" compact comma
  //   RECT{N,W,S,E}      query convention (base class handles)
  return DOCTYPE::ParseBBox(s, FieldName, fld);
}
#endif  


