/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
//#define PLATFORM_INC_MALLOC
#include "treebox.hxx"

BSTNODE::BSTNODE()
{
  _right = NULL;
  _left  = NULL;
  _count = 0;
  _compareFn = CaseCompare;
}

BSTNODE::~BSTNODE()
{
 // Delete the tree
  if(_left)
    delete _left;
  if(_right)
    delete _right;
}

BSTNODE::operator STRINGS () const
{
  STRINGS List;
  _Walk(&List);
  return List;
}


void BSTNODE::_Walk (STRINGS *List) const
{
  /* Do an inorder traversal */
  if(_data.IsEmpty())
    return;
  if(_left)
    _left->_Walk(List);
  List->AddEntry( _data.c_str() );
  if(_right)
    _right->_Walk(List);
}

int BSTNODE::Insert(const STRING& nData)
{
  if (nData.IsEmpty())
    return 0;
  if(_data.IsEmpty()) {
    _data = nData;
    return ++_count;
  }    
  int diff = _compareFn(nData, _data);
  if(diff>0) {
    /* nData falls after */
    if(_right==NULL) _right = new BSTNODE();
    return _right->Insert(nData);
  }
  if(diff<0) {
    /* nData falls before */
    if(_left==NULL) _left = new BSTNODE();
    return _left->Insert(nData);
  }
  /* Otherwise, nData == Data and we don't insert duplicates */
  return ++_count;
}


void BSTNODE::Print(FILE *fp) const
{
  /* Do an inorder traversal */
  if(_data.IsEmpty())
    return;
  if(_left)
    _left->Print(fp);
  fprintf(fp, "%s [%lu]\n", _data.c_str(), (long unsigned)_count);
  if(_right)
    _right->Print(fp);
}


size_t BSTNODE::GetTotalEntries() const
{
  size_t total = 0;
  if(!_data.IsEmpty())
    total++;
  if(_left)
    total += _left->GetTotalEntries();
  if(_right)
    total += _right->GetTotalEntries();
  return total;
}

