/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#pragma ident  "@(#)strstack.cxx  1.11 06/27/00 20:31:59 BSN"

#include "common.hxx"
#include "strstack.hxx"

/*quick and dirty stack class */

STRSTACK::STRSTACK ()
{
  CurrIndex = 0;
}

STRSTACK::~STRSTACK()
{
}

void STRSTACK::Reset(size_t Index)
{
  CurrIndex = Index;
}

void STRSTACK::Push (const STRING& Value)
{
  StackList.SetEntry (++CurrIndex, Value);
}


STRING STRSTACK::Pop ()
{
  STRING Value;
  Pop(&Value);
  return Value;
}


bool STRSTACK::Pop (STRING *Value)
{
  if (CurrIndex >= 1)
    return StackList.GetEntry (CurrIndex--, Value);
  Value->Clear();
  return false;
}


size_t STRSTACK::GetTotalEntries ()
{
  return CurrIndex;
}

bool STRSTACK::IsEmpty ()
{
  return (CurrIndex ? false : true);
}

STRING STRSTACK::Examine() const
{
  return StackList.GetEntry(CurrIndex);
}

bool STRSTACK::Examine (STRING *Value) const
{
  if (CurrIndex >= 1)
    return StackList.GetEntry (CurrIndex, Value);
  return false;
}

bool STRSTACK::Examine (size_t i, STRING *Value) const
{
  return StackList.GetEntry (i, Value);
}


STRING STRSTACK::Join(const STRING& Sep) const
{
  STRING String;
#if STRSTACK_USE_STRLIST
  for(const STRLIST *itor = StackList.Next();;)
   {
     String.Cat(itor->Value());
     itor = itor->Next();
     if (itor == &StackList)
        break;
     String.Cat(Sep);
   }
#else
  for (size_t i = 1; i <= CurrIndex; i++)
    {
      if (i > 1)
	String.Cat(Sep);
      String.Cat(StackList.GetEntry(i));
    }
#endif
  return String;
}

