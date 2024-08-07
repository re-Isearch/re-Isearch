/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
/*-@@@
File:		attrlist.cxx
Version:	2.00
Description:	Class ATTRLIST - Attribute List
@@@*/

#include "attrlist.hxx"
#include "magic.hxx"

#pragma ident  "@(#)attrlist.cxx"

static const char *n = "n"; // Numeric
static const char *r = "r"; // Num Range
static const char *d = "d"; // Date
static const char *e = "e"; // Date Range
static const char *p = "p"; // Poly
static const char *b = "b"; // Box
static const char *h = "h"; // Phonetic hash
static const char *c = "c"; // 64-bit/Numeric hash
static const char *t = "t"; // Telephone number 
static const char *v = "iin"; // Credit Card Number (Visa etc.) = issuer identification number (IIN) 
static const char *i = "iban"; // IBAN (includes Checksum)
static const char *w = "w"; // Hierarchical Navigable Small Worlds (HNSW)
static const char *g = "g"; // Graph vector ANN algorithms (except HNSW)
static const char *f = "f"; // Vector Flat algorithms
static const char *_s = "";

extern "C" long double (*_IB_private_hash)(const char *, const char *, size_t );
extern "C" const char   *_IB_private_hash_descr;

// Data types: "text" (the default), "num", "date", "date-range" and "gpoly".
static struct DataType {
  const char         *Name;
  const char         *dtype;
  const BYTE          Type;
  const char         *Description;
} DataTypes[] = {
  {"any",       "", FIELDTYPE::any, "Any"},  // 2022 EXPERIMENTAL  
  {"string",    "", FIELDTYPE::text, "String (full text)"},
  {"numerical", n,  FIELDTYPE::numerical, "Numerical IEEE floating"},
  {"computed",  n,  FIELDTYPE::computed,  "Computed Numerical"},
  {"range",     r,  FIELDTYPE::numericalrange, "Range of Numerbers"},
  {"date",      d,  FIELDTYPE::date, "Date/Time in any of a large number of well defined formats"},
  {"date-range",e,  FIELDTYPE::daterange, "Range of Date as Start/End but also +N Seconds (to Years)"},
  {"gpoly",     p,  FIELDTYPE::gpoly, "Geospatial n-ary bounding coordinates"},
  {"box",       b,  FIELDTYPE::box,   "Geospatial bounding box coordinates (N,W,S,E)"},
  {"time",      d,  FIELDTYPE::time, "Numeric computed value for seconds since 1970, used as date."},
  {"ttl",       n,  FIELDTYPE::ttl, "Numeric computed value for time-to-live in seconds."},
  {"expires",   d,  FIELDTYPE::ttl_expires, "Numeric computed ttl value as date of expiration."},
  {"boolean",   "0",FIELDTYPE::boolean, "Boolean type"},
  {"currency",  "$",FIELDTYPE::currency, "Monetary currency"},
  {"dotnumber", n,  FIELDTYPE::dotnumber, "Dot number (Internet v4/v6 Addresses, UIDs etc)"},
  {"phonetic",  h,  FIELDTYPE::phonhash, "Computed phonetic hash applied to each word (for names)"},
  {"phone2",    h,  FIELDTYPE::phonhash2, "Phonetic hash applied to the whole field"},
  {"metaphone", h,  FIELDTYPE::metaphone, "Metaphone hash applied to each word (for names)"},
  {"metaphone2",h,  FIELDTYPE::metaphone2, "Metaphone hash (whole field)"},
  {"hash",      c,  FIELDTYPE::hash, "Computed 64-bit hash of field contents"},
  {"casehash",  c,  FIELDTYPE::casehash, "Computed case-independent hash of text field contents"},
  {"lexi",      c,  FIELDTYPE::lexi, "Computed case-independent lexical hash (first 8 characters)"},
  {"smiles",    c,  FIELDTYPE::smiles, "Computed SMILES (Chemical) hash // NOT YET"},
  {"privhash",  c,  FIELDTYPE::privhash, _IB_private_hash_descr ? _IB_private_hash_descr : "Undefined Private Hash (callback)"},
  {"isbn",      _s, FIELDTYPE::isbn, "ISBN: International Standard Book Number"},
  {"telnumber", t,  FIELDTYPE::telnumber, "ISO/CCITT/UIT Telephone Number"},
  {"iin", v, FIELDTYPE::creditcardnum, "Issuer Identification (Crediti/Debit Card) Number"},
  {"iban",       i, FIELDTYPE::iban, "IBAN: International Bank Account Number"},
  {"bic",       _s, FIELDTYPE::bic, "BIC : International Identifier Code (SWIFT)"},
  {"db_string", _s, FIELDTYPE::db_string, "External DB String (callback)"},
  {"callback",  _s, FIELDTYPE::callback,  "Local callback 0 (External)"},
  {"local1",    _s, FIELDTYPE::callback1, "Local callback 1 (External)"},
  {"local2",    _s, FIELDTYPE::callback2, "Local callback 2 (External)"},
  {"local3",    _s, FIELDTYPE::callback3, "Local callback 3 (External)"},
  {"local4",    _s, FIELDTYPE::callback4, "Local callback 4 (External)"},
  {"local5",    _s, FIELDTYPE::callback5, "Local callback 5 (External)"},
  {"local6",    _s, FIELDTYPE::callback6, "Local callback 6 (External)"},
  {"local7",    _s, FIELDTYPE::callback7, "Local callback 7 (External)"},
  {"hnsw",       w, FIELDTYPE::db_hnsw,   "Hierarchical Navigable Small Worlds (HNSW)"},
  {"nsg",        g, FIELDTYPE::db_nsg,    "Spread Out Graph ANNS algorithms (NSG)"}, 
  {"ivfflat",    f, FIELDTYPE::db_IVFFlat,"IVFFlat Vectors"},
  {"special",   _s, FIELDTYPE::special,   "Special text (reserved)"},
#define MAX_DATATYPE (int)(FIELDTYPE::special) 
  // Aliases
  {"dbm_string",    _s,  FIELDTYPE::db_string,     NULL},
  {"bdb_string",    _s,  FIELDTYPE::db_string,     NULL},
  {"gdb_string",    _s,  FIELDTYPE::db_string,     NULL},
  {"local0",        _s,  FIELDTYPE::callback,      NULL},
  {"callback0",     _s,  FIELDTYPE::callback,      NULL},
  {"callback1",     _s,  FIELDTYPE::callback1,     NULL},
  {"callback2",     _s,  FIELDTYPE::callback2,     NULL},
  {"callback3",     _s,  FIELDTYPE::callback3,     NULL},
  {"callback4",     _s,  FIELDTYPE::callback4,     NULL},
  {"callback5",     _s,  FIELDTYPE::callback5,     NULL},
  {"callback6",     _s,  FIELDTYPE::callback6,     NULL},
  {"callback7",     _s,  FIELDTYPE::callback7,     NULL},
  {"text",          _s,  FIELDTYPE::text,          NULL},
  {"num",            n,  FIELDTYPE::numerical,     NULL},
  {"number",         n,  FIELDTYPE::numerical,     NULL},
  {"num-range",      r,  FIELDTYPE::numericalrange,NULL},
  {"numrange",       r,  FIELDTYPE::numericalrange, NULL},
  {"numericalrange", r,  FIELDTYPE::numericalrange, NULL},
  {"numerical-range",r,  FIELDTYPE::numericalrange, NULL},
  {"daterange",      e,  FIELDTYPE::daterange,      NULL},
  {"duration",       e,  FIELDTYPE::daterange,      NULL},
  {"bounding-box",   b,  FIELDTYPE::box,            NULL},
  {"boundingbox",    b,  FIELDTYPE::box,            NULL},
  {"phonhash",       h,  FIELDTYPE::phonhash,       NULL},
  {"name",           h,  FIELDTYPE::metaphone,      NULL},
  {"lastname",       h,  FIELDTYPE::metaphone2,     NULL},
  {"hashcase",       c,  FIELDTYPE::casehash,       NULL},
  {"hash1",          c,  FIELDTYPE::privhash,       NULL},
  {"tel",            t,  FIELDTYPE::telnumber,      NULL},
  {"telnum",         t,  FIELDTYPE::telnumber,      NULL},
  {"phone",          t,  FIELDTYPE::telnumber,      NULL},
  {"telephone",      t,  FIELDTYPE::telnumber,      NULL},
  {"creditcard",     v,  FIELDTYPE::creditcardnum,  NULL},
//
  {"inet",           n,   FIELDTYPE::dotnumber,     NULL},
  {"ipv4",           n,   FIELDTYPE::dotnumber,     NULL},
  {"ipv6",           n,   FIELDTYPE::dotnumber,     NULL},
  {"flat",           f,   FIELDTYPE::db_IVFFlat,    NULL},
  {"sog",            g,   FIELDTYPE::db_nsg,        NULL},
  {"vector",         w,   FIELDTYPE::db_hnsw,        NULL},
// common xs: data type names
  {"xs:string",            _s,  FIELDTYPE::text,      NULL},
  // derived
  {"xs:normalizedString",  _s,  FIELDTYPE::text,      NULL},
  // integer types
  {"xs:boolean",            b,  FIELDTYPE::boolean,   NULL},
  {"xs:decimal",            n,  FIELDTYPE::numerical, NULL},
  {"xs:integer",            n,  FIELDTYPE::numerical, NULL},
  // derived integer types
  {"xs:long",               n,  FIELDTYPE::numerical, NULL},
  {"xs:int",                n,  FIELDTYPE::numerical, NULL},
  {"xs:short",              n,  FIELDTYPE::numerical, NULL},
  {"xs:unsignedLong",       n,  FIELDTYPE::numerical, NULL},
  {"xs:unsignedInt",        n,  FIELDTYPE::numerical, NULL},
  {"xs:unsignedShort",      n,  FIELDTYPE::numerical, NULL},
  {"xs:positiveInteger",    n,  FIELDTYPE::numerical, NULL},
  {"xs:nonNegativeInteger", n,  FIELDTYPE::numerical, NULL},
  {"xs:negativeInteger",    n,  FIELDTYPE::numerical, NULL},
  {"xs:positiveInteger",    n,  FIELDTYPE::numerical, NULL},
  // 
  {"xs:dateTime", d,  FIELDTYPE::date,      NULL},
  {"xs:time",     d,  FIELDTYPE::time,      NULL}
};

void FIELDTYPE::AvailableTypesList(STRLIST *Strlist) const
{
  Strlist->Clear();
  for (size_t i = 0; i < MAX_DATATYPE; i++)
    Strlist->AddEntry (DataTypes[i].Name);
}

void FIELDTYPE::AvailableTypesHelp(ostream& os) const
{
  FIELDTYPE ft;
  STRLIST   strlist;
  size_t    i =0;
  
  os << "The following fundamental data types are currently supported (v." <<  FIELDTYPE::special
	<< "." <<  ((sizeof(DataTypes)/sizeof(DataType ))-FIELDTYPE::special)/3 << "):" << endl;
  for (; i <= MAX_DATATYPE; i++)
    os << "   " << DataTypes[i].Name << "  \t// " << DataTypes[i].Description << endl;
  os << "They are also available via the following alternative 'compatibility' names:" << endl;
  for (; i < sizeof(DataTypes)/sizeof(DataTypes[0]); i++) {
   if (!IsExternal(DataTypes[i].Type))
    os << "   " << DataTypes[i].Name << "     \t// Alias of " <<
		DataTypes[DataTypes[i].Type].Name << endl; // was -1
  }
}



static const char * const FieldObjectFormat = "%s$$%d";


ostream& operator <<(ostream& os, const FIELDTYPE& Ft)
{
 return  os << Ft.c_str();
}

bool operator ==(const FIELDTYPE& s1, const FIELDTYPE& s2)
{
  return s1.Type == s2.Type;
}
bool operator !=(const FIELDTYPE& s1, const FIELDTYPE& s2)
{
  return s1.Type != s2.Type;
}

bool operator ==(const FIELDTYPE& s1, const BYTE s2)
{
  return (BYTE)(s1.Type) == s2;
}
bool operator !=(const FIELDTYPE& s1, const BYTE s2)
{
  return (BYTE)(s1.Type) != s2;
}
bool operator ==(const BYTE s1, const FIELDTYPE& s2)
{
  return (BYTE)(s2.Type) == s1;
}
bool operator !=(const BYTE s1, const FIELDTYPE& s2)
{
  return (BYTE)(s2.Type) != s1;
}



STRING  FIELDOBJ::GetName() const
{
  return ((INT)FieldType > 1) ?
  STRING().form(FieldObjectFormat, FieldName.c_str(), (INT)FieldType) :
        FieldName;
}

void  FIELDOBJ::SetName(const STRING& Name)
{
  char name[64];
  int  x;
  switch( sscanf(Name.c_str(), FieldObjectFormat, name, &x) ) {
     case 1: FieldName = Name; FieldType = 1; break;
     case 2: FieldName = name; FieldType = x; break;
     default:
        FieldName.Clear(); FieldType.Clear(); break;
  }
}


FIELDTYPE::FIELDTYPE()
{
  Type = any;
}

FIELDTYPE::FIELDTYPE(const FIELDTYPE& Other)
{
  Type = Other.Type;
}


static const char undef_msg[] = "Can't use fieldtype '%s', private action (callback) undefined!";

FIELDTYPE::FIELDTYPE(const STRING& TypeName)
{
  if (!TypeName.IsEmpty())
    {
      Type = (enum datatypes)TypeName.GetInt();
      for (size_t i = 0; Type == 0 && i < sizeof(DataTypes)/sizeof(DataType); i++)
	{
	  if (TypeName == DataTypes[i].Name)
	    {
	      if ((Type = (enum datatypes)(DataTypes[i].Type)) == FIELDTYPE::privhash)
		{
		  if (!_IB_private_hash)
		    {
		      Type = FIELDTYPE::any;
		      message_log (LOG_ERROR, undef_msg, TypeName.c_str());
		    }
		}
	    }
	}
    }
  else
   Type = FIELDTYPE::any;
}

FIELDTYPE::FIELDTYPE(const char * TypeName)
{
  if (TypeName && *TypeName)
    {
      Type = (enum datatypes)atol(TypeName);
      for (size_t i = 0; Type == 0 && i < sizeof(DataTypes)/sizeof(DataType); i++)
	{
	  if (strcmp(TypeName, DataTypes[i].Name) == 0)
	    {
	      if ((Type = (enum datatypes)(DataTypes[i].Type)) == FIELDTYPE::privhash)
		{
		  if (!_IB_private_hash)
		    {
		      Type = FIELDTYPE::any;
		      message_log (LOG_ERROR, undef_msg, TypeName);
		    }
		}
	    }
	}
    }
  else
    Type = FIELDTYPE::any;
}


FIELDTYPE::FIELDTYPE(const BYTE newType)
{
  // special is the last one
  if (newType <=  special)
    Type = (enum datatypes)newType;
  else
    Type = unknown;
}

FIELDTYPE::FIELDTYPE(const INT newType)
{
  // special is the last one
  if (newType <=  special)
    Type = (enum datatypes)newType;
  else
    Type = unknown;
}


FIELDTYPE& FIELDTYPE::operator=(const FIELDTYPE& OtherType)
{
  Type = OtherType.Type;
  return *this;
}

FIELDTYPE::operator STRING () const
{
  const char *d = c_str();
  if (d == NULL)
    return NulString;
  return STRING(d);
}

const char * FIELDTYPE::c_str() const
{
  if (Type >= 0 && Type <= MAX_DATATYPE) // was > 0 // 2022
    return DataTypes[Type].Name; // was -1 // 2022

  return "";
}

const char * FIELDTYPE:: datatype() const
{
  if (Type >= 0 && Type <= MAX_DATATYPE) // was > 0 // 2022
    return DataTypes[Type].dtype; // was -1 // 2022
  return  DataTypes[0].dtype;
} 

//////////////////////////////////////////////////////////////


ATTRLIST::ATTRLIST ()
{
  MaxEntries = 0;
  Table = NULL; 
  TotalEntries = 0;
}

ATTRLIST::ATTRLIST (size_t Size)
{
  MaxEntries = Size;
  Table = (MaxEntries > 0 ? new ATTR[MaxEntries] : NULL);
  TotalEntries = 0;
}

ATTRLIST::ATTRLIST (const ATTRLIST& OtherAttrlist)
{
  MaxEntries = 0;
  *this = OtherAttrlist;
}


ATTRLIST& ATTRLIST:: operator =(const ATTRLIST& OtherAttrlist)
{
  const size_t OtherTotal = OtherAttrlist.GetTotalEntries ();

  if (OtherTotal > MaxEntries)
    {
      // Need to get a larger area
      ATTR *oldTable = Table;

      MaxEntries = OtherAttrlist.MaxEntries;
      Table = new ATTR[MaxEntries];
      if (oldTable) delete[] oldTable;
    }
  for (TotalEntries = 0; TotalEntries < OtherTotal; TotalEntries++)
    {
      Table[TotalEntries] = OtherAttrlist.Table[TotalEntries];
    }
  return *this;
}


ATTRLIST& ATTRLIST:: Cat(const ATTRLIST& OtherAttrlist)
{
  const size_t OtherTotal = OtherAttrlist.GetTotalEntries ();
  for (size_t i= 0; i < OtherTotal; i++)
    AddEntry(OtherAttrlist.Table[i]);
  return *this;
}

void ATTRLIST::AddEntry (const ATTR& AttrRecord)
{
  if (TotalEntries == MaxEntries)
    Expand ();
  Table[TotalEntries++] = AttrRecord;
}

bool ATTRLIST::GetEntry (const size_t Index, PATTR AttrRecord) const
{
  if ((Index > 0) && (Index <= TotalEntries))
    {
      *AttrRecord = Table[Index - 1];
      return true;
    }
  return false;
}

void ATTRLIST::SetEntry (const size_t Index, const ATTR& AttrRecord)
{
  if ((Index > 0) && (Index <= TotalEntries))
    {
      Table[Index - 1] = AttrRecord;
    }
}

void ATTRLIST::DeleteEntry (const size_t Index)
{
  if ((Index > 0) && (Index <= TotalEntries))
    {
      for (size_t x = (Index - 1); x < (TotalEntries - 1); x++)
	{
	  Table[x] = Table[x + 1];
	}
      TotalEntries--;
    }
}

void ATTRLIST::Expand ()
{
  Resize ((TotalEntries << 1) + 10);
}

void ATTRLIST::CleanUp ()
{
  Resize (TotalEntries);
}

void ATTRLIST::Resize (const size_t Entries)
{
  PATTR OldTable = Table;

  MaxEntries = Entries;
  Table = new ATTR[MaxEntries];

  TotalEntries = (Entries >= TotalEntries) ? TotalEntries : Entries;

  for (size_t i = 0; i < TotalEntries; i++)
    Table[i] = OldTable[i];

  if (OldTable) delete[]OldTable;
}

size_t ATTRLIST::GetTotalEntries () const
{
  return TotalEntries;
}

size_t ATTRLIST::Lookup (const STRING& SetId, const INT AttrType) const
{
  STRING S;
  for (size_t x = 0; x < TotalEntries; x++)
    {
      Table[x].GetSetId (&S);
      if ((SetId == S) && (AttrType == Table[x].GetAttrType ()))
	{
	  return (x + 1);
	}
    }
  return 0;
}

size_t ATTRLIST::Lookup (const STRING& SetId, const INT AttrType, const INT AttrValue) const
{
  STRING S;
  for (size_t x = 0; x < TotalEntries; x++)
    {
      Table[x].GetSetId (&S);
      if ((SetId == S) && (AttrType == Table[x].GetAttrType ()) &&
	  (AttrValue == Table[x].GetAttrValue ()))
	{
	  return (x + 1);
	}
    }
  return 0;
}

void ATTRLIST::SetValue (const STRING& SetId, const INT AttrType, const STRING& AttrValue)
{
  ATTR Attr;
  const INT y = Lookup (SetId, AttrType);
  if (y)
    {
      if (GetEntry (y, &Attr) && (Attr.GetAttrValue(NULL) != AttrValue))
	{
	  Attr.SetAttrValue (AttrValue);
	  SetEntry (y, Attr);
	}
    }
  else
    {
      Attr.SetSetId (SetId);
      Attr.SetAttrType (AttrType);
      Attr.SetAttrValue (AttrValue);
      AddEntry (Attr);
    }
}

void ATTRLIST::SetValue (const STRING& SetId, const INT AttrType, const INT AttrValue)
{
  STRING S (AttrValue);
  SetValue (SetId, AttrType, S);
}

void ATTRLIST::ClearAttr (const STRING& SetId, const INT AttrType, const INT AttrValue)
{
  INT y = Lookup (SetId, AttrType, AttrValue);
  DeleteEntry (y);
}

bool ATTRLIST::GetValue (const STRING& SetId, const INT AttrType, PSTRING StringBuffer) const
{
  ATTR Attr;
  const INT y = Lookup (SetId, AttrType);
  if (y)
    {
      GetEntry (y, &Attr);
      Attr.GetAttrValue (StringBuffer);
      return true;
    }
  StringBuffer->Clear();
  return false;
}

STRING ATTRLIST::GetValue (const STRING& SetId, const INT AttrType) const
{
  STRING String;
  GetValue(SetId, AttrType, &String);
  return String;
}


bool ATTRLIST::GetValue (const STRING& SetId, const INT AttrType, INT *IntBuffer) const
{
  STRING      S;
  bool res = GetValue (SetId, AttrType, &S) ? true : false;
  if (IntBuffer && res)
    *IntBuffer = S.GetInt ();
  return res;
}


///////////////////////////////////////////////////////////////////////

/*-
     USE ATTRIBUTES           (TYPE = 1)
     RELATION ATTRIBUTES      (TYPE = 2)
     POSITION ATTRIBUTES      (TYPE = 3)
     STRUCTURE ATTRIBUTES     (TYPE = 4)
     TRUNCATION ATTRIBUTES    (TYPE = 5)
     COMPLETENESS ATTRIBUTES  (TYPE = 6)
*/

enum AttrTypes {
  UseAttribute = 1,
  RelationAttribute = 2,
  PositionAttribute = 3,
  StructureAttribute = 4,
  TruncationAttribute = 5
};

enum RelationValues {
  LessThan = 1,
  LessThanOrEqual = 2,
  Equal = 3,
  GreaterOrEqual = 4,
  GreaterThan=5,
  NotEqual=6,
  Phonetic = 100,
  Stem = 101,
  Relevance = 102,
  AlwaysMatches = 103,
  Exact = 200
};

enum StructureValues {
  Phrase = 1, // Literal
  Word = 2,
  Key = 3,
  Year = 4,
  WordList = 5,
  Date = 100,
  NormName = 101,
  UnnormName = 102,
  Structure = 103,
  URX = 104,
  FreeFormText = 105,
  DocumentText = 106,
  LocalNumber = 107,
  String = 108,
  NumericString = 109, // Number
  Glob = 200, // Local Extension
  DenseVector // Dense Vector (TODO)
};

enum TruncationValues {
  Right = 1,
  Left = 2,
  LeftAndRight = 3,
  DoNot = 100
};


// Index Attributes (Features)
void  ATTRLIST::AttrSetFieldObj(const FIELDOBJ& FieldObj)
{
  FIELDTYPE FieldType = FieldObj.GetFieldType();

  AttrSetFieldName ( FieldObj.GetFieldName() );
  SetValue(IsearchAttributeSet, IsearchTypeAttr, (const char *)FieldType);
#if 1
  if (FieldType.IsNumerical())
    {
      AttrSetFieldNumerical(true);
    }
  else if (FieldType.IsDate())
    {
      AttrSetFieldDate(true);
    }
#endif
}


FIELDOBJ ATTRLIST::AttrGetFieldObj() const
{
  STRING s, t;
  if (GetValue (IsearchAttributeSet, IsearchFieldAttr, &s))
    GetValue(IsearchAttributeSet, IsearchTypeAttr, &t);
  return FIELDOBJ(s, t);
}



void ATTRLIST::AttrSetFieldName (const STRING& FieldName)
{
  STRING Field (FieldName);
  SetValue (IsearchAttributeSet, IsearchFieldAttr, Field.ToUpper());
}
 
bool ATTRLIST::AttrGetFieldName (PSTRING StringBuffer) const
{
  return GetValue (IsearchAttributeSet, IsearchFieldAttr, StringBuffer);
}

STRING ATTRLIST::AttrGetFieldName () const
{
  return GetValue (IsearchAttributeSet, IsearchFieldAttr);
}


void ATTRLIST::AttrSetFieldType(const FIELDTYPE& FieldType)
{
  STRING Type (FieldType.c_str());
  SetValue(IsearchAttributeSet, IsearchTypeAttr, Type);

#if 1
// Data types: "text" (the default), "num", "date", "date-range" and "gpoly".

  if (FieldType.IsNumerical())
    {
      AttrSetFieldNumerical(true);
    }
  else if (FieldType.IsDate())
    {
      AttrSetFieldDate(true);
    }
  else
    SetValue(IsearchAttributeSet, IsearchTypeAttr, Type);
#endif
}

bool ATTRLIST::AttrGetFieldType(PSTRING StringBuffer) const
{
  STRING s;

  if (false == GetValue(IsearchAttributeSet, IsearchTypeAttr, &s))
    {
      FIELDTYPE ft = AttrGetFieldType();
      if (ft.Defined())
	s = ft.c_str();
    }
  if (StringBuffer) *StringBuffer = s;

  return s.GetLength();
}

FIELDTYPE ATTRLIST::AttrGetFieldType() const
{
  if (AttrGetFieldNumerical ())
    {
      return FIELDTYPE::numerical;
    }
  if (AttrGetFieldDate())
    {
      return FIELDTYPE::date;
    }
  return GetValue(IsearchAttributeSet, IsearchTypeAttr);
}


#if 0
enum Objects {
  StringObject,
  NumberObject,
  DateObject
};

void ATTRLIST::SetFieldType (enum StructureValues Typ, bool Set)
{
  if (Set)
    SetValue (Bib1AttributeSet, StructureAttribute,  Typ);
  else
    ClearAttr (Bib1AttributeSet, StructureAttribute,  Typ);
}

void ATTRLIST::SetFieldType (enum StructureValues Typ, bool Set)
{
  return Lookup (Bib1AttributeSet, StructureAttribute,  NumericString) ?
	true :
	false;
}
#endif

#if 1
void ATTRLIST::AttrSetFieldNumerical (const bool Set)
{
  if (Set)
    SetValue (Bib1AttributeSet, StructureAttribute,  NumericString);
  else
    ClearAttr (Bib1AttributeSet, StructureAttribute,  NumericString);
}

bool ATTRLIST::AttrGetFieldNumerical () const
{
  return Lookup (Bib1AttributeSet, StructureAttribute, NumericString) ?
	true :
	false;
}

void ATTRLIST::AttrSetFieldDate (const bool Set)
{
  if (Set)
    SetValue (Bib1AttributeSet, StructureAttribute,  Date);
  else
    ClearAttr (Bib1AttributeSet, StructureAttribute,  Date);
}

bool ATTRLIST::AttrGetFieldDate () const
{
  return Lookup (Bib1AttributeSet, StructureAttribute, Date) ?
        true :
        false;
}
#endif


void ATTRLIST::AttrSetFreeForm (const bool Set)
{
  if (Set)
    SetValue (Bib1AttributeSet, StructureAttribute,  FreeFormText);
  else
    ClearAttr (Bib1AttributeSet, StructureAttribute, FreeFormText);
}
 
bool ATTRLIST::AttrGetFreeForm () const
{
  return Lookup (Bib1AttributeSet, StructureAttribute, FreeFormText) ?
	true :
	false;
}

// Dense vector feedback
void ATTRLIST::AttrSetDenseFeedback (const bool Set)
{ 
  if (Set)
    SetValue (Bib1AttributeSet, StructureAttribute, DenseVector);
  else
    ClearAttr (Bib1AttributeSet, StructureAttribute, DenseVector);
}
  
bool ATTRLIST::AttrGetDenseFeedback() const
{ 
  return Lookup (Bib1AttributeSet, StructureAttribute, DenseVector) ?
        true :
        false;
} 



// Search Term Attributes 

void ATTRLIST::AttrSetPhrase(const bool Set)
{
  if (Set)
    SetValue (Bib1AttributeSet, StructureAttribute, Phrase);
  else
    ClearAttr (Bib1AttributeSet, StructureAttribute, Phrase);
}


bool ATTRLIST::AttrGetPhrase() const
{
  return Lookup (Bib1AttributeSet, StructureAttribute,  Phrase) ?
        true :
        false;
}


void ATTRLIST::AttrSetRightTruncation (const bool Set)
{
  if (Set)
    SetValue (Bib1AttributeSet, TruncationAttribute,  Right);
  else
    ClearAttr (Bib1AttributeSet, TruncationAttribute,  Right);
}


bool ATTRLIST::AttrGetRightTruncation () const
{
  return Lookup (Bib1AttributeSet, TruncationAttribute,  Right) ?
	true :
	false;
}

void ATTRLIST::AttrSetLeftTruncation (const bool Set)
{
  if (Set)
    SetValue (Bib1AttributeSet, TruncationAttribute,  Left);
  else
    ClearAttr (Bib1AttributeSet, TruncationAttribute,  Left);
}


bool ATTRLIST::AttrGetLeftTruncation () const
{
  return Lookup (Bib1AttributeSet, TruncationAttribute,  Left) ?
        true :
        false;
}


void ATTRLIST::AttrSetLeftAndRightTruncation (const bool Set)
{
  if (Set) {
    SetValue (Bib1AttributeSet, TruncationAttribute,  LeftAndRight);
  } else {
    ClearAttr (Bib1AttributeSet, TruncationAttribute,  LeftAndRight);
  }
}


bool ATTRLIST::AttrGetLeftAndRightTruncation () const
{
  return Lookup (Bib1AttributeSet, TruncationAttribute, LeftAndRight) ?
        true :
        false;
}

void ATTRLIST::AttrSetGlob (const bool Set)
{
  if (Set)
    SetValue (Bib1AttributeSet, StructureAttribute, Glob);
  else
    ClearAttr (Bib1AttributeSet, StructureAttribute, Glob);
}


bool ATTRLIST::AttrGetGlob () const
{
  return Lookup (Bib1AttributeSet, StructureAttribute,  Glob) ?
	true :
	false;
}

void ATTRLIST::AttrSetPhonetic (const bool Set)
{
  if (Set)
    SetValue (Bib1AttributeSet, RelationAttribute, Phonetic);
  else
    ClearAttr (Bib1AttributeSet,  RelationAttribute, Phonetic);
}

bool ATTRLIST::AttrGetPhonetic () const
{
  return Lookup (Bib1AttributeSet,  RelationAttribute, Phonetic) ?
	true :
	false;
}

void ATTRLIST::AttrSetExactTerm (const bool Set)
{
  if (Set)
    SetValue (Bib1AttributeSet, RelationAttribute, Exact);
  else
    ClearAttr (Bib1AttributeSet,  RelationAttribute, Exact);
}

bool ATTRLIST::AttrGetExactTerm () const
{
  return Lookup (Bib1AttributeSet,  RelationAttribute, Exact) ?
        true :
        false;
}


void ATTRLIST::AttrSetAlwaysMatches (const bool Set)
{
  if (Set)
    SetValue (Bib1AttributeSet,  RelationAttribute, AlwaysMatches);
  else
    ClearAttr (Bib1AttributeSet,  RelationAttribute, AlwaysMatches);
}

bool ATTRLIST::AttrGetAlwaysMatches () const
{
  return Lookup (Bib1AttributeSet,  RelationAttribute, AlwaysMatches) ?
	true :
	false;
}


void ATTRLIST::AttrSetRelation(const INT Relation)
{
  SetValue(IsearchAttributeSet, ZRelationAttribute, Relation );
}


INT ATTRLIST::AttrGetRelation() const
{
  INT Rel;
  if (AttrGetRelation(&Rel)) return Rel;
  return -1;
}

bool ATTRLIST::AttrGetRelation(INT *IntBuffer) const
{
  STRING S;
  bool res = GetValue(IsearchAttributeSet, ZRelationAttribute, &S);
  if (IntBuffer && res)
    *IntBuffer = S.GetInt();
  return res;
}


void ATTRLIST::AttrSetStructure(const INT Structure)
{
  SetValue(IsearchAttributeSet, ZStructureAttribute, Structure);
}


INT ATTRLIST::AttrGetStructure() const
{
  INT Zs;

  if (AttrGetStructure(&Zs)) return Zs;
  return -1;
}

bool ATTRLIST::AttrGetStructure(INT *IntBuffer) const
{
  STRING S;
  bool res = GetValue(IsearchAttributeSet, ZStructureAttribute, &S);
  if (IntBuffer && res)
    *IntBuffer = S.GetInt();
  return res;
}


void ATTRLIST::AttrSetTermWeight (const INT TermWeight)
{
  SetValue (IsearchAttributeSet, IsearchWeightAttr, (STRING) TermWeight);
}

INT ATTRLIST::AttrGetTermWeight () const
{
  STRING S;
  if (GetValue (IsearchAttributeSet, IsearchWeightAttr, &S))
    {
      return S.GetInt ();
    }
  return 1;
}

//////////////////////////////////////////////////////////////////////


void ATTRLIST::Write (PFILE Fp) const
{
#if DFD_COMPAT_IO
  // Backward compatiable
  fprintf (Fp, "%d\n", (int) TotalEntries);
#else
  putObjID(objATTRLIST, Fp);
  ::Write ((INT2)TotalEntries, Fp);
#endif
  for (size_t i = 0; i < TotalEntries; i++)
    {
      Table[i].Write (Fp);
    }
}

bool ATTRLIST::Read (PFILE Fp)
{
#if DFD_COMPAT_IO
  // Backward Compatible
  STRING s;
  s.FGet (Fp, 5);
  size_t Entries = s.GetInt ();

  if (Entries > MaxEntries)
    {
      if (Table) delete[]Table;
      MaxEntries = Entries;
      Table = new ATTR[MaxEntries];
    }
  for (size_t i = 0; i < Entries; i++)
    {
      Table[i].Read (Fp);
    }
  TotalEntries = Entries;
  return true;
#else
  INT2 Entries = 0;
  obj_t obj = getObjID(Fp);
  if (obj != objATTRLIST)
    {
      PushBackObjID (obj, Fp);
    }
  else
    {
      ::Read (&Entries, Fp);
    }
  if ((size_t)Entries > MaxEntries)
    {
      if (Table) delete[]Table;
      MaxEntries = (size_t)Entries;
      Table = new ATTR[MaxEntries];
    }
  for (size_t i = 0; i < (size_t)Entries; i++)
    {
      Table[i].Read (Fp);
    }
  TotalEntries = (size_t)Entries;
  return obj == objATTRLIST;
#endif
}

ATTRLIST::~ATTRLIST ()
{
  if (Table) delete[]Table;
}
