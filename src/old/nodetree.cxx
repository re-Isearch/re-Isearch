/*@@@
File:           nodetree.hxx
Version:        1.0
Description:    Class NODELIST - Node Lists 
Author:         Edward C. Zimmermann
@@@*/

#include "nodetree.hxx"


static int NodeCompare (const void *y, const void *x)
{
  int diff = ((TREENODE *) x)->Fc().GetFieldStart () - ((TREENODE *) y)->Fc().GetFieldStart ();
  if (diff)
    return diff;
  diff =  ((TREENODE *) x)->Fc().GetFieldEnd () - ((TREENODE *) y)->Fc().GetFieldEnd (); 
  if (diff)
    return diff;
  return  ((TREENODE *) x)->Name().Compare ( ((TREENODE *) y)->Name() );
}


void TREENODE::Print (ostream& Os) const
{
  Os << "(\"" << myName << "\","<<  myFc << ")";
}

ostream& operator <<(ostream& Os, const TREENODE& Node)
{
  Node.Print (Os);
  return Os;
}

ostream& operator <<(ostream& Os, const NODETREE& Node)
{
  Node.Print (Os);
  return Os;
}



void TREENODELIST::Print (ostream& Os) const
{
  int count = 0;
  for (const TREENODELIST *p = Next (); p != this; p = p->Next ())
    {
      if (count++) Os << ",";
      else Os << "(";
      Os << p->Node;
    }
  if (count) Os << ")";
}

ostream& operator <<(ostream& Os, const TREENODELIST& List)
{
  List.Print (Os);
  return Os;
}


STRING  TREENODELIST::XMLNodeTree(const STRING& Content) const
{
  STRING Result;
  int count = 0;
  for (const TREENODELIST *p = Prev (); p != this; p = p->Prev ())
    Result << "<" << p->Node.Name() << ">";
  if (Content.IsEmpty())
    Result << "<!--   CONTENT  -->";
  else
    Result << Content;
  for (const TREENODELIST *p = Next (); p != this; p = p->Next ())
    Result << "</" << p->Node.Name() << ">";
  return Result;

}


TREENODELIST *TREENODELIST::AddEntry(const TREENODE& Record)
{
  TREENODELIST *newNode = new TREENODELIST();
  newNode->Node = Record;
  VLIST::AddNode (newNode);
  return this;
}

TREENODELIST *TREENODELIST::AddEntry (const TREENODELIST *Ptr)
{
  if (Ptr)
    {
      for (const TREENODELIST *p = Ptr->Next(); p != Ptr; p = p->Next())
        AddEntry (p->Node);
    }
  return this;
}

TREENODELIST *TREENODELIST::AddEntry (const TREENODELIST& List)
{
  for (const TREENODELIST *p = List.Next(); p != &List; p = p->Next())
    AddEntry (p->Node);
  return this;
}




const TREENODE& TREENODELIST::GetEntry (const size_t Index) const
{
  static TREENODE NulNode;
  TREENODELIST *NodePtr = (TREENODELIST *) (VLIST::GetNodePtr (Index));
  if (NodePtr)
    return NodePtr->Node;
  return NulNode;
}



TREENODELIST& TREENODELIST::operator =(const TREENODELIST& List)
{
  Clear();
  for (const TREENODELIST *p = Next (); p != this; p = p->Next ())
    AddEntry(p->Value());
  return *this;
}


int TREENODELIST::Sort ()
{
  const size_t TotalEntries = GetTotalEntries ();
   // DO we have anything to sort?
  if (TotalEntries > 1)
    {
      TREENODE             *TablePtr = new TREENODE[TotalEntries];
      register TREENODE *ptr = TablePtr;
      register TREENODELIST *p;

      // Put into TablePtr
      for (p = Next(); p != this; p = p->Next())
        *ptr++ = p->Node;

      // Sort
      QSORT (TablePtr, TotalEntries, sizeof (TREENODE), NodeCompare);

      // Put back into list
      ptr = TablePtr;
      for (p = Next(); p != this; p = p->Next())
        p ->Node = *ptr++;
      delete[] TablePtr;                // Cleanup
    }
  return TotalEntries; 
}


