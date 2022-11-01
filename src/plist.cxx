/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#include <iostream>
#include "plist.hxx"


PLIST::PLIST()
{
  Length=0;
  Head=(IPOSITION)NULL;
  Tail=(IPOSITION)NULL;
}


PLIST::~PLIST()
{
  RemoveAll();
}


bool PLIST::IsEmpty(void)
{
  if(Length==0)
    return true;
  return false;
}


IPOSITION PLIST::GetHeadPosition() const
{
  return Head;
}


IPOSITION PLIST::GetTailPosition() const
{
  return Tail;
}


IATOM *PLIST::GetPrev(IPOSITION *p) const
{
  if (*p == (IPOSITION)NULL)
    return (IATOM*)NULL;

  // Get a pointer to the Atom of position c
  IATOM *a = (*p)->Atom;

  // iterate the position for the caller
  *p = (*p)->Prev;

  return a;
}


IATOM *PLIST::GetNext(IPOSITION *p) const
{
  if (*p == (IPOSITION)NULL)
    return (IATOM*)NULL;

  // Get a pointer to the Atom of position p
  IATOM *a = (*p)->Atom;

  // iterate the position for the caller
  *p = (*p)->Next;

  return a;
}


IATOM* PLIST::GetEntry(INT Index, IPOSITION *p) {
  if ((*p == (IPOSITION)NULL) || (Index < 1) || (Index > Length))
    return (IATOM*)NULL;
  
  INT i;

  *p = Head;
  // walk down the list to the proper entry
  for (i=1;i<Index;i++)
    *p = (*p)->Next;

  // Get a pointer to the Atom of position p
  IATOM *a = (*p)->Atom;

  return a;
}

    
bool PLIST::InsertBefore(IPOSITION p, IATOM *a)
{
  if((p == (IPOSITION)NULL) || (a == (IATOM*)NULL))
    return false;

  if(IsEmpty())
    AddTail(a);
  else {
    // Is p the Head of the list?
    if(p == Head)
      return AddHead(a);

    IPOSITION nc;
    if((nc =  new _cell) == NULL)
      return false;
    nc->Atom = a;
	  
    IPOSITION tc;
    tc = p->Prev;
	  
    tc->Next = nc;
    nc->Prev = tc;
		
    p->Prev = nc;
    nc->Next = p;

    Length++;
  }	
  return true;
}


bool PLIST::SetAt(IPOSITION p, IATOM *a)
{
  if((p == (IPOSITION)NULL) || (a == (IATOM*)NULL))
    return false;

  p->Atom = a;

  return true;
}


bool PLIST::InsertAfter(IPOSITION p, IATOM *a)
{

  if(Head == (IPOSITION)NULL) {
    // Inserting into empty list
    if((Head = new struct _cell) == NULL)
      return false;
    Head->Atom = a;
    Head->Next = (IPOSITION)NULL;
    Head->Prev = (IPOSITION)NULL;
    Tail = Head;
  } else {
    IPOSITION nc, tc;

    // Inserting into non-empty list
    if((nc =  new struct _cell) == NULL)
      return false;
    nc->Atom = a;

    tc = p->Next;
    p->Next = nc;
    nc->Prev = p;
    nc->Next = tc;
    tc->Prev = nc;

    if(p == Tail)
      // Inserted after tail, move the pointer
      Tail = nc;
    Length++;
  }

  return true;
}


bool PLIST::Cat(PLIST* PlistPtr, IPOSITION p) 
{
  IPOSITION TempTail;
  TempTail = PlistPtr->GetTailPosition();

  if (p) {
    if (p != PlistPtr->GetHeadPosition()) {
      p->Prev->Next = (IPOSITION)NULL;
      PlistPtr->Tail = p->Prev;
      //    delete PlistPtr;
    }
    else {
      PlistPtr->Head = (IPOSITION)NULL;
      PlistPtr->Tail = (IPOSITION)NULL;
//      delete PlistPtr; yes, I know it's a memory leak, but the
//      method of removing the item from the list with RemoveAt()
//      during the loop doesn't seem to protect the items that get
//      inserted from list 2 into list 1 from being deleted, and
//      causes a NULL to sneak in where it's not expected. - gem
    }
    
    if (Tail) {
      Tail->Next = p;
      Tail->Next->Prev = Tail;
    }
    if (!Head) {
      Head = p;
    }
    Tail = TempTail;

    while (p != (IPOSITION)NULL) {
      p = p->Next;
      Length++;
    }
  	return true;
  }
  else {
	return false;
  }
}


IATOM* PLIST::GetAt(IPOSITION p)
{
  return p->Atom;
}


void 
PLIST::RemoveAt(IPOSITION p)
{
  if((IsEmpty()) || (p == (IPOSITION)NULL))
    return;

  if(Head == p) {
    if(Tail == p) {
      // singleton list
      Head = (IPOSITION)NULL;
      Tail = (IPOSITION)NULL;
    } else {
      // deleting first cell from non-singleton
      Head = p->Next;
      Head->Prev = (IPOSITION)NULL;
    }
  } else {
    if(Tail == p) {
      // deleting last cell from non-singleton
      Tail = p->Prev;
      Tail->Next = (IPOSITION)NULL;
    } else {
      // deleting cell from interior
      p->Prev->Next = p->Next;
      p->Next->Prev = p->Prev;
    }
  }
  delete p;
  Length -= 1;

  return;
}


bool PLIST::AddTail(IATOM *a)
{
  if(Head == (IPOSITION)NULL) {
    // Inserting into empty list
    if((Head = new struct _cell) == NULL)
      return false;
    Head->Atom = a;
    Head->Next = (IPOSITION)NULL;
    Head->Prev = (IPOSITION)NULL;
    Tail = Head;
  } else {
    IPOSITION nc;
    // Inserting into non-empty list
    if((nc =  new struct _cell) == NULL)
      return false;
    nc->Atom = a;
    nc->Next = (IPOSITION)NULL;
    nc->Prev = Tail;

    Tail->Next = nc;
    Tail = nc;
  }
  Length++;

  return true;
}


void PLIST::RemoveAll()
{
  RemoveRight(Head);
  Head = (IPOSITION)NULL;
  Tail = (IPOSITION)NULL;
  Length = 0;
}


void PLIST::DeleteAll() 
{
  IPOSITION p;
  IATOM* a;

  p = Head;
  while ((a = GetNext(&p)) != NULL) {
    // delete a; /* Not only don't we have a delete but we don't create it anyway */
    if (p)
      delete p->Prev;
  }
  Head = (IPOSITION)NULL;
  Tail = (IPOSITION)NULL;
  Length = 0;
}


void PLIST::RemoveRightAndDelete(IPOSITION p)
{
  IPOSITION q = p;
  IPOSITION TempTail = p;
  bool FromHead = false;

  if ((p) && (Length > 0)) {
    TempTail = p->Prev;
    if (p == Head)
      FromHead = true;

    while ((p) && (p->Next)) {
      // delete p->Atom;
      q = p;
      p = p->Next;
      delete q;
      Length--;
    }
    if (p) {
    // delete p->Atom;
    delete p;
    Length--;
    }

    if (FromHead)
      Head = (IPOSITION)NULL;

    if (TempTail) {
      Tail = TempTail;
      Tail->Next = (IPOSITION)NULL;
    }
    else {
      Tail = (IPOSITION)NULL;
    }
  }
}


void 
PLIST::RemoveRight(IPOSITION p)
{
  IPOSITION q;
  IPOSITION TempTail = (IPOSITION)NULL;
  bool FromHead = false;


  if ((p) && (Length > 0)) { // length is quick hack until we can find
    // why stack is leaving entries in there.
    if (p != Head) {
      TempTail = p->Prev;
    }
    else {
      FromHead = true;
    }

    while ((p) && (p->Next)) { // Tail seems to be getting corrupted
      // somewhow.  
      q = p;
      p = p->Next;
      delete q;
      Length--;
    }
    if (p) {
      delete p;
      Length--;
    }
    
    if (FromHead)
      Head = (IPOSITION)NULL;

    if (TempTail) {
      TempTail->Next = (IPOSITION)NULL;
      Tail = TempTail;
    }
    else {
      Tail = (IPOSITION)NULL;
    }
  }
  /*
    if(p->Next)
    RemoveRight(p->Next);
    if(p == Tail) {
    Tail = p->Prev;
    if(Tail)
    Tail->Next = (IPOSITION)NULL;
    }
    delete p;
    Length -= 1;
  */
}


bool 
PLIST::AddHead(IATOM *a)
{
  IPOSITION nc;
  if((nc =  new struct _cell) == NULL)
    return false;
  nc->Atom = a;

  if(IsEmpty()) {
    nc->Next = nc->Prev = (IPOSITION)NULL;
    Head = Tail = nc;
  } else {
    Head->Prev = nc;
    nc->Next = Head;
    nc->Prev = (IPOSITION)NULL;
    Head = nc;
  }	
  Length++;
  return true;
}


/*POSITION PLIST::Merge(POSITION a, POSITION b, INT aLen, INT bLen,
		  INT (*compar)(void* a, void* b)) { 
*/
IPOSITION PLIST::Merge(IPOSITION a, IPOSITION b, INT aLen, INT bLen, 
	     INT (*compar)(void* a, void* b)) 
{
  IPOSITION TempHead, c, tempa, tempb;
  INT aPos = 0;
  INT bPos = 0;
  tempa = a;
  tempb = b;

  if ((*compar)((void*)a->Atom, (void*)b->Atom) <= 0) {
    c = a;
    a = a->Next;
    aPos++;
  }
  else {
    c = b;
    b = b->Next;
    bPos++;
  }

  TempHead = c;

  while ((aPos != aLen) && (bPos != bLen)) {
    if ((*compar)((void*)a->Atom, (void*)b->Atom) <= 0) {
      c->Next = a;
      c = a;
      a = a->Next;
      aPos++;
    }
    else {
      c->Next = b;
      c = b;
      b = b->Next;
      bPos++;
    }
  }
  
  if (aPos == aLen) {
    c->Next = b;
  }
  else {
    c->Next = a;
  }
  
  c = TempHead;
  /*  cout << "\nTo produce: ";
  
      for (i= 0; i< aLen+bLen; i++) {
      cout << (char*)c->Atom << " ";
      c = c->Next;
      }
      cout << endl;
  */      
  return TempHead;
}


void PLIST::Sort(INT (*compar)(void* a, void* b)) 
{
  IPOSITION a, b;
  INT i;
  //  cout << "Setting Prev..." << endl;
  Head = MergeSort(Head, Length, compar);
  a = Head;
  for (i = 1; i < Length; i++) {
    b = a->Next;
    b->Prev = a;
    a = a->Next;
  }
  Head->Prev = (IPOSITION)NULL;
  a->Next = (IPOSITION)NULL;
  Tail = a;
  //  cout << (char*)a->Atom << " is Last Item." << endl;
}


IPOSITION PLIST::MergeSort(IPOSITION c, INT Len, INT (*compar)(void* a, void* b)) 
{
  // Cut the list in half, sort each half and merge
  IPOSITION a, b;//, List1Tail; 
  INT i;
  INT List1Length = Len/2;
  INT List2Length = Len - List1Length;

  //	cout << "MergeSorting " << c << " of length " << Len << endl;

  if (Len > 1) {
    a = c;
    b = c;
    for (i = 0; i < List1Length; i++) {
      b = b->Next;
    }
	  
    return Merge(MergeSort(a, List1Length, compar), 
		 MergeSort(b, List2Length, compar), 
		 List1Length, List2Length, compar);

    /*
      return Merge(MergeSort(a, Mid, compar),
      MergeSort(b, Len-Mid,compar), Mid, Len-Mid,
      compar);  
    */
  } 
  return c;
}


void PLIST::Reverse() 
{
  IPOSITION p = Head;
  IPOSITION TempHead = p;
  IPOSITION TempTail = Tail;
  IPOSITION TempNext, TempPrev;
  
  while (p) {
    TempNext = p->Next;
    TempPrev = p->Prev;
    p->Next = TempPrev;
    p->Prev = TempNext;
    p = p->Prev;
  }
  Head = TempTail;
  Tail = TempHead;
}


PLIST& PLIST::operator=(PLIST& OtherPlist) 
{
  IPOSITION p;
  IATOM *a;
  p = GetHeadPosition();

  while ((a = GetNext(&p)))
    OtherPlist.AddTail(a);

  return *this;
}
