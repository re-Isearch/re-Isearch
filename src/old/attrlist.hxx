/*@@@
File:		attrlist.hxx
Version:	1.00
Description:	Class ATTRLIST - Attribute List
Author:		Nassib Nassar, nrn@cnidr.org
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
	lexi, privhash, isbn, telnumber, creditcardnum,
	db_string, callback, callback1, callback2, callback3, callback4, callback5, callback6, callback7,
	special, __last=255};

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

  GDT_BOOLEAN    Defined() const    { return Type >  0;          }
  GDT_BOOLEAN    Ok() const         { return Type >= 0;          }
  GDT_BOOLEAN    Equals(const FIELDTYPE Other) const {return Type == Other.Type; }
  GDT_BOOLEAN    Equals(const BYTE Other) const      {return (BYTE)Type == Other;      }

  GDT_BOOLEAN    IsBoolean() const  { return (Type == numerical) || (Type == boolean); }

  GDT_BOOLEAN    IsText() const     { return Type <= text || Type == isbn;      }
  GDT_BOOLEAN    IsString() const   { return IsText() || IsPhonetic() || IsHash() ||
			IsCaseHash() || IsPrivHash() || IsLexiHash(); }
  GDT_BOOLEAN    IsNumeric() const { return Type == numerical || Type == computed || Type == currency || Type == dotnumber; }
  GDT_BOOLEAN    IsNumerical() const{ return Type == numerical || Type == dotnumber || Type == ttl; }
  GDT_BOOLEAN    IsComputed() const { return Type == computed;  }
  GDT_BOOLEAN    IsNumericalRange() const { return Type == numericalrange; }
  GDT_BOOLEAN    IsDate() const     { return Type == date || Type == time || Type == ttl_expires; }
  GDT_BOOLEAN    IsDateRange() const{ return Type == daterange; }
  GDT_BOOLEAN    IsGPoly() const    { return Type == gpoly;     }
  GDT_BOOLEAN    IsISBN() const     { return Type == isbn;      }
  GDT_BOOLEAN    IsBox() const      { return Type == box;       }
  GDT_BOOLEAN    IsCurrency() const { return Type == currency;  }
  GDT_BOOLEAN    IsPhoneHash() const{ return Type == phonhash || Type == phonhash2; }
  GDT_BOOLEAN    IsMetaphone() const{ return Type == metaphone || Type == metaphone2; }
  GDT_BOOLEAN    IsPhonetic() const { return IsPhoneHash() || IsMetaphone(); }
  GDT_BOOLEAN    IsHash() const     { return Type == hash;      }
  GDT_BOOLEAN    IsCaseHash() const { return Type == casehash;  }
  GDT_BOOLEAN    IsLexiHash() const { return Type == lexi;      }
  GDT_BOOLEAN    IsPrivHash() const { return Type == privhash;  }
  GDT_BOOLEAN    IsDBMStr() const   { return Type == db_string; }
  GDT_BOOLEAN    IsCallback() const { return IsCallback(Type);  }
  GDT_BOOLEAN    IsExternal() const { return IsExternal(Type);  }

  FIELDTYPE& operator=(const FIELDTYPE& OtherType);

  friend ostream& operator <<(ostream&, const FIELDTYPE&);
  friend GDT_BOOLEAN operator ==(const FIELDTYPE& s1, const FIELDTYPE& s2);
  friend GDT_BOOLEAN operator !=(const FIELDTYPE& s1, const FIELDTYPE& s2);
  friend GDT_BOOLEAN operator ==(const FIELDTYPE& s1, const BYTE s2);
  friend GDT_BOOLEAN operator !=(const FIELDTYPE& s1, const BYTE s2);
  friend GDT_BOOLEAN operator ==(const BYTE s1, const FIELDTYPE& s2);
  friend GDT_BOOLEAN operator !=(const BYTE s1, const FIELDTYPE& s2);

private:
  GDT_BOOLEAN IsExternal(const BYTE& type) const { return type >= db_string && type <= callback7;}
  GDT_BOOLEAN IsCallback(const BYTE& type) const { return type >= callback && type <= callback7; }
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
  GDT_BOOLEAN GetEntry(const size_t Index, PATTR AttrRecord) const;
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
  GDT_BOOLEAN GetValue(const STRING& SetId, const INT AttrType, PSTRING StringBuffer) const;
  GDT_BOOLEAN GetValue(const STRING& SetId, const INT AttrType, INT *IntBuffer) const;
  STRING      GetValue(const STRING& SetId, const INT AttrType) const;

// Index Attributes (Features)
  void        AttrSetFieldObj(const FIELDOBJ& FieldObj);
  FIELDOBJ    AttrGetFieldObj() const;

  void        AttrSetFieldName(const STRING& FieldName);
  GDT_BOOLEAN AttrGetFieldName(PSTRING StringBuffer) const;
  STRING      AttrGetFieldName() const;

  void        AttrSetFieldType(const FIELDTYPE& FieldType);
  GDT_BOOLEAN AttrGetFieldType(PSTRING StringBuffer) const;
  FIELDTYPE   AttrGetFieldType() const;

#if 1 /* These are now obsolete by the full blow type system */
  void        AttrSetFieldNumerical (const GDT_BOOLEAN Set);
  GDT_BOOLEAN AttrGetFieldNumerical () const;

  void        AttrSetFieldDate (const GDT_BOOLEAN Set);
  GDT_BOOLEAN AttrGetFieldDate () const;
#endif

// Search Term Attributes
  void        AttrSetFreeForm (const GDT_BOOLEAN Set);
  GDT_BOOLEAN AttrGetFreeForm () const;

  void        AttrSetPhrase(const GDT_BOOLEAN Phrase);
  GDT_BOOLEAN AttrGetPhrase() const;

  void        AttrSetRightTruncation(const GDT_BOOLEAN RightTruncate);
  GDT_BOOLEAN AttrGetRightTruncation() const;

  void        AttrSetLeftTruncation(const GDT_BOOLEAN LeftTruncate);
  GDT_BOOLEAN AttrGetLeftTruncation() const;

  void        AttrSetLeftAndRightTruncation(const GDT_BOOLEAN Truncate);
  GDT_BOOLEAN AttrGetLeftAndRightTruncation() const;

  void        AttrSetGlob(const GDT_BOOLEAN RightTruncate);
  GDT_BOOLEAN AttrGetGlob() const;

  void        AttrSetAlwaysMatches(const GDT_BOOLEAN AlwaysMatches);
  GDT_BOOLEAN AttrGetAlwaysMatches() const;

  void        AttrSetPhonetic(const GDT_BOOLEAN Phonetic);
  GDT_BOOLEAN AttrGetPhonetic() const;

  void        AttrSetExactTerm(const GDT_BOOLEAN Exact);
  GDT_BOOLEAN AttrGetExactTerm() const;

  void        AttrSetRelation(const INT Relation);
  GDT_BOOLEAN AttrGetRelation(INT *IntBuffer) const;
  INT         AttrGetRelation() const; // -1 means failed

  void        AttrSetStructure(const INT Structure);
  GDT_BOOLEAN AttrGetStructure(INT *IntBuffer) const;
  INT         AttrGetStructure() const; // -1 means failed

  void        AttrSetTermWeight(const INT TermWeight);
  INT         AttrGetTermWeight() const;

  void        Write(PFILE Fp) const;
  GDT_BOOLEAN Read(PFILE Fp);

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

inline GDT_BOOLEAN Read (PATTRLIST AttrlistPtr, PFILE Fp)
{
  return AttrlistPtr->Read (Fp);
}


#endif
