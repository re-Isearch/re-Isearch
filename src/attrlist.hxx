/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */
/*@@@
File:		attrlist.hxx
Description:	Class ATTRLIST - Attribute List
@@@*/

#ifndef ATTRLIST_HXX
#define ATTRLIST_HXX

#include "defs.hxx"
#include "attr.hxx"


class FIELDTYPE {
public:
  enum datatypes { unknown=-1, any = 0, text, numerical, computed, numericalrange,
	date, daterange, gpoly, box, time, ttl, ttl_expires, boolean, currency,
	dotnumber, phonhash, phonhash2, metaphone, metaphone2, hash, casehash,
	lexi, privhash, isbn, telnumber, creditcardnum, iban, bic,
	db_string, callback, callback1, callback2, callback3, callback4, callback5, callback6, callback7,
	db_hnsw, special, __last=255};

  FIELDTYPE();
  FIELDTYPE(const FIELDTYPE& OtherFieldType);
  FIELDTYPE(const char *Type);
  FIELDTYPE(const STRING& Type);
  FIELDTYPE(const BYTE Type);
  FIELDTYPE(const INT  Type);

  operator       INT () const  { return Type; }
  operator       STRING () const;
  operator       const char *() const { return c_str();        }

  const char    *c_str() const;

  const char    *datatype() const;

  void           AvailableTypesList(STRLIST *Strlist) const;
  void           AvailableTypesHelp(ostream& os) const;


  void           Clear() { Type = any; }

  bool    Defined() const    { return Type >  0;          }
  bool    Ok() const         { return Type >= 0;          }
  bool    Equals(const FIELDTYPE Other) const {return Type == Other.Type; }
  bool    Equals(const BYTE Other) const      {return (BYTE)Type == Other;      }

  bool    IsBoolean() const  { return (Type == numerical) || (Type == boolean); }

  bool    IsText() const     { return Type <= text || Type == isbn;      }
  bool    IsString() const   { return IsText() || IsPhonetic() || IsHash() ||
			IsCaseHash() || IsPrivHash() || IsLexiHash(); }
  bool    IsNumeric() const { return Type == numerical || Type == computed || Type == currency || Type == dotnumber; }
  bool    IsNumerical() const{ return Type == numerical || Type == dotnumber || Type == ttl; }
  bool    IsComputed() const { return Type == computed;  }
  bool    IsNumericalRange() const { return Type == numericalrange; }
  bool    IsDate() const     { return Type == date || Type == time || Type == ttl_expires; }
  bool    IsDateRange() const{ return Type == daterange; }
  bool    IsGPoly() const    { return Type == gpoly;     }
  bool    IsISBN() const     { return Type == isbn;      }
  bool    IsIBAN() const     { return Type == iban;      }
  bool    isBIC() const      { return Type == bic;       }
  bool    isBankingnum() const  { return Type == iban || Type == bic || Type == creditcardnum;}
  bool    IsBox() const      { return Type == box;       }
  bool    IsCurrency() const { return Type == currency;  }
  bool    IsPhoneHash() const{ return Type == phonhash || Type == phonhash2; }
  bool    IsMetaphone() const{ return Type == metaphone || Type == metaphone2; }
  bool    IsPhonetic() const { return IsPhoneHash() || IsMetaphone(); }
  bool    IsHash() const     { return Type == hash;      }
  bool    IsCaseHash() const { return Type == casehash;  }
  bool    IsLexiHash() const { return Type == lexi;      }
  bool    IsPrivHash() const { return Type == privhash;  }
  bool    IsDBMStr() const   { return Type == db_string; }
  bool    IsHNSW()           { return Type == db_hnsw;   } // Hierarchical Navigable Small Worlds (HNSW)
  bool    IsCallback() const { return IsCallback(Type);  }
  bool    IsExternal() const { return IsExternal(Type);  }

  FIELDTYPE& operator=(const FIELDTYPE& OtherType);

  friend ostream& operator <<(ostream&, const FIELDTYPE&);
  friend bool operator ==(const FIELDTYPE& s1, const FIELDTYPE& s2);
  friend bool operator !=(const FIELDTYPE& s1, const FIELDTYPE& s2);
  friend bool operator ==(const FIELDTYPE& s1, const BYTE s2);
  friend bool operator !=(const FIELDTYPE& s1, const BYTE s2);
  friend bool operator ==(const BYTE s1, const FIELDTYPE& s2);
  friend bool operator !=(const BYTE s1, const FIELDTYPE& s2);

private:
  bool IsExternal(const BYTE& type) const { return type >= db_string && type <= callback7;}
  bool IsCallback(const BYTE& type) const { return type >= callback && type <= callback7; }
  enum datatypes   Type;
};



class FIELDOBJ {
public:
  FIELDOBJ () {; }
  FIELDOBJ (const STRING& Name) {
    SetName(Name);
  }
  FIELDOBJ (const STRING& fieldName, const FIELDTYPE& fieldType) {
    FieldName = fieldName;
    FieldType = fieldType;
  }
  STRING    GetName() const;
  void      SetName(const STRING& Name);

  void      SetFieldName(const STRING& newFieldName) { FieldName = newFieldName; }
  STRING    GetFieldName() const  { return FieldName; }
  void      SetFieldType(const FIELDTYPE& newFieldType) { FieldType = newFieldType; }
  FIELDTYPE GetFieldType() const  { return FieldType; }

  operator       STRING () const  { return GetName(); }

  FIELDOBJ& operator=(const FIELDOBJ& OtherObj) {
    FieldName = OtherObj.FieldName;
    FieldType = OtherObj.FieldType;
    return *this;
  }

private:
  STRING    FieldName;
  FIELDTYPE FieldType;
};

class ATTRLIST {
public:
  ATTRLIST();
  ATTRLIST(size_t InitialSize);
  ATTRLIST(const ATTRLIST& OtherAttrlist);

  ATTR        operator[](size_t Index) const { return Table[Index]; }


  ATTRLIST&   operator=(const ATTRLIST& OtherAttrlist);

  ATTRLIST&   Cat(const ATTRLIST& OtherAttrlist);

  void        AddEntry(const ATTR& AttrRecord);
  bool GetEntry(const size_t Index, PATTR AttrRecord) const;
  void        SetEntry(const size_t Index, const ATTR& AttrRecord);
  void        DeleteEntry(const size_t Index);
  void        Expand();
  void        CleanUp();
  void        Resize(const size_t Entries);
  size_t      GetTotalEntries() const;
  size_t      Lookup(const STRING& SetId, const INT AttrType) const;
  size_t      Lookup(const STRING& SetId, const INT AttrType, const INT AttrValue) const;
  void        SetValue(const STRING& SetId, const INT AttrType, const STRING& AttrValue);
  void        SetValue(const STRING& SetId, const INT AttrType, const INT AttrValue);
  void        ClearAttr(const STRING& SetId, const INT AttrType, const INT AttrValue);
  bool GetValue(const STRING& SetId, const INT AttrType, PSTRING StringBuffer) const;
  bool GetValue(const STRING& SetId, const INT AttrType, INT *IntBuffer) const;
  STRING      GetValue(const STRING& SetId, const INT AttrType) const;

// Index Attributes (Features)
  void        AttrSetFieldObj(const FIELDOBJ& FieldObj);
  FIELDOBJ    AttrGetFieldObj() const;

  void        AttrSetFieldName(const STRING& FieldName);
  bool AttrGetFieldName(PSTRING StringBuffer) const;
  STRING      AttrGetFieldName() const;

  void        AttrSetFieldType(const FIELDTYPE& FieldType);
  bool AttrGetFieldType(PSTRING StringBuffer) const;
  FIELDTYPE   AttrGetFieldType() const;

#if 1 /* These are now obsolete by the full blow type system */
  void        AttrSetFieldNumerical (const bool Set);
  bool AttrGetFieldNumerical () const;

  void        AttrSetFieldDate (const bool Set);
  bool AttrGetFieldDate () const;
#endif

// Search Term Attributes
  void        AttrSetFreeForm (const bool Set);
  bool AttrGetFreeForm () const;

  void        AttrSetPhrase(const bool Phrase);
  bool AttrGetPhrase() const;

  void        AttrSetRightTruncation(const bool RightTruncate);
  bool AttrGetRightTruncation() const;

  void        AttrSetLeftTruncation(const bool LeftTruncate);
  bool AttrGetLeftTruncation() const;

  void        AttrSetLeftAndRightTruncation(const bool Truncate);
  bool AttrGetLeftAndRightTruncation() const;

  void        AttrSetGlob(const bool RightTruncate);
  bool AttrGetGlob() const;

  void        AttrSetAlwaysMatches(const bool AlwaysMatches);
  bool AttrGetAlwaysMatches() const;

  void        AttrSetPhonetic(const bool Phonetic);
  bool AttrGetPhonetic() const;

  void        AttrSetExactTerm(const bool Exact);
  bool AttrGetExactTerm() const;

  void        AttrSetRelation(const INT Relation);
  bool AttrGetRelation(INT *IntBuffer) const;
  INT         AttrGetRelation() const; // -1 means failed

  void        AttrSetStructure(const INT Structure);
  bool AttrGetStructure(INT *IntBuffer) const;
  INT         AttrGetStructure() const; // -1 means failed

  void        AttrSetTermWeight(const INT TermWeight);
  INT         AttrGetTermWeight() const;

  void        Write(PFILE Fp) const;
  bool Read(PFILE Fp);

  ~ATTRLIST();
private:
  PATTR       Table;
  size_t      TotalEntries;
  size_t      MaxEntries;
};

typedef ATTRLIST* PATTRLIST;

// Common functions

inline void Write (const ATTRLIST& Attrlist, PFILE Fp)
{
  Attrlist.Write (Fp);
}

inline bool Read (PATTRLIST AttrlistPtr, PFILE Fp)
{
  return AttrlistPtr->Read (Fp);
}


#endif
