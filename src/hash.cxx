#pragma ident  "@(#)hash.cxx  1.9 05/08/01 21:36:48 BSN"

/*@@@
File:		hash.cxx
Version:	1.00
$Revision: 1.1 $
Description:	Hash class
Author:		Jim Fullton (Jim.Fullton@cnidr.org)
@@@*/

#include <stdlib.h>
#include <string.h>
#include "common.hxx"
#include "hash.hxx"


static Key_type IndexStr2Num(const STRING& Str)
{
  Key_type Value=0;
  const unsigned char *idxstr = (const unsigned char *)Str.c_str();
  const size_t   len = Str.GetLength();

  if (len > 0)
    {
      register size_t t = 0;
      register int    st = *idxstr;

      Value += st;
      do {
	Value+=((st)*(st))-t++;
	st = *++idxstr;
      } while (t < len);
    }
  return Value;
}


HASH::HASH()
{
  Setup(997);
}

HASH::HASH(size_t Size)
{
  Setup(Size);
}

void HASH::Setup(size_t Size)
{
  TableSize=Size;
  H=new Item_type[TableSize];
}

STRING HASH::GetValue(const STRING& a) const
{
  STRING b;
  if (GetValue(a, &b))
    return b;
  return NulString;
}

GDT_BOOLEAN HASH::GetValue(const STRING& a, STRING *b) const
{
  Item_type *r=Find( IndexStr2Num(a) );

  if(r==NULL){
    b->Clear();
    return GDT_FALSE;
  }
  *b=r->Block;
  delete r;
  return GDT_TRUE;
}

void HASH::AddEntry(const STRING& name, const STRING& value)
{
  Key_type key=IndexStr2Num(name);
  if (!Check(key))
    {
      Item_type r;

      r.Key= key;
      r.Block=value;
      Insert(r);
    }
}

void HASH::AddEntry(const STRING& a)
{
  // a is of type name=value
  STRING name (a);
  STRING value (a);

  STRINGINDEX pos = a.Search('=');
  if (pos)
    {
      name.EraseAfter(pos-1);
      value.EraseBefore(pos+1);
    }
  // else error
  AddEntry(name, value);
}

size_t HASH::HashFunction(Key_type s) const
{
  // this must be adjusted for the Key type!!!
  // Key could be a class, but why bother for a hash table?
  int  result = s%TableSize;
  if (result < 0) result = -result;
  return result;
}

// this is sort of bogus.
INT HASH::Insert(const Item_type& r)
{
  size_t c=0;			// count to ensure table is not full
  size_t i=1;			// increment used for quadratic probing
  size_t p;			// current probed position
  
  p=HashFunction(r.Key);
  while(H[p].State==1 &&	// filled position (not empty or deleted)
	H[p].Key!=r.Key &&		// the key doesn't already exist
	c<=TableSize/2) {		// there is room for probing.....
    c++;
    p+=i;
    i+=2;
    if(p>=TableSize)
      p%=TableSize;
  }

  if(H[p].State!=1) {		// it's empty or deleted
    H[p].State=1;
    H[p].Key=r.Key;
    H[p].Block=r.Block;
  } else if(H[p].Key==r.Key)	// oops - duplicate key
    return(1); 
  else
    return(2);			// table overflow
  return(0);			// valid insertion
  
}

Item_type* HASH::Find(Key_type r) const
{
  size_t c=0;			// count to ensure table is not full
  size_t i=1;			// increment used for quadratic probing
  size_t p;			// current probed position
  Item_type *result;
  
  p=HashFunction(r);
  while(H[p].State!=0 &&	// filled or deleted  position (not empty)
	H[p].Key!=r &&		// key not yet found
	c<=TableSize/2) {	// there is room for probing.....
    c++;
    p+=i;
    i+=2;
    if(p>=TableSize)
      p%=TableSize;
  }
  if (H[p].Key==r) {
    result=new Item_type;
    result->Key=H[p].Key;
    result->State=1;
    result->Block=H[p].Block;
    return(result);
  } else
    return NULL;

}

INT HASH::Check(Key_type r) const
{
  size_t c=0;			// count to ensure table is not full
  size_t i=1;			// increment used for quadratic probing
  size_t p;			// current probed position
  
  p=HashFunction(r);
  while(H[p].State!=0 &&	// filled or deleted  position (not empty)
	H[p].Key!=r &&		// key not yet found
	c<=TableSize/2) {	// there is room for probing.....
    c++;
    p+=i;
    i+=2;
    if(p>=TableSize)
      p%=TableSize;
  }
  if(H[p].Key==r) {
    return(1);
  } else
    return(0);

}

void HASH::Clear()
{
  delete [] H;
  if (TableSize == 0)
    TableSize = 997;
  H=new Item_type[TableSize];
}


HASH::~HASH()
{
  delete [] H;
  TableSize=0;
}
