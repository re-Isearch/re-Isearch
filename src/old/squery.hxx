/*@@@
File:		squery.hxx
Version:	1.00
Description:	Class SQUERY - Search Query
Author:		Nassib Nassar, nrn@cnidr.org
Modifications:	Edward C. Zimmermann, edz@bsn.com
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

  GDT_BOOLEAN Equals(const SQUERY& Other);

  // Is the query plain? No fields, rsets or operators other than OR?
  GDT_BOOLEAN isPlainQuery (t_Operator Operator = OperatorOr) const;
  GDT_BOOLEAN isPlainQuery (STRING *Words, t_Operator Operator = OperatorOr) const;
  GDT_BOOLEAN isPlainQuery (STRLIST *Words, t_Operator Operator = OperatorOr) const;


  // Are all the Ops the same? Example isOpQUery(OperatorAnd) for all AND
  GDT_BOOLEAN isOpQuery (const t_Operator Operator) const;

  // Query is all ANDs, PEERs etc. But no ORs
  GDT_BOOLEAN isIntersectionQuery() const;

  // Covert the Ors in plain query to use Operator
  // example AND:field etc.
  GDT_BOOLEAN SetOperator(const OPERATOR& Operator);
  // Convienience of above, set to AND:<FieldName> or Peer
  GDT_BOOLEAN SetOperatorAndWithin(const STRING& FieldName);
  GDT_BOOLEAN SetOperatorNear(); // Near or Peer with Fielded
  GDT_BOOLEAN SetOperatorPeer();
  GDT_BOOLEAN SetOperatorOr();

  // Set Or and Push REDUCE
  GDT_BOOLEAN PushReduce(int Reduce=0);
  GDT_BOOLEAN SetOperatorOrReduce(int Reduce = 0);

  GDT_BOOLEAN PushUnaryOperator(const OPERATOR& UnaryOperator);

  // Replace means don't add to existing attributes   
  size_t AddAttributes(const ATTRLIST& Attrlist) {
    return SetAttributes(Attrlist, GDT_FALSE);
  }
  size_t SetAttributes(const ATTRLIST& Attrlist, GDT_BOOLEAN Replace=GDT_TRUE);

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

  size_t PhraseToProx (const STRING& Term, const STRING& Field = NulString,
	GDT_BOOLEAN LeftTruncated = GDT_FALSE, GDT_BOOLEAN Case = GDT_FALSE);

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
  GDT_BOOLEAN haveThesaurus() const; 

  void ExpandQuery();

  STRING LastErrorMessage() const { return ErrorMessage; }

  friend ostream& operator <<(ostream& os, const SQUERY& str);
  friend istream& operator >>(istream& os, SQUERY& str);


  void Write (PFILE Fp) const;
  GDT_BOOLEAN Read (PFILE Fp);

  ~SQUERY();
private:
  size_t      SetTerm(const STRING&, GDT_BOOLEAN);
  t_Operator  GetOperator (const STRING& Operator, FLOAT *Metric = NULL,
	STRING *StringArgs = NULL) const;
  size_t      fetchTerm(PSTRING StringBuffer, GDT_BOOLEAN WantRpn) const;
  OPSTACK     Opstack;
  THESAURUS  *Thesaurus;
  GDT_BOOLEAN expanded;
  UINT8       Hash;
  STRING      ErrorMessage;
};

typedef SQUERY* PSQUERY;

void        Write(const SQUERY& SQuery, PFILE Fp);
GDT_BOOLEAN Read(PSQUERY SQueryPtr, PFILE Fp);


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

  GDT_BOOLEAN Ok() { return Run() == 0; }
  int         Run();


  operator enum NormalizationMethods() const { return Method; }
  operator enum SortBy() const { return Sort; }
  operator SQUERY() const { return Squery; }
  operator STRING() const { return Squery.GetRpnTerm(); }

  size_t        GetTotalTerms() const { return Squery.GetTotalTerms(); }
  GDT_BOOLEAN   isPlainQuery (STRING *Words = NULL) { return Squery.isPlainQuery(Words); }

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
  GDT_BOOLEAN   isUnlimited() const {  return MaxResults == 0 || MaxResults > 0x7fffffff ; }

  void        Write (PFILE Fp) const;
  GDT_BOOLEAN Read  (PFILE Fp);

private:
  SQUERY                    Squery;
  enum SortBy               Sort;
  enum NormalizationMethods Method;
  size_t                    MaxResults;
};


void        Write(const QUERY& Query, PFILE Fp);
GDT_BOOLEAN Read(QUERY *QueryPtr, PFILE Fp);


#endif
