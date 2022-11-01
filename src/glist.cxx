/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#pragma ident  "@(#)glist.cxx"
/************************************************************************
************************************************************************/
#include "glist.hxx"
#include "gdt.h"

/*
   PRE:	 None.
   POST: Pointer to allocated GLIST
*/
GLIST::GLIST()
{
  Length=0;
  Head=NULL;
  Tail=NULL;
}

/*
   PRE:	 l has been created.
   POST: Nonzero if empty
	 Zero if not empty
*/
bool GLIST::IsEmpty(void)
{
  if(Length==0)
    return true;
  return false;
}

/*
   PRE:	 l has been created.
	 l contains at least one cell.
   POST: Pointer to first cell in list on success.
	 NULL pointer on failure.
*/
IPOSITION* GLIST::First()
{
  return Head;
}

/*
   PRE:	 l has been created.
	 l contains at least one cell.
   POST: Pointer to last cell in list on success.
	 NULL pointer on failure.
*/
IPOSITION* GLIST::Last()
{
  if(Tail == NULL)
    // Attempt to access first position of empty list
    return NULL;

  return Tail;
}

/*
   PRE:	 l has been created.
	 c is a pointer to a cell in l.
   POST: Pointer to next cell in list on success.
	 NULL pointer if no more cells in list.
*/
IPOSITION* GLIST::Next(IPOSITION *c)
{
  if(Tail == c)
    // Attempt to access one position past end of list
    return NULL;

  return c->Next;
}

/*
   PRE:	 l has been created.
	 c is a pointer to a cell in l.
   POST: Pointer to previous cell in list on success.
	 NULL pointer if no more cells in list before c.
*/
IPOSITION* GLIST::Prev(IPOSITION *c)
{
  if(Head == c)
    // Attempt to access one position before start of list
    return NULL;

  return c->Prev;
}

bool GLIST::InsertBefore(IPOSITION *c, IATOM *a, int Type)
{
  IATOM *ta;

  if(a==NULL)
    return false;

  if(IsEmpty())
    InsertAfter(c,a,Type);
  else {
    InsertAfter(c,a,Type);
    ta = Retrieve(c);
    Update(c,a);
    Update(Next(c), ta);
  }	
  return true;

}

bool GLIST::InsertBefore(IPOSITION *c, IATOM *a)
{
  return(InsertBefore(c,a,LIST_PTR));
}

bool GLIST::Update(IPOSITION *c, IATOM *a)
{
  if(a==NULL)
    return false;

  c->Atom = a;

  return true;
}

/*
   PRE:	 l has been created.
	 c points to a cell in l.
	 a points to an allocated structure.
   POST: 0 if out of memory condition.
	 1 on success.  a is inserted in list after c.
*/
bool GLIST::InsertAfter(IPOSITION *c, IATOM *a, int Type)
{
  IPOSITION *nc, *tc;

  if(Head == NULL) {
    // Inserting into empty list
    if((Head = new IPOSITION()) == NULL)
      return false;
    Head->Atom = a;
    Head->Type = Type;
    Head->Next = NULL;
    Head->Prev = NULL;
    Tail = Head;
  } else {
    // Inserting into non-empty list
    if((nc =  new IPOSITION()) == NULL)
      return false;
    nc->Atom = a;
    nc->Type = Type;
    tc = c->Next;
    c->Next = nc;
    nc->Prev = c;
    nc->Next = tc;
    if(c == Tail)
      // Inserted after tail, move the pointer
      Tail = nc;
  }
  Length=Length+1;

  return true;
}

bool GLIST::InsertAfter(IPOSITION *c, IATOM *a)
{
  return(InsertAfter(c,a,LIST_PTR));
}

/*
   PRE:	 l has been created.
	 c points to a cell in l.
   POST: Pointer to atom in c.
*/
IATOM* GLIST::Retrieve(IPOSITION *c)
{
  return c->Atom;
}

INT GLIST::GetLength() const
{
  return Length;
}

/*
   PRE:	 l has been created.
	 c points to a cell in l.
	 Caller has freed memory allocated within the atom in c.
   POST: c is removed from l.
*/
void GLIST::Delete(IPOSITION *c)
{
  if(IsEmpty())
    return;

  if(Head == c) {
    if(Tail == c) {
      // singleton list
      Head = NULL;
      Tail = NULL;
    } else {
      // deleting first cell from non-singleton
      Head = c->Next;
      c->Next->Prev = NULL;
    }
  } else {
    if(Tail == c) {
      // deleting last cell from non-singleton
      Tail = c->Prev;
      c->Prev->Next = NULL;
    } else {
      // deleting cell from interior
      c->Prev->Next = c->Next;
      c->Next = c->Prev;
    }
  }
  delete c;
  Length=Length - 1;

  return;
}

/*
   PRE:	 c is a valid cell.
   POST: Data type integer as defined in list.h
*/
INT GLIST::DataType(IPOSITION *c)
{
  return(c->Type);
}
