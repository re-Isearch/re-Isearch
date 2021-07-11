#pragma ident  "@(#)gstack.cxx  1.2 11/04/98 09:53:24 BSN"
/************************************************************************
************************************************************************/

#include "gstack.hxx"
#include "gdt.h"

/* 
   Stack Routines
*/
GSTACK::GSTACK()
{
}

INT GSTACK::GetSize(void)
{
  return(Stack.GetLength());
}

void GSTACK::Push(IATOM* a)
{
  CurrentIndex = Stack.First();
  Stack.InsertBefore(CurrentIndex,a);
  CurrentIndex = Stack.First();
}

IATOM* GSTACK::Top(void)
{
  CurrentIndex = Stack.First();
  return(Stack.Retrieve(CurrentIndex));
}

IATOM* GSTACK::Pop(void) 
{
  IATOM *p;
  CurrentIndex = Stack.First();
  p = Stack.Retrieve(CurrentIndex);
  Stack.Delete(CurrentIndex);
  CurrentIndex = Stack.First();
  return p;
}
