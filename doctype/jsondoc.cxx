/*-@@@
File:           jsondoc.cxx
Version:        1.02
Description:    Class JSONDOC - JSON Document Type
Author:         Based on COLONDOC by Edward C. Zimmermann
Copyright:      Copyright (c) 2024 re-Isearch Project
                Licensed under the Apache 2.0 license

Core principle
==============
re-Isearch is coordinate-based.  We do NOT store field values in the
index.  For every JSON leaf value we record:

   fieldname  →  FC { start, end }

where start/end are absolute byte offsets into the original ingested
file pointing at the value content (for strings: the bytes INSIDE the
quotes; for primitives: the literal bytes).  The engine fetches the
actual text directly from the file at retrieval time, exactly as
COLONDOC does for the text following a colon tag.
@@@-*/

#include "jsondoc.hxx"
#include "common.hxx"
#include "doc_conf.hxx"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

JSONDOC::JSONDOC(PIDBOBJ DbParent, const STRING& Name)
  : COLONDOC(DbParent, Name),
    m_PathSep(JSON_PATH_SEP),
    m_IndexArrayElements(true),
    m_AutoFieldTypes(true)
{
  STRING opt( Getoption("PathSep") );
  if (opt.GetLength() == 1)
    m_PathSep = opt.GetChr(1);

  m_IndexArrayElements = Getoption("IndexArrayElements", "true").GetBool();
  m_AutoFieldTypes     = Getoption("AutodetectFieldtypes", "Y").GetBool();
}

JSONDOC::~JSONDOC() {}

// ---------------------------------------------------------------------------
// Description / MIME
// ---------------------------------------------------------------------------

const char *JSONDOC::Description(PSTRLIST List) const
{
  if (List) {
    const STRING ThisDoctype("JSON");
    if (Doctype != ThisDoctype && List->IsEmpty())
      List->AddEntry(Doctype);
    List->AddEntry (ThisDoctype);
    COLONDOC::Description(List);
  }

  return "JSON Document Type – indexes key/value pairs by field coordinates; "
         "nested keys joined with path-separator (default '|').\n\
Indexing options (defined in .ini):\n\
  [General]\n\
  AutodetectFieldtypes=True|False // Guess fieldtypes\n\
  IndexArrayElements=True|False\n\
  PathSep=Character";
}

void JSONDOC::SourceMIMEContent(PSTRING StringPtr) const
{
  if (StringPtr)
    *StringPtr = "application/json";
}

// ---------------------------------------------------------------------------
// ParseRecords – delegate to COLONDOC (handles file/record boundaries)
// ---------------------------------------------------------------------------

void JSONDOC::ParseRecords(const RECORD& FileRecord)
{
  COLONDOC::ParseRecords(FileRecord);
}

// ---------------------------------------------------------------------------
// ParseFields
// ---------------------------------------------------------------------------

void JSONDOC::ParseFields(PRECORD NewRecord)
{
  if (!NewRecord)
    return;

  STRING FileName;
  NewRecord->GetFullFileName(&FileName);

  // base = absolute file offset of the first byte of this record.
  // All FC values are base + buffer-relative-offset, so they point
  // correctly into the original file regardless of multi-record layout.
  GPTYPE base = NewRecord->GetRecordStart();
  GPTYPE recEnd = NewRecord->GetRecordEnd();
  size_t recLen = (size_t)(recEnd - base + 1);

  PFILE fp = fopen(FileName.c_str(), "rb");
  if (!fp) {
    message_log(LOG_ERROR, "JSONDOC::ParseFields: cannot open '%s'",
                FileName.c_str());
    return;
  }

  fseek(fp, (long)base, SEEK_SET);
  char *buf = new char[recLen + 1];
  size_t nRead = fread(buf, 1, recLen, fp);
  fclose(fp);
  buf[nRead] = '\0';

  size_t pos = 0;
  SkipWhitespace(buf, pos);

  if (buf[pos] == '{') {
    STRING emptyPrefix;
    ParseObject(buf, pos, emptyPrefix, 0, NewRecord, base);
  } else if (buf[pos] == '[') {
    STRING rootKey("_root_");
    ParseArray(buf, pos, rootKey, 0, NewRecord, base);
  } else {
    message_log(LOG_WARN,
                "JSONDOC: '%s' offset %ld does not start with '{' or '[', "
                "falling back to COLONDOC parser",
                FileName.c_str(), (long)base);
    delete[] buf;
    COLONDOC::ParseFields(NewRecord);
    return;
  }

  delete[] buf;
}

// ---------------------------------------------------------------------------
// ParseObject  { "key" : value , … }
// ---------------------------------------------------------------------------

void JSONDOC::ParseObject(const char *json, size_t& pos,
                          const STRING& prefix, int depth,
                          PRECORD record, GPTYPE base)
{
  if (depth > JSON_MAX_DEPTH) {
    message_log(LOG_WARN, "JSONDOC: max nesting depth exceeded – skipping subtree");
    int brace = 1;
    ++pos; // skip opening '{'
    while (json[pos] && brace > 0) {
      // NOTE: a robust implementation would respect strings here;
      // for typical document content this is sufficient.
      if      (json[pos] == '{') ++brace;
      else if (json[pos] == '}') --brace;
      ++pos;
    }
    return;
  }

  ++pos; // consume '{'

  while (true) {
    SkipWhitespace(json, pos);

    if (!json[pos] || json[pos] == '}') {
      if (json[pos] == '}') ++pos;
      break;
    }

    if (json[pos] != '"') {
      // malformed – skip to next separator
      while (json[pos] && json[pos] != ',' && json[pos] != '}')
        ++pos;
      if (json[pos] == ',') ++pos;
      continue;
    }

    // Parse key – we need the content but not its FC
    size_t kStart, kEnd;
    SkipString(json, pos, kStart, kEnd);
    STRING key;
    for (size_t i = kStart; i <= kEnd; ++i)
      key += json[i];

    // Build full dotted/piped path
    STRING fullKey;
    if (prefix.GetLength() > 0) {
      fullKey  = prefix;
      fullKey += m_PathSep;
      fullKey += key;
    } else {
      fullKey = key;
    }

    SkipWhitespace(json, pos);
    if (json[pos] == ':') ++pos;
    SkipWhitespace(json, pos);

    ParseValue(json, pos, fullKey, depth + 1, record, base);

    SkipWhitespace(json, pos);
    if (json[pos] == ',') ++pos;
  }
}

// ---------------------------------------------------------------------------
// ParseArray  [ value , … ]
// ---------------------------------------------------------------------------

void JSONDOC::ParseArray(const char *json, size_t& pos,
                         const STRING& prefix, int depth,
                         PRECORD record, GPTYPE base)
{
  ++pos; // consume '['
  int index = 0;

  while (true) {
    SkipWhitespace(json, pos);
    if (!json[pos] || json[pos] == ']') {
      if (json[pos] == ']') ++pos;
      break;
    }

    if (m_IndexArrayElements) {
      char indexBuf[32];
      snprintf(indexBuf, sizeof(indexBuf), "%d", index);
      STRING elemKey = prefix;
      elemKey += m_PathSep;
      elemKey += indexBuf;
      ParseValue(json, pos, elemKey, depth + 1, record, base);
    } else {
      // All elements indexed under the parent key
      ParseValue(json, pos, prefix, depth + 1, record, base);
    }

    ++index;
    SkipWhitespace(json, pos);
    if (json[pos] == ',') ++pos;
  }
}

// ---------------------------------------------------------------------------
// ParseValue  – dispatch on first character
// ---------------------------------------------------------------------------

void JSONDOC::ParseValue(const char *json, size_t& pos,
                         const STRING& prefix, int depth,
                         PRECORD record, GPTYPE base)
{
  SkipWhitespace(json, pos);
  char c = json[pos];

  if (c == '{') {
    // Nested object: recurse; no FC for the container itself, only leaves
    ParseObject(json, pos, prefix, depth, record, base);

  } else if (c == '[') {
    ParseArray(json, pos, prefix, depth, record, base);

  } else if (c == '"') {
    // FC covers the content bytes INSIDE the quotes, mirroring
    // how COLONDOC points at the text after the colon/whitespace.
    size_t contentStart, contentEnd;
    SkipString(json, pos, contentStart, contentEnd);
    if (contentEnd >= contentStart) {
      STRING contents;
      for (size_t i = contentStart; i <= contentEnd; ++i)
        contents += json[i];
      AddField(record,
               SanitiseFieldName(prefix),
               (GPTYPE)(base + contentStart),
               (GPTYPE)(base + contentEnd),
               contents);
    }

  } else {
    // Number / bool / null
    size_t valStart, valEnd;
    SkipPrimitive(json, pos, valStart, valEnd);
    if (valEnd >= valStart) {
      // Skip null – don't index the word "null" as a field value
      size_t len = valEnd - valStart + 1;
      bool isNull = (len == 4 &&
                     json[valStart]   == 'n' && json[valStart+1] == 'u' &&
                     json[valStart+2] == 'l' && json[valStart+3] == 'l');
      if (!isNull) {
        STRING contents;
        for (size_t i = valStart; i <= valEnd; ++i)
          contents += json[i];
        AddField(record,
                 SanitiseFieldName(prefix),
                 (GPTYPE)(base + valStart),
                 (GPTYPE)(base + valEnd),
                 contents);
      }
    }
  }
}

// ---------------------------------------------------------------------------
// SkipString
//
// pos must be on the opening '"'.
// After return pos is past the closing '"'.
// contentStart/contentEnd are buffer-relative indices of the first and
// last bytes of the string content (INSIDE the quotes).
// For an empty string, contentEnd < contentStart.
// ---------------------------------------------------------------------------

void JSONDOC::SkipString(const char *json, size_t& pos,
                         size_t& contentStart, size_t& contentEnd)
{
  ++pos; // skip opening '"'
  contentStart = pos;

  while (json[pos] && json[pos] != '"') {
    if (json[pos] == '\\') {
      ++pos; // skip escape prefix
      if (json[pos] == 'u') pos += 4; // skip 4 hex digits of \uXXXX
    }
    ++pos;
  }

  contentEnd = (pos > contentStart) ? pos - 1 : contentStart - 1;

  if (json[pos] == '"') ++pos; // skip closing '"'
}

// ---------------------------------------------------------------------------
// SkipPrimitive  (number / bool / null)
// ---------------------------------------------------------------------------

void JSONDOC::SkipPrimitive(const char *json, size_t& pos,
                             size_t& valueStart, size_t& valueEnd)
{
  valueStart = pos;
  while (json[pos] &&
         json[pos] != ',' && json[pos] != '}' &&
         json[pos] != ']' && !isspace((unsigned char)json[pos])) {
    ++pos;
  }
  valueEnd = (pos > valueStart) ? pos - 1 : valueStart - 1;
}

// ---------------------------------------------------------------------------
// SkipWhitespace  (tolerates // and /* */ comments found in JSON5/configs)
// ---------------------------------------------------------------------------

void JSONDOC::SkipWhitespace(const char *json, size_t& pos)
{
  while (json[pos]) {
    if (isspace((unsigned char)json[pos])) {
      ++pos;
    } else if (json[pos] == '/' && json[pos+1] == '/') {
      while (json[pos] && json[pos] != '\n') ++pos;
    } else if (json[pos] == '/' && json[pos+1] == '*') {
      pos += 2;
      while (json[pos] && !(json[pos] == '*' && json[pos+1] == '/')) ++pos;
      if (json[pos]) pos += 2;
    } else {
      break;
    }
  }
}

// ---------------------------------------------------------------------------
// SanitiseFieldName
// ---------------------------------------------------------------------------

STRING JSONDOC::SanitiseFieldName(const STRING& raw) const
{
  STRING safe;
  for (size_t i = 1; i <= (size_t)raw.GetLength(); ++i) {
    char ch = raw.GetChr(i);
    if (isalnum((unsigned char)ch) || ch == '_' ||
        ch == '-' || ch == '.' || ch == m_PathSep)
      safe += ch;
    else
      safe += '_';
  }
  return safe;
}

// ---------------------------------------------------------------------------
// AddField
//
// Registers fieldname + FC with the engine.  No value is stored in the
// index — the engine reads bytes [start, end] from the original file at
// query time.
//
// 'contents' is the transient string of those same bytes, used solely
// for field-type autodetection via GuessFieldType (inherited from
// METADOC).  It is never written to the index.
//
// FCT::AddEntry accepts:
//   AddEntry(const FC&     fc)
//   AddEntry(const FCLIST& fct)
//   AddEntry(const FCLIST* fctPtr)
// We use the FC& overload since we have exactly one coordinate range
// per leaf value.
// ---------------------------------------------------------------------------

void JSONDOC::AddField(PRECORD record,
                       const STRING& fieldname,
                       GPTYPE start, GPTYPE end,
                       const STRING& contents)
{
  if (!record || fieldname.IsEmpty() || end < start)
    return;

  FC fc;
  fc.SetFieldStart(start);
  fc.SetFieldEnd  (end);

  FCT fct;
  fct.AddEntry(fc);          // FC& overload

  DF df;
  df.SetFieldName(fieldname);
  df.SetFct(fct);

  if (Db) {
    DFD dfd;
    // Autodetect field type from the transient content string. 
    // GuessFieldType calls Db->AddFieldType internally when it makes a
    // determination, so we only need to call it — we don't act on the
    // return value here.
    FIELDTYPE ft;

    if (m_AutoFieldTypes && contents.GetLength() > 0)
      ft = GuessFieldType(fieldname, contents);
    dfd.SetFieldName(fieldname);
    if (ft.Defined()) dfd.SetFieldType( ft ); // Set the type
    Db->DfdtAddEntry(dfd);  // register field name globally (idempotent)
  }

  // preserve fields already added for this record
  record->AddEntry(df);
}
