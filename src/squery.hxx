/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */
/*@@@
File:		squery.hxx
Description:	Class SQUERY - Search Query
@@@*/

#ifndef SQUERY_HXX
#define SQUERY_HXX

#include "defs.hxx"
#include "string.hxx"
#include "opstack.hxx"
#include "thesaurus.hxx"


enum QueryTypeMethods { QueryAutodetect = 0, QueryRPN, QueryInfix, QueryRelevantId};

class OPERATOR;

class SQUERY {
public:
  SQUERY();
  SQUERY(THESAURUS *Thesaurus);
  SQUERY(const STRING& QueryTerm, THESAURUS *Thesaurus=NULL);
  SQUERY(const STRING& QueryTerm, enum QueryTypeMethods Typ, THESAURUS *Thesaurus=NULL);
  SQUERY(const SQUERY& OtherSquery) { *this = OtherSquery;};
  SQUERY& operator =(const STRING& OtherQueryTerm);
  SQUERY& operator =(const SQUERY& OtherSquery);
  SQUERY& operator +=(const SQUERY& OtherSquery);
  SQUERY& operator *=(const SQUERY& OtherSquery);
  SQUERY& operator -=(const SQUERY& OtherSquery);
  SQUERY& Cat(const SQUERY& OtherSquery, t_Operator OpGlue = OperatorOr);
  SQUERY& Cat(const SQUERY& OtherSquery, const STRING& Opname);
  void SetOpstack(const OPSTACK& NewOpstack);
  void GetOpstack(OPSTACK *OpstackBuffer) const;

  operator STRING() const { return GetRpnTerm(); }

  size_t GetTotalTerms() const;

  bool Equals(const SQUERY& Other);

  // Is the query plain? No fields, rsets or operators other than OR?
  bool isPlainQuery (t_Operator Operator = OperatorOr) const;
  bool isPlainQuery (STRING *Words, t_Operator Operator = OperatorOr) const;
  bool isPlainQuery (STRLIST *Words, t_Operator Operator = OperatorOr) const;


  // Are all the Ops the same? Example isOpQUery(OperatorAnd) for all AND
  bool isOpQuery (const t_Operator Operator) const;

  // Query is all ANDs, PEERs etc. But no ORs
  bool isIntersectionQuery() const;

  // Covert the Ors in plain query to use Operator
  // example AND:field etc.
  bool SetOperator(const OPERATOR& Operator);
  // Convienience of above, set to AND:<FieldName> or Peer
  bool SetOperatorAndWithin(const STRING& FieldName);
  bool SetOperatorNear(); // Near or Peer with Fielded
  bool SetOperatorPeer();
  bool SetOperatorOr();

  // Set Or and Push REDUCE
  bool PushReduce(int Reduce=0);
  bool SetOperatorOrReduce(int Reduce = 0);

  bool PushUnaryOperator(const OPERATOR& UnaryOperator);

  // Replace means don't add to existing attributes   
  size_t AddAttributes(const ATTRLIST& Attrlist) {
    return SetAttributes(Attrlist, false);
  }
  size_t SetAttributes(const ATTRLIST& Attrlist, bool Replace=true);

  // Returns number of terms set, 0 ==> Error
  size_t SetRelevantTerm (const STRING& RelId);
  size_t SetInfixTerm(const STRING& NewTerm);
  size_t SetRpnTerm(const STRING& NewTerm);
  size_t SetQueryTerm(const STRING& NewTerm); // Try to guess Infix, RPN or ORed words
  size_t SetQueryTermUTF(const STRING& UtfTerm); // NOTE: MUST BE in 8-bit space!

  size_t SetLiteralPhrase();
  size_t SetLiteralPhrase(const STRING& Phrase);

  size_t SetFreeFormWords(const STRING& Sentence, int Weight=1);
  size_t SetFreeFormWordsPhonetic(const STRING& Sentence, int Weight);

  size_t SetWords(const STRING& Sentence, ATTRLIST *Attrlist, const LISTOBJ *Stoplist=NULL);
  
  size_t SetWords (const STRING& NewTerm, INT Weight=1, t_Operator Op = OperatorOr);
  size_t SetWords (const STRING& TermList, const OPERATOR& Operator, int Weight);
  size_t SetWords (const STRING& Sentence, const OPERATOR& Operator, ATTRLIST *AttrlistPtr = NULL);

  size_t SetWords (const STRLIST& TermList, t_Operator Op = OperatorOr, PATTRLIST AttrlistPtr = NULL);
  size_t SetWords (const STRLIST& TermList, INT Weight, t_Operator Op = OperatorOr);
  size_t SetWords (const STRLIST& TermList, const STRING& ESet, t_Operator Op = OperatorOr);
  size_t SetWords (const STRLIST& TermList, const STRING& ESet, INT Weight, t_Operator Op = OperatorOr);
  size_t SetWords (const STRLIST& TermList, const OPERATOR& Operator, int Weight);
  size_t SetWords (const STRLIST& TermList, const OPERATOR& Operator, ATTRLIST *AttrlistPtr = NULL);


// size_t SetAndWords (const STRING& TermList, int Weight = 1) { return SetWords(TermList, Weight, OperatorAnd); }
// size_t SetAndWords (const STRLIST& TermList, int Weight = 1) { return SetWords(TermList, Weight, OperatorAnd); }


  size_t PhraseToProx (const STRING& Term, const STRING& Field = NulString,
	bool LeftTruncated = false, bool Case = false);

  size_t SetTerm  (const STRING& NewTerm);  // old style

  size_t GetRpnTerm(PSTRING StringBuffer) const;
  STRING GetRpnTerm() const {
    STRING query;
    GetRpnTerm(&query);
    return query;
  }
  size_t GetTerm(PSTRING StringBuffer) const;	// old style (Obsolete)

  void        OpenThesaurus(const STRING& FullPath);
  void        CloseThesaurus();
  void        SetThesaurus(THESAURUS *Ptr);
  bool haveThesaurus() const; 

  void ExpandQuery();

  STRING LastErrorMessage() const { return ErrorMessage; }

  friend ostream& operator <<(ostream& os, const SQUERY& str);
  friend istream& operator >>(istream& os, SQUERY& str);


  void Write (PFILE Fp) const;
  bool Read (PFILE Fp);

  ~SQUERY();
private:
  size_t      SetTerm(const STRING&, bool);
  t_Operator  GetOperator (const STRING& Operator, FLOAT *Metric = NULL,
	STRING *StringArgs = NULL) const;
  size_t      fetchTerm(PSTRING StringBuffer, bool WantRpn) const;
  OPSTACK     Opstack;
  THESAURUS  *Thesaurus;
  bool expanded;
  UINT8       Hash;
  STRING      ErrorMessage;
};

typedef SQUERY* PSQUERY;

void        Write(const SQUERY& SQuery, PFILE Fp);
bool Read(PSQUERY SQueryPtr, PFILE Fp);


class QUERY {
friend class IDB;
friend class VIDB;
public:

  QUERY(const QUERY& Query) {
    *this = Query;
  }

  QUERY(enum SortBy sort=Unsorted, enum NormalizationMethods method=defaultNormalization) {
    Sort       = sort;
    Method     = method;
    MaxResults = 0;
  }
  QUERY(const SQUERY& query, enum SortBy sort=Unsorted, enum NormalizationMethods method=defaultNormalization) {
    Sort       = sort;
    Method     = method;
    Squery     = query;
    MaxResults = 0;
  }

  QUERY& operator =(const QUERY& Other) {
    Squery     = Other.Squery;
    Method     = Other.Method;
    Sort       = Other.Sort;
    MaxResults = Other.MaxResults;
    return *this;
  }

  QUERY& operator =(const SQUERY& newSquery) {
    Squery = newSquery;
    return *this;
  }

  bool Ok() { return Run() == 0; }
  int         Run();


  operator enum NormalizationMethods() const { return Method; }
  operator enum SortBy() const { return Sort; }
  operator SQUERY() const { return Squery; }
  operator STRING() const { return Squery.GetRpnTerm(); }

  size_t        GetTotalTerms() const { return Squery.GetTotalTerms(); }
  bool   isPlainQuery (STRING *Words = NULL) { return Squery.isPlainQuery(Words); }

  void          SetSQUERY(const SQUERY& newQuery) { Squery = newQuery; }
  const SQUERY& GetSQUERY() const                 { return Squery;     }

  void SetNormalizationMethod(enum NormalizationMethods newMethod) { Method = newMethod; }
  enum NormalizationMethods GetNormalizationMethod() const { return Method; }

  void SetSortBy(enum SortBy newSort) { Sort = newSort; }
  enum SortBy GetSortBy() const { return Sort; }

  // MaxResults of 0 (Zero) means unlimited
  size_t  GetMaximumResults() const        { return MaxResults; }
  void    SetMaximumResults(size_t newMax) { MaxResults = newMax;}

  // Unlimited number of results OK?
  bool   isUnlimited() const {  return MaxResults == 0 || MaxResults > 0x7fffffff ; }

  void        Write (PFILE Fp) const;
  bool Read  (PFILE Fp);

#if 1
  bool SetOperator(const OPERATOR& Operator) {
    return Squery.SetOperator(Operator);
  }
  bool SetOperatorAndWithin(const STRING& FieldName) {
    return Squery.SetOperatorAndWithin(FieldName);
  }
  bool SetOperatorNear() { return Squery.SetOperatorNear(); }
  bool SetOperatorPeer() { return Squery.SetOperatorPeer(); }
  bool SetOperatorOr()   { return Squery.SetOperatorOr(); }
  bool SetOperatorOrReduce(int Reduce = 0) {
    return Squery.SetOperatorOrReduce(Reduce);
  }

#endif

private:
  SQUERY                    Squery;
  enum SortBy               Sort;
  enum NormalizationMethods Method;
  size_t                    MaxResults;
};


void        Write(const QUERY& Query, PFILE Fp);
bool Read(QUERY *QueryPtr, PFILE Fp);


#endif
