/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */
#pragma ident  "@(#)squery.cxx"
/* ########################################################################

   File: squery.cxx
   Description: Class SQUERY - Search Query

   ########################################################################

   Note: None

   ########################################################################

  This software was based upon a concept from CNIDR substantially extended,
  expanded and modified first by BSn then NONMONOTONIC Networks and then open
  sourced into the re-Isearch Project.

   Copyright (c) 2020 : NOMNONOTONIC Networks and the re-iSearch project
   Copyright (c) 1995,1996 : Basis Systeme netzerk. All Rights Reserved.
   Copyright (c) MCNC, Clearinghouse for Networked Information Discovery and
   Retrieval, 1994.

   ######################################################################## */

#include <ctype.h>
#include "common.hxx"
#include "sterm.hxx"
#include "attrlist.hxx"
#include "squery.hxx"
#include "operator.hxx"
#include "infix2rpn.hxx"
#if 1
# include "vidb.hxx"
# include <string.h>
#endif
#include "magic.hxx"
#include "../doctype/doc_conf.hxx"

#ifdef  SVR4
# include <alloca.h>
#endif


#define   NAMESPACE
#define CQL 1 /* Rule is that terms always have quotes */

SQUERY::SQUERY ()
{
  Thesaurus       = (THESAURUS *)NULL;
  expanded        = false;
  Hash            = 0;
}

SQUERY::SQUERY(THESAURUS *ptr)
{
  Thesaurus  = ptr;
  expanded   = false;
  Hash       = 0;
}

SQUERY::SQUERY (const STRING& Term, THESAURUS *ptr)
{
  Thesaurus = ptr;
  expanded  = false;
  Hash      = 0;
  *this     = Term;
}


SQUERY::SQUERY(const STRING& QueryTerm, enum QueryTypeMethods Typ, THESAURUS *ptr)
{
  Thesaurus       = ptr;
  expanded        = false;
  Hash            = 0;
  STRING   query ( QueryTerm.Strip(STRING::both) );

  Hash  = 0;
  if (query.IsEmpty())
    {
      Opstack.Clear();
    }
  else switch(Typ)
    {
      default:
      case QueryAutodetect: SetQueryTerm(query); break;
      case QueryRPN:        SetRpnTerm(query); break;
      case QueryInfix:      SetInfixTerm(query); break;
      case QueryRelevantId: SetRelevantTerm(query); break;
    }
}


bool SQUERY::haveThesaurus() const
{
  return  Thesaurus != NULL && Thesaurus->Ok();
}


SQUERY& SQUERY::operator =(const SQUERY& OtherSquery)
{
  Opstack         = OtherSquery.Opstack;
  Thesaurus       = OtherSquery.Thesaurus;
  expanded        = OtherSquery.expanded;
  Hash            = OtherSquery.Hash;
  return *this;
}

SQUERY& SQUERY::operator =(const STRING& OtherQueryTerm)
{
  expanded = false;

//cerr << "SQUERY::= \"" << OtherQueryTerm << "\"" << endl;

  STRING   query ( OtherQueryTerm.Strip(STRING::both) );

  Hash  = 0;
  if (query.IsEmpty())
    {
      Opstack.Clear();
    }
  else
    {
      SetQueryTerm(query);
      ExpandQuery();
    }
  return *this;
}


// edz: added
SQUERY& SQUERY::operator +=(const SQUERY& OtherSquery)
{
  return Cat(OtherSquery, OperatorOr);
}


// edz: added
SQUERY& SQUERY::operator *=(const SQUERY& OtherSquery)
{
  return Cat(OtherSquery, OperatorAnd);
}

// edz: added
SQUERY& SQUERY::operator -=(const SQUERY& OtherSquery)
{
  return Cat(OtherSquery, OperatorAndNot);
}

static const char SOperatorNoop[]     = "><";
static const char SOperatorPeer[]     = ".=.";
static const char SOperatorNOT[]      = "!";
static const char SOperatorOr[]       = "||";
static const char SOperatorAnd[]      = "&&";
static const char SOperatorAndNot[]   = "!!";
static const char SOperatorNotAnd[]   = "!&";
static const char SOperatorXor[]      = "^^";
static const char SOperatorXnor[]     = "^!";
static const char SOperatorNor[]      = "|!";
static const char SOperatorNand[]     = "&!";
static const char SOperatorLT[]       = "<";
static const char SOperatorLTE[]      = "<=";
static const char SOperatorGT[]       = ">";
static const char SOperatorGTE[]      = ">=";
static const char SOperatorBefore[]   = ".#";
static const char SOperatorAfter[]    = "#.";
static const char SOperatorAdj[]      = "##";
static const char SOperatorFollows[]  = "#>";
static const char SOperatorPrecedes[] = "#<";
static const char SOperatorNear[]     = ".<.";
static const char SOperatorFar[]      = ".>.";
static const char SOperatorNeighbor[] = ".~.";
static const char SOperatorNE[]       = "<>";

static const char SOperatorXPeer[]      = "XPEER";
static const char SOperatorBeforePeer[] = "PEERb";
static const char SOperatorAfterPeer[]  = "PEERa";
static const char SOperatorJoin[]       = "JOIN";
static const char SOperatorJoinL[]      = "JOINl";
static const char SOperatorJoinR[]      = "JOINr";

/*

  prox/relation/distance/unit/ordering

  prox/<=/1/word/unordered
  prox/<=/1/word
  prox/<=/1
  prox/<=
  prox

means
  prox -> Adj
  prox//nn/char -> proximity:nn

  prox///sentence -> AND:sentence
  prox///element  -> PEER

  prox/>/4/char/ordered -> AFTER:4
  prox/</4/char/ordered -> BEFORE:4


*/


// Operator(%d)  where d is a number --> metric, else string
t_Operator SQUERY::GetOperator (const STRING& Operator, FLOAT *Metric, STRING *StringArgs) const
{
    // Expand this table as more operators are added
    const struct
      {
        const char *name;
        t_Operator code;
      } Ops[] =
    {
      { "OR",          OperatorOr },
      {  SOperatorOr,  OperatorOr },
      { "AND",     OperatorAnd },
      { SOperatorAnd,  OperatorAnd },
      { "ANDNOT",  OperatorAndNot },
      { SOperatorAndNot,OperatorAndNot },
      { "NOTAND",  OperatorNotAnd }, // A B NOTAND := B A ANDNOT
      { "!&",      OperatorNotAnd },
      { "NOT",     OperatorNOT },
      { SOperatorNOT, OperatorNOT },
      { "XOR",     OperatorXor },
      { SOperatorXor,  OperatorXor },
      { SOperatorGT,   OperatorGT},
      { SOperatorGTE,  OperatorGTE},
      { SOperatorJoin,   OperatorJoin },
      { SOperatorJoinL,  OperatorJoinL },
      { SOperatorJoinR,  OperatorJoinR },
      { SOperatorLT,   OperatorLT},
      { SOperatorLTE,  OperatorLTE},
      { SOperatorNeighbor, OperatorNeighbor },
      { "NEIGHBOR",    OperatorNeighbor },
      { "PEER",        OperatorPeer},
      { SOperatorPeer, OperatorPeer},
      { SOperatorBeforePeer,     OperatorBeforePeer},
      { SOperatorAfterPeer,      OperatorAfterPeer},
      { SOperatorXPeer,       OperatorXPeer},
      { "SIBLING",     OperatorSibling},
      { "PROX",        OperatorProximity},
      { "PROXIMITY",   OperatorProximity},
      { SOperatorNear, OperatorNear },
      { "NEAR",    OperatorNear },
      { "BEFORE",  OperatorBefore },
      { SOperatorBefore,      OperatorBefore },
      { "AFTER",   OperatorAfter },
      { SOperatorAfter,      OperatorAfter },
      { "ADJ",	OperatorAdj },
      { SOperatorAdj,	OperatorAdj },
      { "FOLLOWS", OperatorFollows },
      { SOperatorFollows, OperatorFollows },
      { "PRECEDES", OperatorPrecedes },
      { SOperatorPrecedes, OperatorPrecedes },
      { "FAR",	OperatorFar },
      { SOperatorFar,     OperatorFar },
      { "NAND",    OperatorNand }, // A B NAND := A B AND NOT
      { SOperatorNand,      OperatorNand },
      { "NOR",     OperatorNor },  // A B NOR := A B OR NOT
      { SOperatorNor,   OperatorNor },
      { SOperatorXnor,  OperatorXnor }, // A B XNOR := A B XOR NOT
      { "XNOR",    OperatorXnor },
// Special Cases for the Infix Processor
      { "!&&",     OperatorNand }, // Added
      { "!||",     OperatorNor  }, // Added
      { "!|!",     OperatorOr   }, // A B OR NOT NOT --> A B OR
      { "!&!",     OperatorAnd  }, // A B AND NOT NOT --> A B AND
// Last stuff
      { "<noop>",  OperatorNoop }, //  A B NOOP OP := A B OP
      { SOperatorNoop,  OperatorNoop}
    };

    if (Metric) *Metric = 0;
    if (StringArgs) StringArgs->Clear();
    for (size_t i=0; i<sizeof(Ops)/sizeof(Ops[0]); i++)
      {
        // Case DEPENDENT
        if (Operator == Ops[i].name)
          {
            switch (Ops[i].code)
              {
                // Special case is Proximity
                // PROX -> NEAR
                // but PROX:0 -> PROX:0 -> ADJ
              case OperatorProximity:
                //cerr << "Special case Prox => Near " << endl;
                return OperatorNear;
              default: break; // Should not get here
              }
            return Ops[i].code;
          }
      }

// Handle OPERATOR:arg
    const size_t arg_pos = Operator.Search(':');
if (arg_pos)
    switch ( arg_pos )
      {
      case 3:
        if (Operator.Compare("OR", 2) == 0)
          {
            if (StringArgs) *StringArgs = Operator.c_str() + 3;
            return OperatorOrWithin;
          }
        break;
      case 4:
        if (Operator.Compare("AND",  3) == 0)
          {
            if (StringArgs) *StringArgs = Operator.c_str() + 4;
            return OperatorAndWithin;
          }
        if (Operator.Compare("NOT",  3) == 0)
          {
            if (StringArgs) *StringArgs = Operator.c_str() + 4;
            return OperatorNotWithin;
          }

	// For now the operator KEY does ABSOLUTELY NOTHING!!!!
	// --> map to WITHKEY 
        if (Operator.Compare("KEY", 3) == 0)
          {
            if (StringArgs)
              {
                *StringArgs = Operator.c_str() + 4;
                if (StringArgs->GetLength())
                  return OperatorWithKey; // OperatorKey;
              }
          }
        break;
// PEER
      case 5:
        if (Operator.Compare("PEER", 4) == 0)
          {
            if (Metric)
              *Metric = atof ( Operator.c_str() + 5 );
            return OperatorPeer;
          }
        if (Operator.Compare("NEAR", 4) == 0)
          {
            if (Metric)
              {
                const char *ptr = Operator.c_str() +5;
                *Metric = atof ( ptr );
                if (Operator[(unsigned)(Operator.GetLength()-1)] == '%')
                  *Metric /= 100.0;
              }
            return OperatorNear;
          }
        if (Operator.Compare("PROX", 4) == 0)
          {
            if (Metric)
              {
                const char *ptr = Operator.c_str() +10;
                *Metric = atof ( ptr );
                if (Operator[(unsigned)(Operator.GetLength()-1)] == '%')
                  *Metric /= 100.0;
              }
            return OperatorProximity;
          }
        else if (Operator.Compare("FILE", 4) == 0)
          {
            if (StringArgs)
              {
                *StringArgs = Operator.c_str() + 5;
                if (StringArgs->GetLength())
                  {
                    return OperatorWithinFile;
                  }
              }
          }
        else if (Operator.Compare("TRIM", 4) == 0)
          {
            const char   *ptr = Operator.c_str() + 5;
            DOUBLE metric = atof (  ptr );
            if ((metric <= 0 && !isdigit(*ptr)) || (DOUBLE)((int)(metric)) != metric)
              return OperatorERR; // Bad metric
            if (Metric) *Metric = metric;
            return OperatorTrim;
          }
        break;
      case 6:
       if (Operator.Compare("BOOST", 5) == 0)
          {
            const char   *ptr = Operator.c_str() + 6;
            DOUBLE metric = atof (  ptr );
            if (metric == 0 && !isdigit(*ptr))
              return OperatorERR; // Bad metric
            if (Metric) *Metric = metric;
            return OperatorBoostScore;
          }
        if (Operator.Compare("XPEER", 5) == 0)
          {
            if (Metric)
              *Metric = atof ( Operator.c_str() + 6);
            return OperatorXPeer;
          }
	if (Operator.Compare("ONEAR",  5) == 0)
	  {
	    // Really just an alias of BEFORE (ORDER'd NEAR)
	    if (StringArgs)
	      {
		*StringArgs = Operator.c_str() + 6;
		if (StringArgs->GetLength() && !StringArgs->IsNumber())
                  return OperatorBeforeWithin;
	      }
	    if (Metric)
	      {
		*Metric = atof ( Operator.c_str() + 6 );
		if (Operator[(unsigned)(Operator.GetLength()-1)] == '%')
		  *Metric /= 100.0;
	      }
	    return OperatorBefore;
	  }
        if (Operator.Compare("AFTER",  5) == 0)
          {
            if (StringArgs)
              {
                *StringArgs = Operator.c_str() + 6;
                if (StringArgs->GetLength() && !StringArgs->IsNumber())
                  return OperatorAfterWithin;
              }
            if (Metric)
              {
                *Metric = atof ( Operator.c_str() + 6 );
                if (Operator[(unsigned)(Operator.GetLength()-1)] == '%')
                  *Metric /= 100.0;
              }
            return OperatorAfter;
          }
        break;
      case 7:
        if (Operator.Compare("BEFORE",  6) == 0)
          {
            if (StringArgs)
              {
                *StringArgs = Operator.c_str() + 7;
                if (StringArgs->GetLength() && !StringArgs->IsNumber())
                  return OperatorBeforeWithin;
              }
            if (Metric)
              {
                *Metric = atof ( Operator.c_str() + 7 );
                if (Operator[(unsigned)(Operator.GetLength()-1)] == '%')
                  *Metric /= 100.0;
              }
            return OperatorBefore;
          }
        else if (Operator.Compare("WITHIN",  6) == 0)
          {
            if (StringArgs) *StringArgs = Operator.c_str() + 7;
            return OperatorWithin;
          }
        else if (Operator.Compare("INSIDE",  6) == 0)
          {
            if (StringArgs) *StringArgs = Operator.c_str() + 7;
            return OperatorInside;
          }
        else if (Operator.Compare("REDUCE", 6) == 0)
          {
            const char   *ptr = Operator.c_str() + 7;
            DOUBLE metric = atof (  ptr );
            if (metric < 0 || (metric == 0 && !isdigit(*ptr)) || (DOUBLE)((int)(metric)) != metric)
              return OperatorERR; // Bad metric
            if (Metric) *Metric = metric;
            return OperatorReduce;
          }
        else if (Operator.Compare("SORTBY", 6) == 0)
          {
	    const char *ptr =  Operator.c_str() + 7;

	    // Skip By as in ByScore
	    if ((ptr[0] == 'b' || ptr[0] == 'B') && (ptr[1] == 'y' || ptr[1] == 'Y'))
	       ptr += 2;

	    const STRING val ( ptr );
 	    if (StringArgs) *StringArgs = val; 

	    // edz: 2021 Adding hook to allow for ByPrivate in the query
	    // SORTBY:ByPrivate
            if (strncasecmp(ptr, "private", 7) == 0)
                return OperatorSortBy;

	    const char * const keywords[] = {
		"Key", "Hits", "Date", "Index", "Score", "AuxCount", "Newsrank",
		"Function", "Category", "ReverseHits", "ReverseDate", NULL};
		// What about a reverse score?

	    if (val.IsNumber()) return OperatorSortBy;
	    for (size_t i=0; keywords[i]; i++)
	      {
		if (val ^= keywords[i])
		  return OperatorSortBy; // OK
	      }
	    return OperatorERR; // Bad value
          }
        break;
      case 8:
	if (Operator.Compare("SIBLING",  7) == 0)
	  {
	    if (StringArgs) *StringArgs = Operator.c_str() + 8;
	    return OperatorInside;
	  }
        if (Operator.Compare("XWITHIN",  7) == 0)
          {
            if (StringArgs) *StringArgs = Operator.c_str() + 8;
            return OperatorXWithin;
          }
        else if (Operator.Compare("DOCTYPE", 7) == 0)
          {
            const char *ptr = Operator.c_str() + 8;
            if (*ptr && StringArgs)
              {
                *StringArgs = ptr;
                return OperatorWithinDoctype;
              }
          }
	else if (Operator.Compare("WITHKEY", arg_pos-1) == 0)
	  {
	    if (StringArgs)
	      {
		*StringArgs = Operator.c_str() + arg_pos;
		if (StringArgs->GetLength())
		  return OperatorWithKey;
	      }
	  }
	break;

      case 9:
        if (Operator.Compare("HITCOUNT", 8) == 0)
          {
            const char   *ptr = Operator.c_str() + 9;
            DOUBLE metric = atof (  ptr );
            if ((metric == 0 && !isdigit(*ptr)) || (DOUBLE)((int)(metric)) != metric)
              return OperatorERR; // Bad metric
            if (Metric) *Metric = metric;
            return OperatorHitCount;
          }
	break;

      case 10:
	if (Operator.Compare("INCLUSIVE", 9) == 0)
	  {
	    if (StringArgs) *StringArgs = Operator.c_str() + 10;
	      return OperatorInclusive;
	  }
        else if (Operator.Compare("EXTENSION", 9) == 0)
          {
            const char *ptr = Operator.c_str() + 10;
            if (*ptr == '.') ptr++;
            if (*ptr && StringArgs)
              {
                *StringArgs = ptr;
                return OperatorWithinFileExtension;
              }
          }
        else if (Operator.Compare("PROXIMITY", 9) == 0)
          {
            if (Metric)
              {
                const char *ptr = Operator.c_str() +10;
                *Metric = atof ( ptr );
                if (Operator[(unsigned)(Operator.GetLength()-1)] == '%')
                  *Metric /= 100.0;
              }
            return OperatorProximity;
          }
      }

  size_t pos = Operator.Search(">=");
  if (pos == 0)
    {
      if ((pos = Operator.Search("<=")) == 0)
	{
	  if ((pos = Operator.Search('<')) == 0)
	    pos = Operator.Search('>');
	}
    }
  if (pos)
    {
      const int offset = Operator.GetChr(pos+1) == '=' ? 1 : 0;
      switch (pos) {
	case 5:
	  if (Operator.Compare("PROX", 4) == 0 || Operator.Compare("NEAR", 4) == 0)
	    {
  	       // prox>4 -> AFTER:4 prox<4 -> BEFORE:4 
	       const char   *ptr = Operator.c_str() + 5 + offset;
	       DOUBLE metric = (atof (  ptr ) + offset ) * (Operator.GetChr(5) == '<' ? -1 : 1 );
	       if (metric < 0)
		{
		  if (Metric) *Metric = -metric;
		  return OperatorBefore;
		}
	       if (Metric) *Metric = metric;
	       return OperatorAfter;
	     }
          if (Operator.Compare("DIST", 4) == 0)
            {
               const char   *ptr = Operator.c_str() + 5 + offset;
               DOUBLE metric = (atof (  ptr ) + offset ) * (Operator.GetChr(5) == '>' ? -1 : 1 );
               if (Metric) *Metric = metric;
               return OperatorProximity;
             }
	   break;
	case 9:
	  if (Operator.Compare("HITCOUNT", 8) == 0)
	    {
	      const char   *ptr = Operator.c_str() + 9 + offset;
	      DOUBLE metric = (atof (  ptr ) + (offset == 0 ? 1 : 0 )) * (Operator.GetChr(9) == '<' ? -1 : 1 );
	      if ((metric == 0 && !isdigit(*ptr)) || (DOUBLE)((int)(metric)) != metric)
		return OperatorERR; // Bad metric

	      if (Metric) *Metric = metric;
	      return OperatorHitCount;
	    }
	default:
	  break;
      }
    }

  // Not an op
  return OperatorERR;
}


// edz: Added Concatination
SQUERY& SQUERY::Cat(const SQUERY& OtherSquery, const STRING& Opname)
{
  t_Operator Op = GetOperator(Opname);
  if (Op == OperatorERR)
    message_log (LOG_PANIC, "SQUERY::Cat : Operator %s undefined!", Opname.c_str());
  Cat(OtherSquery, Op);
  return *this;
}

SQUERY& SQUERY::Cat(const SQUERY& OtherSquery, t_Operator OpGlue)
{
  // Cat to an empty query?
  if (Opstack.IsEmpty())
    {
      // just replace it
      *this = OtherSquery;
      return *this;
    }

  // Cat query
  POPOBJ OpPtr;
  STERM Sterm;
  OPERATOR Operator;
  ATTRLIST Attrlist;
  STRING S;

  // Push the Other Queries stack onto ours
  OPSTACK MyStack = Opstack;
  MyStack.Reverse ();

  OPSTACK OtherStack;
  OtherSquery.GetOpstack (&OtherStack);
  OtherStack.Reverse ();
  OPSTACK NewOpstack;

  Hash = 0;
  while (OtherStack >> OpPtr)
    {
      if (OpPtr->GetOpType () == TypeOperator)
        {
          Operator.SetOperatorType (OpPtr->GetOperatorType ());
          Operator.SetOperatorMetric (OpPtr->GetOperatorMetric ());
          Operator.SetOperatorString (OpPtr->GetOperatorString () );
          NewOpstack << Operator;
        }
      if (OpPtr->GetOpType () == TypeOperand)
        {
          Sterm.SetTerm ( OpPtr->GetTerm () );
          OpPtr->GetAttributes (&Attrlist);
          Sterm.SetAttributes (Attrlist);
          NewOpstack << Sterm;
        }
    }
  while (MyStack >> OpPtr)
    {
      if (OpPtr->GetOpType () == TypeOperator)
        {
          Operator.SetOperatorType (OpPtr->GetOperatorType ());
          Operator.SetOperatorMetric (OpPtr->GetOperatorMetric ());
          Operator.SetOperatorString ( OpPtr->GetOperatorString() );
          NewOpstack << Operator;
        }
      if (OpPtr->GetOpType () == TypeOperand)
        {
          Sterm.SetTerm ( OpPtr->GetTerm () );
          OpPtr->GetAttributes (&Attrlist);
          Sterm.SetAttributes (Attrlist);
          NewOpstack << Sterm;
        }
    }
  Operator.SetOperatorType (OpGlue);
  NewOpstack << Operator;
  Opstack = NewOpstack;
  return *this;
}

void SQUERY::SetOpstack (const OPSTACK& NewOpstack)
{
  Hash    = 0;
  Opstack = NewOpstack;
}

void SQUERY::GetOpstack (OPSTACK *OpstackBuffer) const
  {
    *OpstackBuffer = Opstack;
  }

bool SQUERY::PushUnaryOperator(const OPERATOR& UnaryOperator)
{
  t_Operator Op =  UnaryOperator.GetOperatorType();
  if (Op < OperatorOr)
    {
      OPSTACK  Stack;
      GetOpstack (&Stack);
      Stack << UnaryOperator;
      Opstack = Stack;
      return true;
    }
  return false;
}


bool SQUERY::SetOperator(const OPERATOR& BinaryOperator)
{
  OPSTACK  Stack;
  OPSTACK  NewOpstack;
  POPOBJ   OpPtr;
  size_t   count = 0;

  GetOpstack (&Stack);
  Stack.Reverse (); // BUGFIX Maybe...
  while (Stack >> OpPtr)
    {
      if (OpPtr->GetOpType () == TypeOperator)
        {
          // We only change binary operators!
          if (OpPtr->GetOperatorType () >=  OperatorOr)
            {
              count++;
              NewOpstack << BinaryOperator;
              delete OpPtr;
              continue;
            }
        }
      NewOpstack << *OpPtr;
      delete OpPtr;
    }
  if (count)
    {
      Opstack = NewOpstack;
      return true;
    }
  return false;
}


// Replace means don't add to existing attributes
size_t SQUERY::SetAttributes(const ATTRLIST& Attrlist, bool Replace)
{

  OPSTACK  Stack, newOpstack;
  POPOBJ   OpPtr;
  ATTRLIST newAttrlist;
  size_t   count = 0;

  GetOpstack (&Stack);
  Stack.Reverse (); // BUGFIX Maybe...
  while (Stack >> OpPtr)
    {
      if (OpPtr->GetOpType () == TypeOperand)
        {
          if (Replace)
            {
              newAttrlist = Attrlist;
            }
          else
            {
              OpPtr->GetAttributes(&newAttrlist);
              newAttrlist.Cat(Attrlist);
            }
          count++;
          OpPtr->SetAttributes(Attrlist);
        }
      newOpstack << *OpPtr;
      delete OpPtr;
    }
  Opstack = newOpstack;
  return count;
}


class __STOPLIST : public LISTOBJ
  {
  public:
    // Creation
    __STOPLIST () {;}

    // Words stream in list?
    virtual bool InList (const UCHR* Word) const  {return strlen((const char *)Word)<3; }
  };


size_t SQUERY::SetLiteralPhrase()
{
  STRING S;
  if (isPlainQuery(&S))
    return SetLiteralPhrase(S);
  return 0;
}

size_t  SQUERY::SetLiteralPhrase(const STRING& QueryString)
{
  if (QueryString.IsEmpty()) return 0;

  ATTRLIST Attrlist;
  Attrlist.AttrSetPhrase (true);

  OPSTACK Stack;
  STERM Sterm;

  Sterm.SetTerm       (QueryString);
  Sterm.SetAttributes (Attrlist);

  Stack << Sterm;
  SetOpstack (Stack);
  return 1;
}


// ORed words, no field or other ops, truncated
size_t SQUERY::SetFreeFormWords(const STRING& Sentence, int Weight)
{
  ATTRLIST   Attrlist;
  __STOPLIST stop;
  Attrlist.AttrSetRightTruncation (true);
  Attrlist.AttrSetFreeForm (true);
  Attrlist.AttrSetTermWeight (Weight);
  return SetWords(Sentence, &Attrlist, &stop);
}
size_t SQUERY::SetFreeFormWordsPhonetic(const STRING& Sentence, int Weight)
{
  ATTRLIST   Attrlist;
  __STOPLIST stop;
  Attrlist.AttrSetFreeForm (true);
  Attrlist.AttrSetPhonetic(true);
  Attrlist.AttrSetTermWeight (Weight);
  return SetWords(Sentence, &Attrlist, &stop);
}

size_t SQUERY::SetWords(const STRING& Sentence, ATTRLIST *AttrlistPtr, const LISTOBJ *Stopwords)
{
  if (AttrlistPtr == NULL)
    return SetFreeFormWords(Sentence, 1);

  STRLIST Words;
  Words.SplitWords(Sentence, Stopwords);

  OPERATOR Operator;
  Operator.SetOperatorType (OperatorOr);

  INT      Weight= AttrlistPtr->AttrGetTermWeight();

  OPSTACK Stack;
  STERM   Sterm;
  size_t  count = 0;
  STRING  lastWord;
  int     factor = 1;

  const bool rightTrunc = AttrlistPtr->AttrGetRightTruncation();
  const bool leftTrunc  = AttrlistPtr->AttrGetLeftTruncation ();

  if (Weight == 0) Weight = 1;
  Words.Sort();
  for (const STRLIST *p = Words.Next(); p; p = p->Next())
    {
      bool Done = (p == &Words);

      if (!Done && (p->Value() ^= lastWord))
        {
          factor++;
          continue;
        }
      Sterm.SetTerm ( lastWord );
      AttrlistPtr->AttrSetTermWeight (factor*Weight);

      if ((rightTrunc | leftTrunc) && (lastWord.GetLength() < 4))
        {
          AttrlistPtr->AttrSetRightTruncation(false);
          AttrlistPtr->AttrSetLeftTruncation(false);
        }
      Sterm.SetAttributes (*AttrlistPtr);

      Stack << Sterm;
      // if this is not the first term, push an OR
      if (++count > 1)
        Stack << Operator;
      if (Done)
        break;
      lastWord = p->Value();
      factor = 1;
    }

  SetOpstack (Stack);

  return count;

}

// ORed words, no field or other ops
size_t SQUERY::SetWords (const STRING& NewTerm, INT Weight, t_Operator Op)
{
  STRLIST TermList;
  TermList.SplitWords(NewTerm);
 
  return SetWords (TermList, Weight, Op);
}


size_t SQUERY::SetWords (const STRING& Sentence, const OPERATOR& Operator, ATTRLIST *AttrlistPtr)
{
  STRLIST TermList;
  TermList.SplitWords(Sentence);
  return SetWords(TermList, Operator, AttrlistPtr);
}


size_t SQUERY::SetWords (const STRING& Sentence, const OPERATOR& Operator, int Weight)
{
  STRLIST TermList;
  TermList.SplitWords(Sentence);
  return SetWords(TermList, Operator, Weight);
}



size_t SQUERY::SetWords (const STRLIST& TermList, INT Weight, t_Operator Op)
{
  PATTRLIST AttrlistPtr = NULL;
  if (Weight != 1)
    {
      AttrlistPtr = new ATTRLIST (1);
      AttrlistPtr->AttrSetTermWeight (Weight);
    }
  size_t res = SetWords (TermList, Op, AttrlistPtr);
  if (AttrlistPtr)
    delete AttrlistPtr;
  return res;
}

size_t SQUERY::SetWords (const STRLIST& TermList, const STRING& ESet, t_Operator Op)
{
  return SetWords(TermList, ESet, 1, Op);
}

size_t SQUERY::SetWords (const STRLIST& TermList, const STRING& ESet, INT Weight, t_Operator Op)
{
  PATTRLIST AttrlistPtr = NULL;
  if (Weight != 1 || !ESet.IsEmpty())
    {
      AttrlistPtr = new ATTRLIST (2);
      AttrlistPtr->AttrSetTermWeight (Weight);
      if (!ESet.IsEmpty())
        AttrlistPtr->AttrSetFieldName (ESet); // All in one field
    }
  size_t res = SetWords (TermList, Op, AttrlistPtr);
  if (AttrlistPtr)
    delete AttrlistPtr;
  return res;
}

size_t SQUERY::SetWords (const STRLIST& TermList, t_Operator Op, PATTRLIST AttrlistPtr)
{
  return SetWords(TermList, OPERATOR(Op), AttrlistPtr);
}

size_t SQUERY::SetWords (const STRLIST& TermList, const OPERATOR& Operator, int Weight)
{
  ATTRLIST Attrlist;
  Attrlist.AttrSetTermWeight (Weight);
  return SetWords(TermList, Operator, &Attrlist);
}


size_t SQUERY::SetWords (const STRLIST& TermList, const OPERATOR& Operator, PATTRLIST AttrlistPtr)
{
  OPSTACK Stack;
  STERM Sterm;
  size_t count = 0;

 //  cerr << "OPERATOR " <<  Operator << endl;

  for (const STRLIST *p = TermList.Next(); p != &TermList; p = p->Next())
    {
      Sterm.SetTerm ( p->Value() );
      if (AttrlistPtr)
        Sterm.SetAttributes (*AttrlistPtr);
      Stack << Sterm;
      // if this is not the first term, push an OR
      if (++count > 1)
        Stack << Operator;
    }
  SetOpstack (Stack);

  return count;
}

// Alternative phrase heuristic..
size_t SQUERY::PhraseToProx(const STRING& Term, const STRING& Field,
                            bool RightTruncated, bool Case)
{
  OPSTACK  Stack;
  STERM    Sterm;
  ATTRLIST Attrlist;
  size_t   count = 0;
  int      last = 0;
  OPERATOR Operator;
  Operator.SetOperatorType (OperatorPrecedes);

  if (Case)
    Attrlist.AttrSetAlwaysMatches(Case);
  if (!Field.IsEmpty())
    Attrlist.AttrSetFieldName(Field);
  Sterm.SetAttributes (Attrlist);
  STRLIST TermList;
  TermList.Split(" ", Term);
  for (const STRLIST *p = TermList.Next(); !last; p = p->Next())
    {
      Sterm.SetTerm ( p->Value() );
      last = (p->Next() == &TermList);
      if (last && RightTruncated)
        {
          Attrlist.AttrSetRightTruncation(RightTruncated);
          Sterm.SetAttributes (Attrlist);
        }
      Stack << Sterm;
      // if this is not the first term, push an OR
      if (++count > 1)
        Stack << Operator;
    }
  SetOpstack (Stack);
  return count;
}

size_t SQUERY::SetQueryTermUTF(const STRING& UtfTerm)
{
  size_t len = UtfTerm.GetLength();
  if (len <= 1)
    return SetQueryTerm(UtfTerm);

#ifdef  SVR4
  UINT2 *buffer = (UINT2 *)alloca (sizeof(UINT2)*(len+1));
#else
  UINT2  buffer[len+1];
#endif
  STRING term;
  int    tlen = ::UTFTo(buffer, UtfTerm.c_str(), len);
  for (int i=0; i< tlen; i++)
    {
      UINT2  ch = buffer[i];
      term.Cat ((char)((ch <= 255 ? ch&0xFF : '?')));
    }
  return SetQueryTerm(term);
}


size_t SQUERY::SetQueryTerm(const STRING& NewTerm)
{
  size_t res = SetInfixTerm (NewTerm);
  if (res == 0)
    {
      // See if its got an operator?
      STRLIST TermList;
      TermList.SplitTerms(NewTerm);
      for (const STRLIST *p = TermList.Next(); p != &TermList; p = p->Next())
        {
          if (GetOperator(p->Value()) != OperatorERR)
            {
              // Got an operator.. Try RPN
              res = SetRpnTerm( NewTerm);
              break;
            }
        }
      if (res == 0)
        res = SetTerm(NewTerm);
    }
  return res;
}

// Or'd terms, support field, weight etc
size_t SQUERY::SetTerm (const STRING& NewTerm)
{
  return SetTerm (NewTerm, true);
}

// Set Term to contents of RelId
//
// RelId: DBFullPath//Key/Element:Weight
//
// Returns number of terms processed. 0 ==> Error
//
size_t SQUERY::SetRelevantTerm (const STRING& RelId)
{
  STRING DBname, RecordKey, ESet;
  INT weight = 1;

  if (RelId.GetLength())
    {
      char *element = NULL;
      char *url = RelId.NewCString();
      char *dbname = url;
      char *key = strstr (url, "//");
      if (key != NULL)
        {
          *key++ = '\0';
          if ((element = strchr (++key, '/')) != NULL)
            {
              char *tcp;
              *element++ = '\0';
              if ((tcp = strrchr (element, ':')) != NULL)
                {
                  *tcp++ = '\0';
                  weight = atoi(tcp);
                }
              ESet = element;
            }
          RecordKey = key;
        }
      DBname = dbname;
      delete[] url;
    }
  else
    {
      return 0; // Error
    }

  if (DBname.IsEmpty() || RecordKey.IsEmpty() || ESet.IsEmpty())
    return 0; // Error

  // Open database (Use virtual class since we are doing a search anyway)
  VIDB *pdb = new VIDB (DBname);
  if (pdb == NULL || !pdb->IsDbCompatible ())
    {
      if (pdb) delete pdb;
      return 0; // Error
    }

  // TODO: If Element is "F" and the document is structured then
  // walk through the .dfd elements and use them to build a
  // "structured" relevant query.
  STRLIST Wordlist;
  size_t  y = 0;
  if (pdb->GetTotalRecords () > 0)
    {
      // Locate the Record
      RESULT RsRecord;
      STRING Data;
      if (pdb->KeyLookup (RecordKey, &RsRecord) == false)
        {
          message_log (LOG_NOTICE, "Stale relevent element %s", RelId.c_str());
        }
      else if (ESet == BRIEF_MAGIC)
        {
          pdb->Headline (RsRecord, NulString, &Data);
          ESet.Clear();
        }
      else if (ESet == FULLTEXT_MAGIC)
        {
          pdb->Present (RsRecord, ESet, &Data);
          // Add words from Headline (increase Headline words)
          STRING Headline;
          pdb->Headline (RsRecord, NulString, &Headline);
          Data.Cat (" "); Data.Cat(Headline); // Add Headline
          ESet.Clear();
        }
      else if ((ESet == SOURCE_MAGIC) || (ESet == METADATA_MAGIC))
        {
          pdb->Present (RsRecord, ESet, &Data);
          ESet.Clear(); // Unfielded search
        }
      else
        {
          pdb->Present (RsRecord, ESet, &Data);
          if (ESet.GetChr(1) == '[') // Is this an indexed field?
            {
              STRINGINDEX pos = ESet.Search(']');
              if (pos) ESet.EraseBefore(pos+1);
            }
        }
      if (!Data.IsEmpty())
        {
          Wordlist.SplitWords(Data); // Break-up words into a list
          y = Wordlist.Sort(); // Sort
          // Get the records language
          pdb->SetStoplist( RsRecord.GetLocale () );
        }
    }

  OPSTACK Stack;
  if (y > 0)
    {
#if 0
      bool HavePositive = false;
      STRING PostiveFn;
      STOPLIST PostiveList; // Put in class to cache
      pdb->ComposeDbFn(&PostiveFn, ".lst");
      if (PostiveList.Load(PositiveFn))
        HavePositive = true;
#endif

      STERM Sterm;
      OPERATOR Operator;
      Operator.SetOperatorType (OperatorOr); // Or all the terms

      PATTRLIST AttrlistPtr = new ATTRLIST ();
      if (!ESet.IsEmpty())
        AttrlistPtr->AttrSetFieldName (ESet); // All in one field
      AttrlistPtr->AttrSetFreeForm (true); // This is feedback

      size_t count=0;
      size_t factor=1;
      STRING Term, Value;
      size_t trim = 0; // Trim count

      for (const STRLIST *p = Wordlist.Next(); p ; p = p->Next())
        {
          bool Done = (p == &Wordlist);

          if (!Done && (p->Value() ^= Term))
            {
              factor++;
              continue; // Get next word
            }
#if 1
          if (!Term.IsEmpty() && !pdb->IsStopWord (Term))
#else
          if (Term.GetLength() &&
              ((!HavePositive && !pdb->IsStopWord (Term)) ||
               PositiveList.InList(Term)))
#endif
            {
              if (trim < 15 || factor > 1 || Term.GetLength() > 8)
                {
                  if (factor*weight <= 1) trim++;
                  Sterm.SetTerm (Term);
                  AttrlistPtr->AttrSetTermWeight ((INT)(weight*factor));
                  Sterm.SetAttributes (*AttrlistPtr);
                  Stack << Sterm;
                  if (count++ >= 1)
                    {
                      // if this is not the first term, push an OR
                      Stack << Operator;
                    }
                }
              factor = 1;
            }
          if (Done)
            break;
          Term = p->Value();
        }

      delete AttrlistPtr;
    }
  delete pdb;

  SetOpstack (Stack);
  return y; // Number of Terms processed
}


size_t SQUERY::SetInfixTerm (const STRING& NewTerm)
{
  STRING TempString;
  size_t status = 0;

  ErrorMessage.Clear();
  INFIX2RPN *Parser = new INFIX2RPN(NewTerm, &TempString);
  if (Parser->InputParsedOK())
    status = SetRpnTerm( TempString );
  else if (!Parser->GetErrorMessage(&ErrorMessage))
    ErrorMessage.form("Unable to parse infix query: %s", NewTerm.c_str());
  delete Parser;

  return status;
}

// TODO: This routine assume that the RPN expression is well-founded.
size_t SQUERY::SetRpnTerm (const STRING& NewTerm)
{
  ErrorMessage.Clear();
  return SetTerm (NewTerm, false);
}

size_t SQUERY::SetTerm (const STRING& NewTerm, bool Ored)
{
  STRING StrTemp;
  STERM Sterm;
  OPSTACK Stack;
  OPERATOR Operator;
  PATTRLIST AttrlistPtr;
  STRINGINDEX pos, slashpos;

//cerr << "SetTerm: " << NewTerm << endl;

  bool is_term = true;

  STRLIST TermList;
  TermList.SplitTerms(NewTerm);

  size_t term_count = 0;
  size_t op_count   = 0;
  bool special = false;
  for (const STRLIST *p = TermList.Next(); p != &TermList; p = p->Next() )
    {
      if ((StrTemp = p->Value()).IsEmpty())
        continue;

      if (!Ored)
        {
          FLOAT metric;
          STRING arg;
          t_Operator Op = GetOperator(StrTemp, &metric, &arg);
          if (Op != OperatorERR)
            {
	      if (Op == OperatorBoostScore)
		{
		  FLOAT metric2;
		  while (p != &TermList)
		    {
		      if (GetOperator(p->Next()->Value(), &metric2, NULL) == OperatorBoostScore)
			{
			  metric = metric * metric2;
			  p = p->Next();
			}
		      else break; // Not a boost
		    }
		}
#if 1 /* EXPERIMENTAL */
	      else if (Op == OperatorWithin)
		{
		  const STRLIST *prev = p->Prev();
		  const STRING pVal (prev->Value());
		  t_Operator Op2 =  GetOperator(pVal, NULL, NULL);
		  if (OperatorERR == Op2 && pVal.Search("/") == 0)
		    {
		      ATTRLIST list;
		      POPOBJ   optr;
		      Stack >> optr;
		      optr->GetAttributes (&list);
		      list.AttrSetFieldName ( arg );
		      Sterm = *optr;
		      Sterm.SetAttributes (list);
		      Stack << Sterm;
		      continue;
		    }
		  else if (IsBinaryOperator(Op2) )
		    {
		      // Replace term0 term1 Op WITHIN:XX to XX/term0 XX/term1 Op
		      POPOBJ   op0;
		      POPOBJ   t1;
		      POPOBJ   t2;

		      Stack >> op0; // Get the operator
		      Stack >> t1;  // Get the term
		      Stack >> t2;  // Get the term

		      // Make sure they are not operators
		      if (t1->GetOpType () == TypeOperand && t2->GetOpType () == TypeOperand)
			{
			  ATTRLIST list1;
			  ATTRLIST list2;
			  STRING   field1, field2;

			  t1->GetAttributes(&list1);
			  t2->GetAttributes(&list2);

			  field1 = list1.AttrGetFieldName();
			  field2 = list2.AttrGetFieldName();

			  if ((field1 ^= arg) && (field1 == field2))
			    {
			      // Redundant field search specification
			      Stack << t2;
			      Stack << t1;
			      Stack << op0;
			      continue; // Can ignore this Operator!
			    }

			  // Either no field specified OR if specified it needs to be the same
			  // as the arg specified in the WITHIN operator
			  if ((field1.IsEmpty() && field2.IsEmpty()) ||
				((field1 ^= arg) && ((field1 == field2) || field2.IsEmpty())) ||
                              ((field2 ^= arg) && field1.IsEmpty()) )
			    {
			      list2.AttrSetFieldName (arg);
			      Sterm = *t2;
			      Sterm.SetAttributes (list2);
			      Stack << Sterm;

			      // Now the same for t1
			      t1->GetAttributes(&list1);
			      list1.AttrSetFieldName (arg);
			      Sterm = *t1;
			      Sterm.SetAttributes (list1);
			      Stack << Sterm;

			      // Now push operator
			      Stack << op0;
			      continue;
			    }
			}
		      // Push back
		      Stack << t2;
		      Stack << t1;
		      Stack << op0;
		    }
		}
#endif
	      else if (Op == OperatorNOT)
		{
		  if (GetOperator(p->Next()->Value(), NULL, NULL) == OperatorNOT)
		    {
		      p = p->Next();
		      continue;
		    }
		  // !  AND -> ANDNOT
		  if (GetOperator(p->Next()->Value(), NULL, NULL) == OperatorAnd)
		    {
		      p = p->Next();
		      Op = OperatorAndNot;
		    }
		}
	      else if (Op == OperatorAnd)
		{
		  // && ! ->   !(A and B) == Nand
		  if (GetOperator(p->Next()->Value(), NULL, NULL) == OperatorNOT)
		    {
		      p = p->Next();
		      Op = OperatorNand;
		    }
		}
	      else if (Op == OperatorOr)
		{
		  if (GetOperator(p->Next()->Value(), NULL, NULL) == OperatorNOT)
		    {
		      p = p->Next();
		      Op = OperatorNor;
		    }
		}
              // Is an Operator
              Operator.SetOperatorType ( Op );
              Operator.SetOperatorMetric( metric );
              Operator.SetOperatorString ( arg );
              Stack << Operator;
              is_term = false; // Not a term
              op_count++;
	      if (op_count == 1)
		{
		 if (Op ==  OperatorWithinFile || Op == OperatorWithinFileExtension ||
		 	Op == OperatorWithKey || OperatorWithinDoctype)
		    special = true;
		}
            }
          else
            {
              is_term = true; // Is a term
	      special = false;
            }
        }
      if (is_term)
        {
          bool left    = false;
          bool literal = false;

          AttrlistPtr = new ATTRLIST ();

          AttrlistPtr->AttrSetFieldType(FIELDTYPE::any);

          if ((slashpos = StrTemp.Search('/')) == 0)
            slashpos = StrTemp.GetLength()+1;

// >[X,Y] is > after or during
// >{x,Y} is > after
// >#u means u is  date
          if ((pos = StrTemp.Search(SOperatorGTE)) > 2 && (pos < slashpos))
            {
              if (StrTemp.GetChr(pos-1) != '\\')
                {
		  STRING fieldName ( StrTemp.SubString(1, pos-1) );
                  AttrlistPtr->AttrSetFieldName ( fieldName );
                  StrTemp.EraseBefore(pos+sizeof(SOperatorGTE)-1);
                  AttrlistPtr->AttrSetRelation(ZRelGE);
                }
            }
          else if ((pos = StrTemp.Search(SOperatorLTE)) > 2 && (pos < slashpos))
            {
              if (StrTemp.GetChr(pos-1) != '\\')
                {
                  AttrlistPtr->AttrSetFieldName ( StrTemp.SubString(1, pos-1) );
                  StrTemp.EraseBefore(pos+sizeof(SOperatorLTE)-1);
                  AttrlistPtr->AttrSetRelation(ZRelLE);
                }
            }
          else if ( (((pos = StrTemp.Search(SOperatorNE)) > 2) ||
                     (pos = StrTemp.Search("!=")) > 2) && (pos < slashpos) )
            {
              if (StrTemp.GetChr(pos-1) != '\\')
                {
                  AttrlistPtr->AttrSetFieldName ( StrTemp.SubString(1, pos-1) );
                  StrTemp.EraseBefore(pos+sizeof(SOperatorNE)-1);
                  AttrlistPtr->AttrSetRelation(ZRelNE);
                }
            }
          else if ((pos = StrTemp.Search('=')) > 2 && (pos < slashpos) &&
		/* Added April 2008 for weights and case */
		!(StrTemp.GetChr(pos+1) == ':' &&
			(StrTemp.GetChr(pos+2) == '-' || isdigit(StrTemp.GetChr(pos+2)) )) )
            {
              if (StrTemp.GetChr(pos-1) != '\\')
                {
		  if (pos == StrTemp.GetLength())
		    {
		      AttrlistPtr->AttrSetAlwaysMatches(true);
		      StrTemp.EraseAfter (--pos);
		    }
		  else
		    {
		      AttrlistPtr->AttrSetFieldName ( StrTemp.SubString(1, pos-1) );
		      StrTemp.EraseBefore(pos+1);
		      AttrlistPtr->AttrSetRelation(ZRelEQ);
		    }
                }
            }
          else if ((pos = StrTemp.Search('>')) > 2 && (pos < slashpos))
            {
              if (StrTemp.GetChr(pos-1) != '\\')
                {
                  AttrlistPtr->AttrSetFieldName ( StrTemp.SubString(1, pos-1) );
                  StrTemp.EraseBefore(pos+1);
                  if (StrTemp.Search('{')) /* } */
                    AttrlistPtr->AttrSetRelation(ZRelAfter);
                  else if (StrTemp.Search('['))
                    AttrlistPtr->AttrSetRelation(ZRelDuringAfter);
                  else
                    AttrlistPtr->AttrSetRelation(ZRelGT);
                }
            }
          else if ((pos = StrTemp.Search('<')) > 2 && (pos < slashpos))
            {
              if (StrTemp.GetChr(pos-1) != '\\')
                {
                  AttrlistPtr->AttrSetFieldName ( StrTemp.SubString(1, pos-1) );
                  StrTemp.EraseBefore(pos+1);
                  if (StrTemp.Search('{')) /* } */
                    AttrlistPtr->AttrSetRelation(ZRelBefore);
                  else if (StrTemp.Search('['))
                    AttrlistPtr->AttrSetRelation(ZRelBeforeDuring);
                  else
                    AttrlistPtr->AttrSetRelation(ZRelLT);

                }
            }
          else if (slashpos < StrTemp.GetLength())
            {
              if ((pos = slashpos) > 1)
                {
                  AttrlistPtr->AttrSetFieldName ( StrTemp.SubString(1, pos-1) );
                  AttrlistPtr->AttrSetFieldType(FIELDTYPE::text);
                }
              StrTemp.EraseBefore(pos+1); // Remove
            }

          if (StrTemp.GetChr(1) == '*')
            {
              left = true;
              StrTemp.EraseBefore(2);
            }
          if (StrTemp.GetChr(1) == '"')
            {
              StrTemp.EraseBefore(2);
              literal = true;
            }
          if ((pos = StrTemp.SearchReverse(':')) > 1 && (
                isdigit(StrTemp.GetChr(pos+1)) ||
                ((StrTemp.GetChr(pos+1) == '-' || StrTemp.GetChr(pos+1) == '+') &&
                 isdigit(StrTemp.GetChr(pos+2)) ) ) )
            {
              AttrlistPtr->AttrSetTermWeight( atoi (((const CHR *)StrTemp)+pos) );
              StrTemp.EraseAfter(pos - 1); // Remove weight
            }
          pos = StrTemp.GetLength ();
          if (StrTemp.GetChr (pos) == '*' && StrTemp.GetChr(pos-1) != '\\')
            {
              if (left)
                AttrlistPtr-> AttrSetLeftAndRightTruncation (true);
              else
                AttrlistPtr->AttrSetRightTruncation (true);
              StrTemp.EraseAfter (--pos);
            }
          else if (StrTemp.GetChr (pos) == '>' && StrTemp.GetChr(pos-1) != '\\')
            {
              if (left)
                AttrlistPtr-> AttrSetLeftAndRightTruncation (true);
              else
                AttrlistPtr->AttrSetRightTruncation (true);
              AttrlistPtr->AttrSetAlwaysMatches (true);
              StrTemp.EraseAfter (--pos);
            }
          else if (left)
            {
              AttrlistPtr->AttrSetLeftTruncation (true);
            }
          if (StrTemp.GetChr (pos) == '=' && StrTemp.GetChr(pos-1) != '\\')
            {
              AttrlistPtr->AttrSetAlwaysMatches (true);
              StrTemp.EraseAfter (--pos);
              // Can have =* but also the mutation *=
              if (StrTemp.GetChr (pos) == '*')
                {
                  if (left)
                    AttrlistPtr-> AttrSetLeftAndRightTruncation (true);
                  else
                    AttrlistPtr->AttrSetRightTruncation (true);
                  StrTemp.EraseAfter (--pos);
                }
            }
#if 1
	  if (StrTemp.IsWild())
	    {
//cerr << "StrTemp = " << StrTemp << endl;
	      if ( AttrlistPtr-> AttrGetLeftAndRightTruncation())
		{
		   AttrlistPtr-> AttrSetLeftAndRightTruncation(false);
		   StrTemp.Prepend("*");
		   StrTemp.Cat ("*");
		   pos += 2;
		}
	      else if ( AttrlistPtr->AttrGetRightTruncation())
		{
		  AttrlistPtr->AttrSetRightTruncation(false);
		  StrTemp += "*";
		  pos++;
		}
	      else if ( AttrlistPtr->AttrGetLeftTruncation())
		{
		  AttrlistPtr->AttrSetLeftTruncation(false);
		  StrTemp.Prepend("*");
		  pos++;
		}
	      AttrlistPtr->AttrSetGlob(true);
	    }
#endif

	 // Let's process post-ops
	 if (StrTemp.GetChr(pos-1) != '\\') {
	   bool zap = true;
	   switch ( StrTemp.GetChr (pos) ) {
	     case '.': AttrlistPtr->AttrSetExactTerm (true); break;
	     case '~': AttrlistPtr->AttrSetPhonetic (true); break;
	     case '$': AttrlistPtr->AttrSetFreeForm (true); break;
	     case '@': AttrlistPtr->AttrSetDenseFeedback (true); break;
	     default:  zap = false; // Not a proper post op
	   }
	  if (zap) StrTemp.EraseAfter (--pos);
	 }

          ///////////////////////////
          // Extract Term
          ///////////////////////////
          // Remove trailing '"'
          if (StrTemp.GetChr(pos) == '"')
            {
              pos--;
              if (literal)
                AttrlistPtr->AttrSetPhrase (true);
            }
          // Remove tailing space
          while (pos && isspace(StrTemp.GetChr(pos)))
            pos--;
          if (pos != StrTemp.GetLength())
            StrTemp.EraseAfter(pos);
          // Remove leading white space
          for (STRINGINDEX i = 1; i <= pos; i++)
            {
              if (!isspace(StrTemp.GetChr(i)))
                {
                  if (i != 1) StrTemp.EraseBefore(i);
                  break;
                }
            }
          term_count++;
          Sterm.SetTerm (StrTemp);
          Sterm.SetAttributes (*AttrlistPtr);
          delete AttrlistPtr;
          Stack << Sterm;

          // if this is not the first term, push an OR
          if (Ored && term_count > 1)
            {
              Operator.SetOperatorType (OperatorOr);
              Stack << Operator;
              op_count++;
            }
        }
    } /* for() */
  SetOpstack (Stack);

  if (NewTerm.GetLength() < 1024)
    message_log (LOG_DEBUG, "SQUERY::SetTerm(\"%s\",%d) -> %d terms / %d ops", NewTerm.c_str(),
        Ored, term_count, op_count);
  if (special) return 1;
  return term_count;
}

bool SQUERY::Equals(const SQUERY& Other)
{
  STRLIST     words, other_words;
  bool isplain       = isPlainQuery(&words, OperatorOr);
  bool other_isplain = Other.isPlainQuery(&other_words, OperatorOr);

  if (!isplain && !other_isplain)
    {
      isplain = isPlainQuery(OperatorAnd);
      other_isplain = Other.isPlainQuery(OperatorAnd);
    }
  if (isplain && other_isplain)
    {
      words.UniqueSort();
      other_words.UniqueSort();
      return words ^= other_words;
    }
  if (words != other_words)
    return false;
  STRING s1, s2;
  if (GetRpnTerm(&s1) == Other.GetRpnTerm(&s2))
    return s1 ^= s2;
  return false;
}

bool SQUERY::isPlainQuery (t_Operator Operator) const
{
  return isPlainQuery((STRLIST *)NULL, Operator);
}

bool SQUERY::isPlainQuery(STRING *Words, t_Operator Operator) const
{
  if (Words == NULL) return isPlainQuery(Operator);

  STRLIST     List;
  bool result = isPlainQuery(&List, Operator);
  *Words = List.Join(" ");
  return result;
}

bool SQUERY::isPlainQuery (STRLIST *Words, t_Operator Operator) const
{
    size_t   fieldCount    = 0;
    size_t   operatorCount = 0;
    size_t   otherCount    = 0;
    size_t   rsetCount     = 0;

    OPSTACK Stack;
    GetOpstack (&Stack);

    POPOBJ   OpPtr;
    ATTRLIST Attrlist;
    STRING   S;

    if (Words) Words->Clear();

    Stack.Reverse (); // BUGFIX Maybe...

    while (Stack >> OpPtr)
      {
        if (OpPtr->GetOpType () == TypeOperand)
          {
            if (OpPtr->GetOperandType () == TypeRset)
              {
                rsetCount++;
              }
            else if (OpPtr->GetOperandType () == TypeTerm)
              {
                if (Words)
                  {
                    OpPtr->GetTerm(&S);
                    if (S.GetLength())
                      {
			Words->AddEntry(S);
                      }
                  }
                OpPtr->GetAttributes (&Attrlist);
                if (Attrlist.AttrGetFieldName (&S) && S.GetLength())
                  fieldCount++;
              }
            else
              otherCount++;
          }
        else if (OpPtr->GetOpType () == TypeOperator)
          {
            if (OpPtr->GetOperatorType () != Operator)
              operatorCount++;
          }
        else
          otherCount++;
        delete OpPtr;
        // If we're not collecting words we're quicker finished
        if (Words == NULL && (fieldCount || operatorCount || rsetCount)) break;
      }

    if (fieldCount || operatorCount || rsetCount) return false;
    if (otherCount) message_log (LOG_WARN, "isPlainQuery(%d): otherCount = %d",
	(int)Operator, otherCount);
    return true;
}

// Same as below really BUT never OR
bool SQUERY::isIntersectionQuery() const
{
    size_t   operatorCount = 0;
    size_t   rsetCount     = 0;

    OPSTACK Stack;
    GetOpstack (&Stack);

    POPOBJ   OpPtr;

    Stack.Reverse (); // BUGFIX Maybe...

    while (Stack >> OpPtr)
      {
        if (OpPtr->GetOpType () == TypeOperand)
          {
            if (OpPtr->GetOperandType () == TypeRset)
              rsetCount++;
          }
        else if (OpPtr->GetOpType () == TypeOperator)
          {
	    const t_Operator Operator = OpPtr->GetOperatorType ();
	    if (Operator == OperatorOr || Operator == OperatorNOT || Operator == OperatorNotWithin ||
		Operator == OperatorXWithin || Operator == OperatorXor || Operator ==  OperatorXnor ||
		Operator == OperatorNotAnd  || Operator ==  OperatorNor || Operator ==  OperatorNand )
              operatorCount++;
          }
        delete OpPtr;
        if (operatorCount) break;
      }
  return operatorCount == 0;
}


bool SQUERY::isOpQuery (const t_Operator Operator) const
{
    size_t   operatorCount = 0;
    size_t   otherCount    = 0;
    size_t   rsetCount     = 0;

    OPSTACK Stack;
    GetOpstack (&Stack);

    POPOBJ   OpPtr;

    Stack.Reverse (); // BUGFIX Maybe...

    while (Stack >> OpPtr)
      {
        if (OpPtr->GetOpType () == TypeOperand)
          {
            if (OpPtr->GetOperandType () == TypeRset)
              rsetCount++;
            else if (OpPtr->GetOperandType () != TypeTerm)
              otherCount++;
          }
        else if (OpPtr->GetOpType () == TypeOperator)
          {
            if (OpPtr->GetOperatorType () != Operator)
              operatorCount++;
          }
        else
          otherCount++;
        delete OpPtr;
        if (operatorCount) break;
      }

    if (operatorCount) return false;
    if (otherCount) message_log (LOG_WARN, "isOpQuery(%d): otherCount = %d",
	(int)Operator, otherCount);
    return true;
}

bool SQUERY::SetOperatorAndWithin(const STRING& FieldName)
{
  OPERATOR Operator;
  Operator.SetOperatorType(OperatorAndWithin);
  Operator.SetOperatorString(FieldName);
  return SetOperator(Operator);
}

bool SQUERY::SetOperatorNear() { return SetOperator (OPERATOR(OperatorNear)); }
bool SQUERY::SetOperatorPeer() { return SetOperator (OPERATOR(OperatorPeer)); }
bool SQUERY::SetOperatorOr()   { return SetOperator (OPERATOR(OperatorOr));   }

bool SQUERY::PushReduce(int Reduce)
{
  OPERATOR op (OperatorReduce);
  op.SetOperatorMetric(Reduce);
  return PushUnaryOperator (op);
}

bool SQUERY::SetOperatorOrReduce(int Reduce)
{
  if (SetOperatorOr())
    return PushReduce(Reduce);
  return false;
}



// Rebuild the Term
size_t SQUERY::GetTerm (PSTRING StringBuffer) const
{
  return fetchTerm (StringBuffer, false);
}

// Rebuild the RpnTerm
size_t SQUERY::GetRpnTerm (PSTRING StringBuffer) const
{
  return fetchTerm (StringBuffer, true);
}

// The main routine to rebuild the query (private)
// author: edz@nonmonotonic.com
size_t SQUERY::fetchTerm (PSTRING StringBuffer, bool WantRpn) const
{
    StringBuffer->Clear();
    OPSTACK Stack;
    GetOpstack (&Stack);

    POPOBJ OpPtr;
    ATTRLIST Attrlist;
    STRING S, T;
    CHR op;
    size_t termCount = 0;
    while (Stack >> OpPtr)
      {
        if (OpPtr->GetOpType () == TypeOperand)
          {
            OpPtr->GetTerm (&T);
            if (!T.IsEmpty())
              {
                termCount++;
                OpPtr->GetAttributes (&Attrlist);
                if (Attrlist.AttrGetFieldName (&S))
                  {
                    INT Relation;
                    STRING glue ("/");
                    if (Attrlist.AttrGetRelation(&Relation))
                      {
                        switch (Relation)
                          {
                          case ZRelGE: glue = SOperatorGTE; break;
                          case ZRelLE: glue = SOperatorLTE; break;
                          case ZRelNE: glue = SOperatorNE;  break;
                          case ZRelEQ: glue = "="; break;
                          case ZRelGT: glue = ">"; break;
                          case ZRelLT: glue = "<"; break;
                          default: glue.form("?(Rel=%ld)?", (long)Relation);
                          }
                      }
                    S.Cat(glue);
                  }
                else if (T.Search('/'))
                  {
                    S = "/"; // Protect "/" in terms.
                  }
                if (!Attrlist.AttrGetGlob())
                  {
                    T.cEncode(); // map " to \" etc
                  }
                if (Attrlist.AttrGetLeftTruncation () ||
                    Attrlist.AttrGetLeftAndRightTruncation ())
                  {
                    S += (char)'*';
                  }
#if CQL
                S << (char)'"' << T << (char)'"';
#else
                if (T.Search(' '))
                  S << (char)'"' << T << (char)'"';
                else
                  S.Cat (T);
#endif
                op = 0;
                if (Attrlist.AttrGetAlwaysMatches ())
                  {
                    op = '=';
                  }
                if (Attrlist.AttrGetExactTerm())
                  {
                    op = '.';
                  }
                if (Attrlist.AttrGetRightTruncation () ||
                    Attrlist.AttrGetLeftAndRightTruncation())
                  {
                    op = (op == '=') ? '>' : '*';
                  }
                else if (Attrlist.AttrGetPhonetic ())
                  {
                    if (op) S+= op;
                    op = '~';
                  }
                if (op) S+= op;
                const INT Weight = Attrlist.AttrGetTermWeight ();
                if ((Weight != 1) || (GetOperator(T) != OperatorERR))
                  {
                    S << ':' << Weight;
                  }
              }
          }
        else if (OpPtr->GetOpType () == TypeOperator)
          {
            if (WantRpn == false)
              {
                if (OpPtr->GetOperatorType () != OperatorOr)
                  {
                    // Not a "plain" query!
                    fetchTerm (StringBuffer, true);
                    delete OpPtr;
                    break;
                  }
                else
                  {
                    S.Clear();
                  }
              }
            else
              {
                switch (OpPtr->GetOperatorType ())
                  {
                  case OperatorWithin:
                    S.form("WITHIN:%s", (OpPtr->GetOperatorString()).c_str() );
                    break;
                  case OperatorXWithin:
                    S.form("XWITHIN:%s", (OpPtr->GetOperatorString()).c_str() );
                    break;
		  case OperatorInclusive:
		    S.form("INCLUSIVE:%s", (OpPtr->GetOperatorString()).c_str() );
		    break;
                  case OperatorInside:
                    S.form("INSIDE:%s", (OpPtr->GetOperatorString()).c_str() );
                    break;
                  case OperatorNoop:	S = SOperatorNoop;	break;
                  case OperatorNOT:	S = SOperatorNOT;	break;
		  case OperatorSibling: S = "SIBLING";          break;
                  case OperatorOr:	S = SOperatorOr;	break;
                  case OperatorAnd:	S = SOperatorAnd;	break;
                  case OperatorAndNot:	S = SOperatorAndNot;	break;
                  case OperatorNotAnd:	S = SOperatorNotAnd;	break;
                  case OperatorXor:	S = SOperatorXor;	break;
                  case OperatorXnor:	S = SOperatorXnor;	break;
                  case OperatorNor:	S = SOperatorNor;	break;
                  case OperatorNand:	S = SOperatorNand;	break;
                  case OperatorLT:	S = SOperatorLT;	break;
                  case OperatorLTE:	S = SOperatorLTE;	break;
                  case OperatorGT:	S = SOperatorGT;	break;
                  case OperatorGTE:	S = SOperatorGTE;	break;
                  case OperatorProximity:
                  {
                    FLOAT metric = OpPtr->GetOperatorMetric();
                    if (metric - (int)metric == 0)
                      {
                        S.form("PROX:%d", (int)metric);
                        break;
                      }
                  }
                  // fall into...
                  case OperatorBefore:
                  case OperatorAfter:
                  case OperatorNear:
                  case OperatorPeer:
                  case OperatorXPeer:
                  case OperatorBeforePeer:
                  case OperatorAfterPeer:
                  case OperatorReduce:
		  case OperatorHitCount:
		  case OperatorTrim:
		  case OperatorSortBy:
		  case OperatorBoostScore:
                  {
                    FLOAT metric = OpPtr->GetOperatorMetric();
                    const char *val = OpPtr->GetOperatorString().c_str();
                    if (metric || val)
                      {
                        const char *what;
                        switch (OpPtr->GetOperatorType ())
                          {
                          case OperatorProximity: what = "PROXIMITY";   break;
                          case OperatorNear:      what = "NEAR";   break;
                          case OperatorBefore:    what = "BEFORE"; break;
                          case OperatorAfter:     what = "AFTER";  break;
                          case OperatorPeer:      what = "PEER";   break;
                          case OperatorBeforePeer:what = SOperatorBeforePeer; break;
                          case OperatorAfterPeer: what = SOperatorAfterPeer; break;
                          case OperatorXPeer:     what =  SOperatorXPeer;  break;
                          case OperatorReduce:    what = "REDUCE"; break;
			  case OperatorHitCount:  what = "HITCOUNT"; break;
			  case OperatorTrim:      what = "TRIM"; break;
			  case OperatorSortBy:    what = "SORTBY"; break;
			  case OperatorBoostScore:what = "BOOST"; break;
                          default: // Should not get here
                            what = "???"; break;
                          }
                        if (metric == 0)
                          {
                            if (val && *val)
                              S.form("%s:%s", what, val);
                            else
                              S = what;
                          }
                        else if (metric - (int)metric == 0)
                          S.form("%s:%d", what, (int)metric);
                        else
                          S.form("%s:%.3f", what, metric);
                      }
                    else switch (OpPtr->GetOperatorType ())
                        {
                        case OperatorProximity: S = SOperatorNear;      break;
                        case OperatorNear:      S = SOperatorNear;      break;
                        case OperatorBefore:    S = SOperatorBefore;    break;
                        case OperatorAfter:     S = SOperatorAfter;     break;
                        case OperatorPeer:	S = SOperatorPeer;      break;
                        case OperatorXPeer:     S = SOperatorXPeer;     break;
                        case OperatorBeforePeer:S = SOperatorBeforePeer; break;
                        case OperatorAfterPeer: S = SOperatorAfterPeer; break;
			case OperatorSibling:   S = "SIBLING";          break;
                        case OperatorReduce:    S = "REDUCE:0";         break; // SPECIAL CASE
			case OperatorHitCount:  S = "HITCOUNT:0";       break; // SPECIAL CASE
			case OperatorTrim:      S = "TRIM:0";           break; // Clear set
			case OperatorBoostScore:S = "BOOST:1";          break;
                        default: break; // Should not get here
                        }
                  }
                  break;
                  case OperatorAdj:	S = SOperatorAdj;	break;
                  case OperatorFollows: S = SOperatorFollows;   break;
                  case OperatorPrecedes:S = SOperatorPrecedes;  break;
                  case OperatorFar:	S = SOperatorFar;	break;
                  case OperatorNeighbor:S = SOperatorNeighbor;	break;
                  case OperatorJoin:    S = SOperatorJoin;      break;
                  case OperatorJoinL:   S = SOperatorJoinL;     break;
                  case OperatorJoinR:   S = SOperatorJoinR;     break;

		  case OperatorFile:
                  case OperatorWithinFile:
                    S.form("FILE:%s", (OpPtr->GetOperatorString()).c_str() );
                    break;
                  case OperatorWithinFileExtension:
                    S.form("EXTENSION:%s",  (OpPtr->GetOperatorString()).c_str() );
                    break;
                  case OperatorKey:
                    S.form("KEY:%s", (OpPtr->GetOperatorString()).c_str() );
                    break;
                  case OperatorWithKey:
                    S.form("WITHKEY:%s", (OpPtr->GetOperatorString()).c_str() );
                    break;
                  case OperatorWithinDoctype:
                    S.form("DOCTYPE:%s", (OpPtr->GetOperatorString()).c_str() );
                    break;
                  case OperatorNotWithin:
                    S.form("NOT:%s", (OpPtr->GetOperatorString()).c_str() );
                    break;
                  case OperatorAndWithin:
                    S.form("AND:%s", (OpPtr->GetOperatorString()).c_str() );
                    break;
                  case OperatorOrWithin:
                    S.form("OR:%s", (OpPtr->GetOperatorString()).c_str() );
                    break;
                  case OperatorBeforeWithin:
                    S.form("BEFORE:%s", (OpPtr->GetOperatorString()).c_str() );
                    break;
                  case OperatorAfterWithin:
                    S.form("AFTER:%s", (OpPtr->GetOperatorString()).c_str() );
                    break;
                  case OperatorERR:
                    S = "<ERROR>";
                    break;
//		  default: S = "<??>";	// Should not happen
                  }
              }
          }

        if (S.GetLength ())
          {
            S.Cat(' ');
            StringBuffer->Insert (1, S);	// Prepend
            S.Clear(); // Clear S
          }
        delete OpPtr;
      }
    StringBuffer->Trim(true);
    return termCount;
}

size_t SQUERY::GetTotalTerms() const
{
    size_t Total = 0;
    OPSTACK Stack;
    GetOpstack (&Stack);
    POPOBJ OpPtr;

    while (Stack >> OpPtr)
      {
        if (OpPtr->GetOpType () == TypeOperand)
          Total++;
        delete OpPtr;
      }
    return Total;
}

void SQUERY::Write (PFILE Fp) const
{
    OPSTACK Stack;
    GetOpstack (&Stack);
    POPOBJ OpPtr;
    ATTRLIST Attrlist;

    Stack.Reverse (); // BUGFIX Maybe...

    putObjID (objSQUERY, Fp);
    while (Stack >> OpPtr)
      {
        t_OpType op_typ = OpPtr->GetOpType ();

        ::Write ((UCHR) op_typ, Fp);
        if (op_typ == TypeOperand)
          {
            t_Operand operand_typ = OpPtr->GetOperandType ();
            ::Write((UCHR) operand_typ, Fp);
            if (operand_typ == TypeTerm)
              {
                OpPtr->GetTerm().Write (Fp);
                OpPtr->GetAttributes (&Attrlist);
                Attrlist.Write (Fp);
              }
            else if (operand_typ == (t_Operand)TypeRset)
              {
                IRSET irset (*OpPtr);
                irset.Write(Fp);
              }
          }
        else if (op_typ == TypeOperator)
          {
            ::Write ((UCHR) OpPtr->GetOperatorType (), Fp);
            ::Write ((DOUBLE) OpPtr->GetOperatorMetric (), Fp);
            ::Write ( OpPtr->GetOperatorString(), Fp);
          }
        delete OpPtr;
      }
    // Now Write End
    ::Write ((UCHR)255 /* '\377'*/, Fp);
}

bool SQUERY::Read (PFILE Fp)
{
  OPSTACK Stack;

  obj_t obj = getObjID (Fp);
  if (obj != objSQUERY)
    {
      // Not a SQUERY!
      PushBackObjID (obj, Fp);
    }
  else
    {
      PATTRLIST AttrlistPtr = new ATTRLIST ();
      PSTRING StringPtr = new STRING ();
      STERM Sterm;
      OPERATOR Operator;
      for (;;)
        {
          BYTE typ;
          ::Read (&typ, Fp);
          const t_OpType op_typ = (t_OpType)typ;
          if (op_typ == TypeOperand)
            {
              ::Read(&typ, Fp);
              const t_Operand operand_typ = (t_Operand)typ;
              if (operand_typ == TypeTerm)
                {
                  StringPtr->Read (Fp);
                  Sterm.SetTerm (*StringPtr);
                  AttrlistPtr->Read (Fp);
                  Sterm.SetAttributes (*AttrlistPtr);
                  Stack << Sterm;
                }
              else if (operand_typ == TypeRset)
                {
                  // NEED TO STORE A POINTER(!)
                  IRSET irset;
                  irset.Read(Fp);
                  Stack << irset;
                }
              else if (operand_typ == TypeNil)
                message_log (LOG_PANIC, "SQUERY:: TypeNil Operand found in read!");
            }
          else if (op_typ == TypeOperator)
            {
              DOUBLE metric;
              STRING StringValue;
              ::Read (&typ, Fp);
              ::Read (&metric, Fp);
              ::Read (&StringValue, Fp);
              Operator.SetOperatorType ((t_Operator)typ);
              Operator.SetOperatorMetric ((FLOAT)metric);
              Operator.SetOperatorString ( StringValue );
              Stack << Operator;
            }
          else if (typ == 255 /*'\377'*/)
            {
              break; // Done
            }
        }
      delete StringPtr;
      delete AttrlistPtr;
    }
  SetOpstack (Stack);
  return (bool)(obj == objSQUERY);
}


void SQUERY::SetThesaurus(THESAURUS *ptr)
{
  if (ptr != Thesaurus)
    {
      CloseThesaurus();
      Thesaurus = ptr;
      expanded  = false;
    }
}

void SQUERY::OpenThesaurus(const STRING& Path)
{
  CloseThesaurus(); // Close Old
  Thesaurus = new THESAURUS(Path);
  expanded  = false;
}


void SQUERY::CloseThesaurus()
{
  if (Thesaurus)
    {
      delete Thesaurus;
      Thesaurus = NULL;
    }
}

static STRING __TermValue(const STRING& Term, int *Weight)
{
  int           weight = 1;
  STRING        StrTemp (Term);
  STRINGINDEX   pos;
  if ((pos = StrTemp.SearchReverse(':')) > 2 && StrTemp.GetChr(pos-1) != '\\' && (
        isdigit(StrTemp.GetChr(pos+1)) || ((StrTemp.GetChr(pos+1) == '-' ||
                                            StrTemp.GetChr(pos+1) == '+') && isdigit(StrTemp.GetChr(pos+2)) ) ) )
    {
      weight = atoi (((const CHR *)StrTemp)+pos);
      StrTemp.EraseAfter(pos - 1); // Remove weight
    }
  *Weight = weight;
  return StrTemp.cDecode();
}

void SQUERY::ExpandQuery()
{
//cerr << "ExpandQuery:" << endl;
  if (expanded || !haveThesaurus())
    return;

  STRING   StringBuffer;
  OPSTACK  Stack;
  OPOBJ   *OpPtr;
  ATTRLIST Attrlist;
  STRING   S, ParentTerm, FieldName, ChildTerm;
  STRLIST  ChildTerms;
  INT      Count = 0;

  GetOpstack(&Stack);
  while (Stack >> OpPtr)
    {
      if (OpPtr->GetOpType() == TypeOperand)
        {
          if (Count++ > 0)
            StringBuffer.Cat(" ");

          OpPtr->GetTerm(&S);

//cerr << "Term=" << S << endl;
          Thesaurus->GetParent(S,&ParentTerm);
          Thesaurus->GetChildren(ParentTerm,&ChildTerms);

          if (ChildTerms.IsEmpty())
            {
//cerr << "Nope!" << endl;
              continue;
            }

          OpPtr->GetAttributes(&Attrlist);
          if (Attrlist.AttrGetFieldName(&S))
            {
              if (!S.IsEmpty())
                {
                  FieldName = S;
                }
            }
          else FieldName.Clear();
          const INT Weight = Attrlist.AttrGetTermWeight ();
          int  w;
          for (STRLIST *p = ChildTerms.Next(); p != &ChildTerms; p = p->Next())
            {
              if (!FieldName.IsEmpty())
                StringBuffer << FieldName << "/";
              StringBuffer << "\"" << __TermValue(p->Value(), &w) << "\"";
              // w--; // Take away 1 from the weight
              if (Weight != 1 || w != 1)
                StringBuffer << ":" << (Weight*w) << " ";
            }
        }
      delete OpPtr;
    }
//cerr << "Term = " << StringBuffer << endl;
  SetTerm(StringBuffer);
//cerr << "Done" << endl;
  expanded = true;
}

SQUERY::~SQUERY ()
{
}


void Write(const SQUERY& SQuery, PFILE Fp)
{
  SQuery.Write (Fp);
}

bool Read(PSQUERY SQueryPtr, PFILE Fp)
{
  return SQueryPtr->Read (Fp);
}


void QUERY::Write(PFILE Fp) const
{
  putObjID(objQUERY,          Fp);
  Squery.Write(Fp);
  ::Write ((BYTE)Sort,        Fp);
  ::Write ((BYTE)Method,      Fp);
  ::Write ((UINT4)MaxResults, Fp);
}


bool QUERY::Read (PFILE Fp)
{
  OPSTACK Stack;

  obj_t obj = getObjID (Fp);
  if (obj != objQUERY)
    {
      // Not a SQUERY!
      PushBackObjID (obj, Fp);
      return false;
    }

  if (Squery.Read(Fp) == false)
    return false;

  BYTE x;
  ::Read(&x, Fp); Sort = (enum SortBy)x;
  ::Read(&x, Fp); Method = (enum NormalizationMethods)x;
  UINT4 m;
  ::Read(&m, Fp); MaxResults = (size_t)m;

  if (x >= UndefinedNormalization)
    return false; // Bad Value

  return true;
}


NAMESPACE istream& operator>>(NAMESPACE istream& is, SQUERY& squery)
{
  STRING str;
  is >> str;
  squery = str;
  return is;
}

NAMESPACE ostream& operator<<(NAMESPACE ostream& os, const SQUERY& query)
{
  STRING s;
  if (query.GetRpnTerm(&s))
    return os << s;
  return os;
}

void Write(const QUERY& Query, PFILE Fp)
{
  Query.Write (Fp);
}

bool Read(QUERY *QueryPtr, PFILE Fp)
{
  return QueryPtr->Read (Fp);
}

int QUERY::Run ()
{
  // Flip OPSTACK upside-down to convert so we can
  // pop from it in RPN order.
  OPSTACK Stack, TempStack;
  IRSET   Foo;

  Squery.GetOpstack (&Stack);
  Stack.Reverse ();

  // Pop OPOBJ's, converting OPERAND's to result sets, and
  // executing OPERATOR's
  POPOBJ       OpPtr;
  POPOBJ       Op1, Op2;
  t_Operator op_t;

  int terms = 0; // Term count (not stopwords)
  while (Stack >> OpPtr)
    {
      if (OpPtr->GetOpType () == TypeOperator)
        {
          if ((op_t = OpPtr->GetOperatorType ()) == OperatorNOT)
            {
              // Unary operators are "Special Case"
              TempStack >> Op1;
              if (Op1 == NULL)
                {
		  return 108;
                }
	       TempStack << Foo;
            }
          else if (op_t == OperatorReduce || op_t == OperatorHitCount || op_t == OperatorTrim ||
		op_t == OperatorBoostScore )
            {
              FLOAT metric =  OpPtr->GetOperatorMetric();
              if ((op_t == OperatorReduce || op_t == OperatorTrim) && metric < 0)
                {
                  return 107;
                }
              TempStack >> Op1;
              if (Op1 == NULL)
                {
		  return 108;
                }
	       TempStack << Foo;
            }
          else if (op_t == OperatorWithinFile || op_t == OperatorWithKey ||
		   op_t == OperatorSortBy ||
		   op_t == OperatorWithinFileExtension || op_t == OperatorWithinDoctype ||
                   op_t == OperatorNotWithin ||
                   op_t == OperatorWithin || op_t == OperatorXWithin ||
		   op_t == OperatorInclusive ||  op_t == OperatorInside)
            {
              STRING fieldName ( OpPtr->GetOperatorString() );
              // Unary operators are "Special Case"
              TempStack >> Op1;
              if (Op1 == NULL)
                {
		  return 108;
                }
              switch (op_t)
                {
		case OperatorWithinFileExtension:
                case OperatorWithinFile:
                case OperatorWithKey:
		case OperatorWithinDoctype:
                case OperatorNotWithin:
                case OperatorWithin:
                case OperatorInside: 
		case OperatorInclusive:
                case OperatorXWithin: 
		case OperatorSortBy:
		  TempStack << Foo;
		  break;
                default: message_log (LOG_PANIC, "INTERNAL ERROR: Unknown Unary Operator %d!", (int)op_t );
                }
            }
          else if (op_t != OperatorNoop && op_t != OperatorERR)
            {
              TempStack >> Op1;
              TempStack >> Op2;
              // Make sure that the RPN stack is not defective!
              if (Op1 == NULL || Op2 == NULL)
                {
                  if (Op1) delete Op1;
                  if (Op2) delete Op2;
		  return 108;
                }
              else
                {
		  // Booleans
                  switch (OpPtr->GetOperatorType () )
                    {
                    case OperatorOr:
                    case OperatorAnd:
                    case OperatorAndNot:
                    case OperatorNotAnd:
		    case OperatorJoin:
		    case OperatorJoinL:
                    case OperatorJoinR:
                    case OperatorXor:
                    case OperatorXnor:
                    case OperatorNor:
                    case OperatorNand:
                    case OperatorNeighbor:
                    case OperatorNear:
                    case OperatorProximity:
                    case OperatorFar:
                    case OperatorAfter:
                    case OperatorBefore:
                    case OperatorAdj:
                    case OperatorFollows:
                    case OperatorPrecedes:
                    case OperatorAndWithin:
                    case OperatorBeforeWithin:
                    case OperatorAfterWithin:
                    case OperatorOrWithin:
                    case OperatorPeer:
                    case OperatorAfterPeer:
                    case OperatorBeforePeer:
                    case OperatorXPeer:
		      TempStack << Foo;
                      break;
                    default:
                      message_log (LOG_ERROR, "RPN Stack contains bogus ops.");
                      // Bad case
                      if (Op1) delete Op1;
                      if (Op2) delete Op2;
		      return 110;
                    }
                }
	      if (Op1) delete Op1;
              if (Op2) delete Op2;
            }
	  delete OpPtr; // ADDED: 2008 March 
        }
      else if (OpPtr->GetOpType () == TypeOperand)
        {
          if (OpPtr->GetOperandType () == TypeRset)
            {
	      delete OpPtr;
            }
          else if (OpPtr->GetOperandType () == TypeTerm)
            {
              terms++; // Increment count
              TempStack << Foo;
            } /* TypeTerm*/
	  delete OpPtr;
        } /* TypeOperand */
    }

  return terms > 0;
}
