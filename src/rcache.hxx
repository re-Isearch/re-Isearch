/*@@@
File:		rcache.hxx
Version:	1.00
Description:	Class RCACHE - Result Set Cache
Author:		Jim Fullton
@@@*/

#ifndef RCACHE_HXX
#define RCACHE_HXX
#include "string.hxx"
#include "irset.hxx"
#include "idbobj.hxx"

class SearchObj {
public:
  SearchObj() { }

  SearchObj(const STRING& term, INT relation, size_t threshold=0, float cost=1.0) {
    Relation  = relation;
    Threshold = threshold;
    Term      = term;
    Cost      = cost;
  }
  SearchObj(const STRING& term, INT relation, const STRING& fieldName,
	size_t threshold=0, float cost=1.0) {
    Relation  = relation;
    Threshold = threshold;
    Term      = term;
    FieldName = fieldName;
    Cost      = cost;
  }
  SearchObj(const STRING& term, INT relation, const STRING& fieldName,
	const DATERANGE& dateRange, size_t threshold=0, float cost = 1.0) {
    Relation  = relation;
    Threshold = threshold;
    DateRange = dateRange;
    Term      = term;
    FieldName = fieldName;
    Cost      = cost;
  }
  SearchObj& operator = (const SearchObj& Other) {
    Relation  = Other.Relation;
    Threshold = Other.Threshold;
    DateRange = Other.DateRange;
    Term      = Other.Term;
    FieldName = Other.FieldName;
    Cost      = Other.Cost;
    return *this;
  }

  GDT_BOOLEAN operator == (const SearchObj& Other) const {
    return ( Relation==Other.Relation &&
        (Threshold==0 || (Other.Threshold<=Threshold && Other.Threshold != 0)) &&
	DateRange == Other.DateRange &&
        Term==Other.Term && FieldName==Other.FieldName );
  }
  void Read(FILE *Fp) {
    ::Read(&Relation,  Fp);
    ::Read(&Term,      Fp);
    ::Read(&FieldName, Fp);
    ::Read(&Threshold, Fp);
    ::Read(&DateRange, Fp);
    ::Read(&Cost,      Fp);
  }
  void Write(FILE *Fp) const {
    ::Write(Relation,  Fp);
    ::Write(Term,      Fp);
    ::Write(FieldName, Fp);
    ::Write(Threshold, Fp);
    ::Write(DateRange, Fp);
    ::Write(Cost,      Fp);
  }

  ~SearchObj() { }

  UINT2     Relation;
  UINT4     Threshold;
  DATERANGE DateRange;
  STRING    Term;
  STRING    FieldName;
  float     Cost;
};

class RCacheObj {
friend class RCACHE;
public:
  RCacheObj(const PIDBOBJ DbParent) {
    ResultSet = new IRSET(DbParent); 
  }
  RCacheObj(const STRING& Term, INT Relation, const STRING& FieldName,
	const IRSET *set, size_t Clip=0, float Cost = 1.0) {
    ResultSet = set->Duplicate();
    Data      = SearchObj (Term, Relation, FieldName, Clip, Cost);
  }
  RCacheObj(const STRING& Term, INT Relation, const STRING& FieldName,
	const DATERANGE& DateRange, const IRSET *set, size_t Clip=0, float Cost=1.0) {
    ResultSet = set->Duplicate();
    Data      = SearchObj (Term, Relation, FieldName, DateRange, Clip, Cost);
  }
  RCacheObj& operator = (const RCacheObj& Other) {
    if (ResultSet != Other.ResultSet)
      {
	if (Other.ResultSet == NULL)
	  {
	    delete ResultSet;
	    ResultSet = NULL;
	  }
	else if (ResultSet == NULL)
	  ResultSet = Other.ResultSet->Duplicate();
	else
	  *ResultSet = *(Other.ResultSet);
      }
    Data      = Other.Data;
    return *this;
  }
  ~RCacheObj() {
    if (ResultSet) delete ResultSet;
  }
  void Read(FILE *Fp) {
    Data.Read(Fp);
    ((IRSET *)ResultSet)->Read(Fp);
  }
  void Write(FILE *Fp) const {
    Data.Write(Fp);
    ((IRSET *)ResultSet)->Write(Fp);
  }
  const IRSET *GetIRSET() const { return (const IRSET *)ResultSet; }

private:
  RCacheObj() {
    ResultSet = NULL;
  }
  OPOBJ    *ResultSet;
  SearchObj Data;
};

class RCACHE {
public:
  RCACHE(const PIDBOBJ Parent);
  RCACHE(const PIDBOBJ Parent, size_t MaxCache);
  INT         Check(const SearchObj& SearchObject);
  INT         Add(const RCacheObj& RCacheObject);
  IRSET      *Fetch(INT Location);
  void        Write (FILE *Fp) const;
  GDT_BOOLEAN Read (FILE *Fp);
  GDT_BOOLEAN Modified() const { return Changed; }
  void        SetPersist(GDT_BOOLEAN Use=GDT_TRUE) { Persist = Use;  }
  GDT_BOOLEAN GetPersist() const                   { return Persist; }
  ~RCACHE();
private:
  void Init(const PIDBOBJ DbParent, size_t MaxCache);
  GDT_BOOLEAN  Changed;
  GDT_BOOLEAN  Persist;
  GDT_BOOLEAN  Sorted;
  size_t       MaxEntries;
  size_t       Count;
  IDBOBJ      *Parent;
#if 0
  Dictionary  *Registry;
#else
  RCacheObj   *Table;
#endif
};


void Write(const RCACHE& Cache, PFILE Fp);
GDT_BOOLEAN Read(RCACHE * Ptr, PFILE Fp);

#endif
