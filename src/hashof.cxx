/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#pragma ident  "@(#)hashof.cxx  1.2 06/27/00 20:30:08 BSN"
/*--
///////////////////////////////////////////////////////////////////////////////
//
// Description
//
//	The hash-table size must be a prime number, but an unlimited number
//	of entries can be added.
//
// Edit History
//
//	Written November 1993 by Andrew Davison.
//	Modified September 1994 by AD to allow insertion of duplicates.
//	Modified September 1994 by AD to allow key iteration.
//
///////////////////////////////////////////////////////////////////////////////
*/

#include "common.hxx"
#include "hashof.hxx"

///////////////////////////////////////////////////////////////////////////////
HashOf::HashOf(unsigned _hash_prime)
{
  hash_prime = _hash_prime;
  items = 0;
#if HASHOF_DEBUG
  sets = tries = 0;
#endif
  table = new ListOf[(size_t) hash_prime];
}

///////////////////////////////////////////////////////////////////////////////
HashOf::~HashOf()
{
  Clear();
  delete[] table;
}

///////////////////////////////////////////////////////////////////////////////
void            HashOf::Clear()
{
  for (unsigned i = 0; i < hash_prime; i++) {
    void           *t;

    while (table[i].Pop(t)) {
      HashOfElement  *element = (HashOfElement *) t;
      delete          element;
    }
  }

  items = 0;
}

///////////////////////////////////////////////////////////////////////////////
#define HASHWORDBITS 32

/*
 * Defines the so called `hashpjw' function by P.J. Weinberger [see
 * Aho/Sethi/Ullman, COMPILERS: Principles, Techniques and Tools, 1986, 1987
 * Bell Telephone Laboratories, Inc.]
 */
unsigned long   HashOf::hash(const char *key)
{
  unsigned long   h = 0;

  while (*key) {
    unsigned long   g;

    h <<= 4;
    h += (unsigned char) *key++;
    if ((g = (h & ((unsigned long) 0xf << (HASHWORDBITS - 4)))) != 0) {
      h ^= g >> (HASHWORDBITS - 8);
      h ^= g;
    }
  }
  return h;
}

///////////////////////////////////////////////////////////////////////////////
int             HashOf::Get(const STRING& key) const
{
  if (!items)
    return 0;

  unsigned long   val = hash(key.c_str());
  unsigned        idx = (unsigned) (val % hash_prime);

  ListOfIterator  ilist(table[idx]);
  void           *t;

  for (int ok = ilist.First(t); ok; ok = ilist.Next(t)) {
    HashOfElement  *element = (HashOfElement *) t;

    if (element->key == key) {
      return 1;
    }
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
int             HashOf::Get(const STRING& key, T& item) const
{
  if (!items)
    return 0;

  unsigned long   val = hash(key.c_str());
  unsigned        idx = (unsigned) (val % hash_prime);

  ListOfIterator  ilist(table[idx]);
  void           *t;

  for (int ok = ilist.First(t); ok; ok = ilist.Next(t)) {
    HashOfElement  *element = (HashOfElement *) t;

    if (element->key == key) {
      item = element->item;
      return 1;
    }
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
int             HashOf::Drop(const STRING& key)
{
  if (!items)
    return 0;

  unsigned long   val = hash(key.c_str());
  unsigned        idx = (unsigned) (val % hash_prime);

  ListOfIterator  ilist(table[idx]);
  void           *t;

  for (int ok = ilist.First(t); ok; ok = ilist.Next(t)) {
    HashOfElement  *element = (HashOfElement *) t;

    if (element->key == key) {
      ilist.Delete();
      delete          element;
      items--;
      return 1;
    }
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
int             HashOf::Set(const STRING& key, const T& item)
{
  unsigned long   val = hash(key.c_str());
  unsigned        idx = (unsigned) (val % hash_prime);

#if HASHOF_DEBUG
  sets++;
  tries++;
#endif

  ListOfIterator  ilist(table[idx]);
  void           *t;

  for (int ok = ilist.First(t); ok; ok = ilist.Next(t)) {
    HashOfElement  *element = (HashOfElement *) t;

    if (element->key == key) {
      element->item = item;
      return 0;
    }
#if HASHOF_DEBUG
    tries++;
#endif
  }

  HashOfElement  *element = new HashOfElement(key, item);
  table[idx].Push(element);
  items++;

  return 1;
}

///////////////////////////////////////////////////////////////////////////////
#if HASHOF_DEBUG
void            HashOf::Map(ostream& stream)
{
  stream << " |";

  for (unsigned i = 0; i < hash_prime; i++) {
    if (table[i].IsEmpty())
      stream << '.';
    else
      stream << '+';

    if (!((i + 1) % 70))
      stream << "|\n |";
  }

  while (i++ % 70)
    stream << ' ';

  stream << "|\n";

  stream << "Items: " << items << "/" << hash_prime;
  stream << " = " << (double) items *100 / hash_prime << "%";

  if (sets)
    stream << " Hit ratio: " << (double) tries / sets;

  stream << '\n';
}
#endif
