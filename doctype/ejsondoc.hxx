/*-@@@
File:           ejsondoc.hxx
Version:        1.01
Description:    Class EJSONDOC - MongoDB Extended JSON / Extended JSON Document Type
Author:         Based on JSONDOC by Edward C. Zimmermann
Copyright:      Copyright (c) 2026 re-Isearch Project
                Licensed under the Apache 2.0 license

Notes:
  Extends JSONDOC to handle MongoDB Extended JSON (EJSON v1/v2) and a
  generalised "$type" convention for any re-Isearch FIELDTYPE.

  Any single-key object whose key starts with '$' is treated as a
  type-wrapper:

    { "field": { "$<typename>": <value> } }

  The '$' is stripped and the remainder is looked up as a registered
  re-Isearch FIELDTYPE name (including all aliases):

    { "name":    { "$metaphone":   "Smith"            } }  -> FIELDTYPE::metaphone
    { "address": { "$iban":        "GB29NWBK..."      } }  -> FIELDTYPE::iban
    { "loc":     { "$box":         "51.5,-0.1,52,0.1" } }  -> FIELDTYPE::box
    { "score":   { "$numberLong":  "9223372036854775807" } } -> FIELDTYPE::numerical
    { "created": { "$date":        "2024-03-20T10:00:00Z" } } -> FIELDTYPE::date

  MongoDB-specific names that don't match re-Isearch names are mapped
  via a small built-in alias table:
    $numberInt/$numberLong/$numberDouble/$numberDecimal -> numerical
    $oid       -> casehash // was dotnumber
    $bool      -> boolean
    $timestamp -> time      (uses the "t" seconds sub-field)
    $code/$symbol/$regex    -> string

  Types with no meaningful text representation are skipped:
    $binary, $dbPointer, $minKey, $maxKey, $undefined

  Doctype options (in .ini / IDB option string):
    PathSep=<char>              inherited from JSONDOC (default '|')
    IndexArrayElements=True     inherited from JSONDOC
    AutodetectFieldtypes=True   inherited from JSONDOC
@@@-*/

#ifndef EJSONDOC_HXX
#define EJSONDOC_HXX

#include "jsondoc.hxx"

class EJSONDOC : public JSONDOC {
public:
  EJSONDOC(PIDBOBJ DbParent, const STRING& Name);

  const char *Description(PSTRLIST List) const;
  void        SourceMIMEContent(PSTRING StringPtr) const;

  ~EJSONDOC();

  // Handle EJSON's box convention
  int ParseBBox(const STRING&, const STRING&, BBOXFLD*) const;

protected:
  // Override ParseObject to detect and unwrap { "$type": value } objects.
  void ParseObject(const char *json, size_t& pos,
                   const STRING& prefix, int depth,
                   PRECORD record, GPTYPE base);

private:
  // Lookahead: is the object at pos a { "$<type>": <value> } wrapper?
  // If yes, sets ft to the resolved FIELDTYPE and returns true.
  // If skip=true on return, the type is known but non-indexable.
  // Returns false if the '$' key is not recognised as any FIELDTYPE.
  bool IsEJsonWrapper(const char *json, size_t pos,
                      FIELDTYPE& ft, bool& skip) const;

  // Parse and index the value inside the wrapper.
  // pos is on opening '{'; advances past closing '}'.
  void HandleEJsonWrapper(const char *json, size_t& pos,
                          const STRING& fieldname,
                          const FIELDTYPE& ft,
                          PRECORD record, GPTYPE base);

  // Force an explicit FIELDTYPE for fieldname in the DFDT.
  void ForceFieldType(const STRING& fieldname, const FIELDTYPE& ft);
};

typedef EJSONDOC* PEJSONDOC;

#endif /* EJSONDOC_HXX */
