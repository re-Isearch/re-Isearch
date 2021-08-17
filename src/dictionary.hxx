/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */
//
// Dictionary.h
//
// This class provides an object lookup table.  Each object in the dictionary
// is indexed with a string.  The objects can be returned by mentioning their
// string index.
//
//
#ifndef	_Dictionary_h_
#define	_Dictionary_h_

#include "object.hxx"

class DictionaryEntry;

class Dictionary : public Object
{
public:
  // Construction/Destruction
  Dictionary();
  Dictionary(size_t initialCapacity);
  Dictionary(size_t initialCapacity, float loadFactor);
  ~Dictionary();

  // Adding and deleting items to and from the dictionary
  void		Add(const STRING& name, Object *obj);
  GDT_BOOLEAN	Remove(const STRING& name);

  // Searching can be done with the Find() member of the array indexing
  // operator
  Object        *Find(const char *name) const;
  Object	*Find(const STRING& name) const;
  Object	*operator[](const char *name) const   { return Find(name); }
  Object        *operator[](const STRING& name) const { return Find(name); }
  GDT_BOOLEAN	Exists(const char *name) const;
  GDT_BOOLEAN   Exists(const STRING& name) const;

  // We want to be able to go through all the entries in the
  // dictionary in sequence.  To do this, we have the same
  // traversal interface as the List class
  const STRING& Get_Next();
  Object       *Get_NextObject();
  void		Start_Get();
  void		Release();
  void		Destroy();
  size_t	Count()		{return count;}
    
private:
  DictionaryEntry  *Get_NextPtr();
  DictionaryEntry **table;
  size_t	tableLength;
  size_t	initialCapacity;
  size_t	count;
  size_t	threshold;
  float		loadFactor;

  // Support for the Start_Get and Get_Next routines
  size_t		currentTableIndex;
  DictionaryEntry	*currentDictionaryEntry;
    
  void		rehash();
  void		init(size_t, float);
  unsigned int	hashCode(const char *key) const;
  unsigned int  hashCode(const STRING& key) const;
};

#endif
