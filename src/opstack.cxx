/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */
#pragma ident  "@(#)opstack.cxx"

/*-@@@
File:		opstack.cxx
Description:	Class OPSTACK - Operand/operator Stack
@@@*/

#include "opstack.hxx"

OPSTACK::OPSTACK ()
{
  Head = NULL;
}

OPSTACK::OPSTACK (const OPSTACK& OtherOpstack)
{
  Head = NULL;
  // push OtherOpstack's ops onto this stack
  for (POPOBJ OpPtr = OtherOpstack.Head; OpPtr; OpPtr = OpPtr->Next)
    {
      Push(*OpPtr);
    }
  // flip this stack upside down
  Reverse ();
}

bool OPSTACK::IsEmpty() const
{
  return Head == NULL;
}

void OPSTACK::Clear()
{
  if (Head != NULL)
    {
      POPOBJ OpPtr;
      while (*this >> OpPtr) delete OpPtr;
      Head = NULL;
    }
}

OPSTACK& OPSTACK::operator =(const OPSTACK& OtherOpstack)
{
  // [faster method using OPSTACK::Reverse()]
  // pop everything off this stack
  POPOBJ OpPtr;
  while (*this >> OpPtr)
    {
      delete OpPtr;
    }
  // push OtherOpstack's ops onto this stack
  OpPtr = OtherOpstack.Head;
  while (OpPtr)
    {
      *this << *OpPtr;
      OpPtr = OpPtr->Next;
    }
  // flip this stack upside down
  Reverse ();
  return *this;
}

void OPSTACK::Reverse ()
{
  POPOBJ p = Head;
  POPOBJ pp = NULL;
  POPOBJ tp;
  while (p)
    {
      tp = p->Next;
      p->Next = pp;
      pp = p;
      p = tp;
    }
  Head = pp;
}

OPSTACK& OPSTACK::operator <<(const OPOBJ& Op)
{
  Push (Op);
  return *this;
}

OPSTACK& OPSTACK::operator <<(const POPOBJ& OpPtr)
{
  Push (OpPtr);
  return *this;
}

void OPSTACK::Push (const OPOBJ& Op)
{
  POPOBJ OpPtr = Op.Duplicate ();
  OpPtr->SetNext (Head);
  Head = OpPtr;
}

void OPSTACK::Push (const POPOBJ& OpPtr)
{
  if (OpPtr)
    {
      OpPtr->SetNext (Head);
      Head = OpPtr;
    }
}

POPOBJ OPSTACK::operator >>(POPOBJ& OpPtr)
{
  return (OpPtr = Pop ());
}

PIRSET OPSTACK::operator >>(PIRSET& OpPtr)
{
//cerr << "OPSTACK >> " << endl;
  POPOBJ OpobjPtr = Pop ();
  PIRSET IrsetPtr = (PIRSET) OpobjPtr;
//cerr << "RETURN" << endl;
  return (OpPtr = IrsetPtr);
}

#if 1
// BUGFIX!!!!!!!
POPOBJ OPSTACK::Pop ()
{
  if (Head)
    {
      POPOBJ OldHead = Head;
      Head = Head->GetNext ();
      return OldHead;
    }
  return NULL;
}
#else

POPOBJ OPSTACK::Pop ()
{
  POPOBJ OldHead = Head;
  if (Head)
    Head = Head->GetNext ();
  return OldHead;
}
#endif

OPSTACK::~OPSTACK ()
{
  OPOBJ *OpPtr;
  while (*this >> OpPtr)
    {
      delete OpPtr;
    }
}
