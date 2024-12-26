/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#pragma ident  "@(#)infix2rpn.cxx  1.16 02/05/01 00:32:50 BSN"

#include "common.hxx"
#include "infix2rpn.hxx"
#include "strstack.hxx"

#define PROX 1
#define UNARYNOT 1

enum operators {
  NOP, LeftParen,
  BoolOR, BoolAND, BoolNOT, BoolXOR, BoolNOR, BoolXNOR, BoolNAND
#ifdef PROX
    ,ProxNEAR, ProxBEFORE, ProxAFTER, ProxADJ, ProxPRECEDES
    ,ProxFOLLOWS, ProxFAR, ProxNEIGHBOR
#endif
#ifdef UNARYNOT
    ,UnNOT
#endif
   ,BoolPEER
   ,BoolXPEER
   ,BoolPEERa
   ,BoolPEERb
   ,BoolXXXX
   ,BoolFRAC = BoolXXXX + 100
   ,BoolPROXIMITY = 32767 + BoolFRAC + 100 
};

#define DEFAULT_OP NOP

INFIX2RPN::INFIX2RPN ()
{
  DefaultOp = (int)DEFAULT_OP;
}

INFIX2RPN::INFIX2RPN (const STRING& StrInput, STRING *StrOutput)
{
  DefaultOp = (int)DEFAULT_OP;
  Parse (StrInput, StrOutput);
}


bool INFIX2RPN::Parse (const STRING& StrInput, STRING *StrOutput)
{
  STRSTACK TheStack;
  STRSTACK OperandStack;
  STRING token, TmpVal, Error;
  STRING field;

  TOKENGEN *TokenList = new TOKENGEN (StrInput);

  StrOutput->Clear();
  TermsWithNoOps = 0;
  bool LastTokenWasTerm = false;
  int  op;

  TokenList->DoParse();
  for (const STRLIST *p = TokenList->TokenList.Next();
	p != &(TokenList->TokenList); p = p->Next())
    {
      token = p->Value();

//cerr << "TOKEN=" << token << endl;
      if (token == "(" || (token == '!' || token == "NOT") || token.Last() == '/' ||
	token.CaseCompare("within:", 7) == 0 || token.CaseCompare("xwithin:", 8) == 0
	|| token.CaseCompare("inside:", 7) == 0 
	|| token.CaseCompare("reduce:", 7) == 0 )
	{
	  TheStack.Push (token.ToUpper());
	  if (token.Search(':'))
	    field = token.Right(':');
	  else if (token.Last() == '/')
	    field = token.Left('/');
//cerr << "FIELD = " << field << endl;
	}
      else if (token == "REDUCE")
	{
	  TheStack.Push ("REDUCE:0");
	}
      else if (token == "WITH")
	{
	  if (field.GetLength())
	    TheStack.Push("WITH:"+field);
	}
      else if (token.GetChr(1) == ')')
	{
	  float Weight = 0;
	  if (token.GetChr(2) == ':') // BOOST
	    {
	      token.EraseBefore(3);
	      Weight = token.GetFloat();
	    }
	  for (TmpVal = NulString; TheStack.Examine (&TmpVal) && TmpVal != "(";)
	    {
	      TheStack.Pop (&TmpVal);
	      if (TmpVal.Trim().GetLength())
		{
		  StrOutput->Cat (TmpVal);
		  StrOutput->Cat (' ');
		  TermsWithNoOps--;
		}
	    }
	  TheStack.Pop (&TmpVal);	//get rid of left paren
	  if ( TheStack.Examine (&TmpVal) && (
		TmpVal == "!" || TmpVal == "NOT" || TmpVal.Last() == '/'
		|| TmpVal.CaseCompare("within:", 7) == 0 || TmpVal.CaseCompare("xwithin:", 8) == 0
		|| TmpVal.CaseCompare("inside:", 7) == 0
		|| TmpVal.Compare(    "REDUCE:", 7) == 0
		))
	    {
	      // Add a space
	      if (StrOutput->GetLength()) StrOutput->Cat (' ');

	      if (TmpVal.Last() == '/')
		{
		  char plast = TmpVal.GetChr ( TmpVal.GetLength() - 1);
		  //   field//(XXX) means XXX INSIDE:field
		  //   field!/(XXX) means XXX XWITHIN:field
		  StrOutput->Cat ( plast == '/' ?
			"INSIDE:" :
			plast == '!' ? "XWITHIN:" : "WITHIN:");
		  StrOutput->Cat ( field = TmpVal.Left('/')  );
		}
	      else
		{
	          StrOutput->Cat (TmpVal);
		  field = TmpVal.Right(':');
		}
	      TheStack.Pop (&TmpVal);       //get rid of '!'
	      StrOutput->Cat (" "); // 2008
	    }
	  if (Weight != 0 && Weight != 1)
	    {
	      StrOutput->Cat("BOOST:"+token);
	      StrOutput->Cat (" ");
	    }
	}
      else if (
	token.CaseCompare("or:", 3) == 0    || token.CaseCompare("and:", 4) == 0 ||
	token.CaseCompare("peer:", 5) == 0  || token.CaseCompare("near:", 5)  == 0 ||
	token.CaseCompare("xpeer:", 6) == 0 || token.Compare("WITH:", 5) == 0 ||
	token.CaseCompare("after:", 6) == 0 || token.CaseCompare("before:", 7) == 0)
	{
	  LastTokenWasTerm = false;
	  TheStack.Push (token.ToUpper());
	  field = token.Right(':');
	}
      else if ((op = string2op (token)) != NOP)
	{
	  LastTokenWasTerm = false;
	  ProcessOp (op, TheStack, StrOutput);
	}
      else
	{
	  if (!LastTokenWasTerm)
	    {
//cerr << "Push " << token << endl;
	      StrOutput->Cat (token);
	      StrOutput->Cat (' ');
	      OperandStack.Push (token);
	      TermsWithNoOps++;
	      LastTokenWasTerm = true;
//cerr << "StrOut=" << *StrOutput << endl;
	    }
	  else
	    {
	      //two terms in a row is invalid in infix.
	      //so we set TermsWithNoOps impossibly high and bail.
	      TermsWithNoOps = 6969;
	      STRING TmpOper;
	      OperandStack.Pop (&TmpOper);

	      Error.form("Two operands '%s' '%s' without a binary operator", TmpOper.c_str(), token.c_str());
	      RegisterError (Error);
	      break;
	    }
	}
    }
  delete TokenList;

  for (TmpVal = NulString; !TheStack.IsEmpty ();)
    {
      TheStack.Pop (&TmpVal);
      TermsWithNoOps--;
      if (TmpVal.Trim().GetLength())
	{
	  StrOutput->Cat (TmpVal);
	  if (!TheStack.IsEmpty()) StrOutput->Cat (" ");

	}
    }

  StrOutput->Trim();

#if 0
  cerr << "OUT = " << *StrOutput << endl;
#endif

  return InputParsedOK();
}

bool INFIX2RPN::InputParsedOK () const
{
  //with no unary not or other weird operators, 
  //you should have n-1 operators for n terms
  return (TermsWithNoOps == 1 ? true : false);
}

void INFIX2RPN::RegisterError (const STRING& Error)
{
  ErrorMessage = Error;
}

bool INFIX2RPN::GetErrorMessage (STRING *Error) const
{
  *Error = ErrorMessage;
  return (Error->GetLength() != 0);
}

void INFIX2RPN::ProcessOp (int op, STRSTACK& TheStack, STRING *result)
{
  STRING TmpVal;
#ifdef UNARYNOT
  //I'll need a special treatment for TermsWithNoOps for unarynot (if ever 
  // implemented).
  if (op != BoolNOT)
    {
#endif
      while ((TheStack.Examine (&TmpVal)) && (TmpVal != "("))
	{
	  TheStack.Pop (&TmpVal);
	  result->Cat (StandardizeOpName (TmpVal));
	  result->Cat (" ");
	  TermsWithNoOps--;
	}
#ifdef UNARYNOT
    }
#endif
  TheStack.Push (op2string (op));
}


//standardizes the various possible representations of
//the various operators.
static const struct {
  const CHR *name;
  enum operators code;
} Ops[] = {
 { "",        NOP      },
 { ")",       LeftParen},
 { "||",      BoolOR   },
 { "&&",      BoolAND  },
 { "!!",      BoolNOT  },
 { "^^",      BoolXOR  },
 { "!|",      BoolNOR  },
 { "^!",      BoolXNOR },
 { "!&",      BoolNAND },
#ifdef PROX
 { ".<.",     ProxNEAR },
 { ".#",      ProxBEFORE},
 { "#.",      ProxAFTER },
 { "##",      ProxADJ   },
 { "#<",      ProxPRECEDES},
 { "#>",      ProxFOLLOWS},
 { ".>.",     ProxFAR   },
 { ".~.",     ProxNEIGHBOR },
#endif
#ifdef UNARYNOT
 { "!",       UnNOT    },
#endif
 { "PEER",    BoolPEER },
 { "XPEER",   BoolXPEER},
 { "PEERa",   BoolPEERa},
 { "PEERb",   BoolPEERb},

// Expand this table as more operators are added
 { "or",      BoolOR   },
 { "and",     BoolAND  },
 { "andnot",  BoolNOT  },
 { "xor",     BoolXOR  },
#ifdef PROX
 { "near",    ProxNEAR },
 { ".#.",     ProxNEAR },
 { "before",  ProxBEFORE},
 { "after",   ProxAFTER },
 { "adj",     ProxADJ   },
 { "far",     ProxFAR   },
 { "prox",    ProxNEIGHBOR },
 { "precedes",ProxPRECEDES},
 { "follows", ProxFOLLOWS},  
#endif
#ifdef UNARYNOT
 { "not",     UnNOT    },
#else
 { "not",     BoolNOT  },
#endif
 { "xnor",    BoolXNOR },
 { "nor",     BoolNOR  },
 { "nand",    BoolNAND }
};  

const STRING INFIX2RPN::StandardizeOpName (const STRING op) const
{
  for (size_t i=0; i < sizeof(Ops)/sizeof(Ops[0]);  i++)
    {
      if (op ^= Ops[i].name)
	return op2string(Ops[i].code);
    }
  return op;
}

int INFIX2RPN::string2op (const STRING op) const
{
  for (size_t i=0; i < sizeof(Ops)/sizeof(Ops[0]);  i++)
    {
      if (op ^= Ops[i].name)
        return Ops[i].code;
    }
#if 1
   if (op.GetChr(1) == '.')
    {
      char tmp[12];
      int metric;
      if (sscanf(op.c_str(), ".%d%10s", &metric, tmp) == 2)
	{
	  if (tmp[0] == '%')
	    {
	      if (metric >= 100)
		return BoolAND;
	      else if (metric <= -100)
		return BoolNOT;
	      else if (metric)
		return (metric + (int)BoolFRAC + 100);
	    }
	  else if (tmp[0] == '.' && metric)
	    return (metric + (int)BoolPROXIMITY);
	  return ProxADJ;
	}
    }
  else if (op.CaseCompare("near", 4) == 0)
    {
      // Near10, Near16
      int metric = atoi(op.c_str()+4);
      if (op.Last() == '%')
	{
	  if (metric >= 100)
	    return BoolAND;
	  else if (metric <= -100)
	    return BoolNOT;
	  if (metric)
	    return (metric + (int)BoolFRAC + 100);
	}
      else if (metric)
	return (metric + (int)BoolPROXIMITY);
      if (op.GetChr(5) == '0')
	return ProxADJ;
    } 
#endif
  return NOP;
}


//converts the internal operator token name to a standard string
const STRING INFIX2RPN::op2string (int op) const
{
  if (op == 0)
    return NulString;
  if (op < 0)
    {
      return op2string(DefaultOp);
    }
  if (op >= BoolXXXX)
    {
#if 1
      STRING tmp;
      if (op - BoolFRAC > 200)
	tmp.sprintf("NEAR:%d",  op - BoolPROXIMITY);
      else
	tmp.sprintf("NEAR:%d%%.", op - BoolFRAC - 100);
#else
      char tmp[33];
      if (op - BoolFRAC > 200)
	sprintf(tmp, "NEAR:%d",  op - BoolPROXIMITY);
      else
	sprintf(tmp, "NEAR:%d%%.", op - BoolFRAC - 100);
#endif
      return tmp;
    }
  return Ops[op].name;
}
