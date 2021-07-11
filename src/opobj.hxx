/************************************************************************
************************************************************************/

/*@@@
File:		opobj.hxx
Version:	1.00
Description:	Class OPOBJ - Operand/operator Base Class
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

#ifndef OPOBJ_HXX
#define OPOBJ_HXX 1

#include "defs.hxx"
#include "string.hxx"
#include "attrlist.hxx"
#include "iresult.hxx"
#include "idbobj.hxx"

// Op Types
typedef enum { TypeOperand = 1, TypeOperator = 2 } t_OpType;

// Operand Types
typedef enum { TypeNil, TypeTerm, TypeRset } t_Operand;

// Operator Types
typedef enum {
// Error Operator
  OperatorERR = -1,
// 0-ary Operator (Bad Operator)
  OperatorNoop = 0,
// Unary Operators
  OperatorNOT, 
  OperatorWithin,
  OperatorXWithin,
  OperatorInside,
  OperatorInclusive,
  OperatorSibling,
  OperatorNotWithin,  // Like NOT but to field
  OperatorReduce,     // Trim set
  OperatorHitCount,   // Trim set by hits
  OperatorTrim,       // Trim set to max.
  OperatorWithinFile,
  OperatorWithinFileExtension,
  OperatorWithinDoctype,
  OperatorWithKey,
  OperatorSortBy,
  OperatorBoostScore,
// Binary Operators 
  OperatorOr, OperatorAnd, OperatorAndNot, OperatorXor, OperatorXnor,
  OperatorNotAnd, OperatorNor, OperatorNand,
  OperatorLT, OperatorLTE, OperatorGT, OperatorGTE,
  OperatorJoin, OperatorJoinL, OperatorJoinR,
  OperatorProximity = 40, // In same level- adj, near, same, with
  OperatorBefore,
  OperatorAfter,
  OperatorAdj,
  OperatorFollows,
  OperatorPrecedes,
  OperatorNear,
  OperatorFar,
  OperatorNeighbor,
  OperatorAndWithin,
  OperatorOrWithin,
  OperatorBeforeWithin,
  OperatorAfterWithin,
  OperatorPeer,
  OperatorBeforePeer,
  OperatorAfterPeer,
  OperatorXPeer,
// Special Unary operator (that mimic a term search)
  OperatorKey,
  OperatorFile,
} t_Operator;

inline GDT_BOOLEAN IsBinaryOperator(t_Operator Op) { return Op >= OperatorOr; }
inline GDT_BOOLEAN IsUnaryOperator(t_Operator Op)   { return Op > OperatorNoop && Op < OperatorOr;  }

class OPOBJ {
friend class OPSTACK;
public:
  typedef enum {BEFOREorAFTER, BEFORE, AFTER} DIR_T;

  OPOBJ() {; }
  virtual t_OpType GetOpType () const = 0;
  virtual t_Operand GetOperandType () const = 0;
  virtual OPOBJ* Duplicate () const = 0;
  virtual OPOBJ& operator =(const OPOBJ&) = 0;
  virtual void SetAttributes (const ATTRLIST&) { }
  virtual void GetAttributes (PATTRLIST) const { }
  virtual void SetTerm (const STRING&)  { }
  virtual void GetTerm (STRING *) const { }
  virtual const STRING& GetTerm () const { return NulString; }
  virtual void SetOperatorType (const t_Operator) { }
  virtual t_Operator GetOperatorType () const { return OperatorNoop; }
  virtual void SetOperatorMetric (const FLOAT) { }
  virtual FLOAT GetOperatorMetric () const { return 0; }
  virtual void SetOperatorString(const STRING&) { }
  virtual STRING GetOperatorString() const { return NulString; }
  virtual size_t GetTotalEntries () const { return 0; }
//virtual void AddEntry (const IRESULT&, const INT) { }
  virtual GDT_BOOLEAN GetEntry (const size_t, PIRESULT) const { return GDT_FALSE; }

  virtual OPOBJ *Or (const OPOBJ&)        { return NULL; }
  virtual OPOBJ *Nor (const OPOBJ&)       { return NULL; }
  virtual OPOBJ *And (const OPOBJ&)       { return NULL; }
  virtual OPOBJ *Join (const OPOBJ&)      { return NULL; }
  virtual OPOBJ *Join (OPOBJ *)           { return NULL; }
  virtual OPOBJ *Nand (const OPOBJ&)      { return NULL; }
  virtual OPOBJ *AndNot (const OPOBJ&)    { return NULL; }
  virtual OPOBJ *Within(const OPOBJ&, const STRING&) { return NULL; }
  virtual OPOBJ *BeforeWithin(const OPOBJ&, const STRING&) { return NULL; }
  virtual OPOBJ *AfterWithin(const OPOBJ&, const STRING&) { return NULL; }

  virtual OPOBJ *WithinFile(const STRING&) { return NULL; }
  virtual OPOBJ *WithKey(const STRING&)    { return NULL; }
  virtual OPOBJ *WithinDoctype(const STRING&){ return NULL; }

  virtual OPOBJ *Xor (const OPOBJ&)       { return NULL; }
  virtual OPOBJ *Near (const OPOBJ&)      { return NULL; }
  virtual OPOBJ *CharProx(const OPOBJ&, FLOAT,
	 DIR_T dir = BEFOREorAFTER) { return NULL; }
  virtual OPOBJ *Far (const OPOBJ&)       { return NULL; }
  virtual OPOBJ *After (const OPOBJ&)     { return NULL; }
  virtual OPOBJ *Before (const OPOBJ&)    { return NULL; }
  virtual OPOBJ *Adj (const OPOBJ&)       { return NULL; }
  virtual OPOBJ *Follows (const OPOBJ&)   { return NULL; }
  virtual OPOBJ *Precedes (const OPOBJ&)  { return NULL; }
  virtual OPOBJ *Neighbor (const OPOBJ&)  { return NULL; }
  virtual OPOBJ *Peer (const OPOBJ&)      { return NULL; }
  virtual OPOBJ *AfterPeer (const OPOBJ&) { return NULL; }
  virtual OPOBJ *BeforePeer (const OPOBJ&){ return NULL; }
  virtual OPOBJ *XPeer (const OPOBJ&)     { return NULL; }

  virtual OPOBJ *Not ( )                  { return NULL; }
  virtual OPOBJ *Not (const STRING&)      { return NULL; }
  virtual OPOBJ *Within (const STRING&)   { return NULL; }
  virtual OPOBJ *Inside (const STRING&)   { return NULL; }
  virtual OPOBJ *Inclusive (const STRING&){ return NULL; }
  virtual OPOBJ *XWithin (const STRING&)  { return NULL; }
  virtual OPOBJ *Sibling ()               { return NULL; }

  virtual OPOBJ *BoostScore (const float Weight) { return NULL; }

  virtual OPOBJ *Reduce (const float metric) { return NULL; }
  virtual OPOBJ *Trim (const float Metric)   { return NULL; }
  virtual OPOBJ *HitCount (const float Metric) { return NULL; }

  virtual OPOBJ *SortBy(const STRING&)    {return NULL; }

  virtual void SetParent(PIDBOBJ const) { }
  virtual PIDBOBJ GetParent() const { return 0; }
  virtual INT GetSort() const { return 0; }
  virtual DOUBLE GetMaxScore () const {return 0;}
  virtual DOUBLE GetMinScore () const {return 0;}

  virtual GDT_BOOLEAN IsEmpty() const { return GDT_TRUE; }

  virtual ~OPOBJ();

private:
  void SetNext (OPOBJ* const OpPtr) { Next = OpPtr; }
  OPOBJ* GetNext() const            { return Next;  }
  OPOBJ* Next;
};

typedef OPOBJ* POPOBJ;

#endif
