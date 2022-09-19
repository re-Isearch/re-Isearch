//
// Dictionary.cc
//
// Implementation of the Dictionary class
//
// $Log: dictionary.cxx,v $
// Revision 1.1  2007/05/15 15:47:23  edz
// Initial revision
//
// Revision 1.2  1998/01/05 05:20:43  turtle
// Fixed memory leaks
//
// Revision 1.1.1.1  1997/02/03 17:11:04  turtle
// Initial CVS
//
//
#include <stdlib.h>
#include <string.h>
#include "common.hxx"
#include "dictionary.hxx"
#include <iostream>
using namespace std;

class DictionaryEntry
{
public:
    unsigned int	hash;
    STRING		key;
    Object		*value;
    DictionaryEntry	*next;

    ~DictionaryEntry();
    void		release();
};

DictionaryEntry::~DictionaryEntry()
{
    if (value) delete value;
    if (next)  delete next;
}

void DictionaryEntry::release()
{
    value = NULL;		// Prevent the value from being deleted
    if (next) next->release();
}


//*********************************************************************
//
Dictionary::Dictionary()
{
    init(101, 10.0f);
}

Dictionary::Dictionary(size_t initialCapacity, float loadFactor)
{
    init(initialCapacity, loadFactor);
}

Dictionary::Dictionary(size_t initialCapacity)
{
    init(initialCapacity, 0.75f);
}


//*********************************************************************
//
Dictionary::~Dictionary()
{
#if 1
  if (table)
    {
      Destroy();
      delete[] table;
    }
#else
  if (table)
    {
      for (size_t i = 0; i < tableLength; i++)
	{
	  if (table[i]) delete table[i];
	}
      delete [] table;
    }
#endif
}


//*********************************************************************
//
void Dictionary::Destroy()
{
  if (table)
    for (size_t i = 0; i < tableLength; i++)
    {
	if (table[i] != NULL)
	{
	    delete table[i];
	    table[i] = NULL;
	}
    }
    count = 0;
}

//*********************************************************************
//
void Dictionary::Release()
{
    for (size_t i = 0; i < tableLength; i++)
    {
	if (table[i] != NULL)
	{
	    table[i]->release();
	    delete table[i];
	    table[i] = NULL;
	}
    }
    count = 0;
}


//*********************************************************************
//
void Dictionary::init(size_t initialCapacity, float loadFactor)
{
    if (initialCapacity <= 0)
	initialCapacity = 101;
    if (loadFactor <= 0.0)
	loadFactor = 0.75f;
    Dictionary::loadFactor = loadFactor;
    table = new DictionaryEntry*[initialCapacity];
    for (size_t i = 0; i < initialCapacity; i++)
    {
	table[i] = NULL;
    }
    threshold = (int)(initialCapacity * loadFactor);
    tableLength = initialCapacity;
    count = 0;
}

//*********************************************************************
//
/*
template <class V>
int CHashTable<V>::Hash(const CString& ckey, int counter) const {
  CString key(ckey);key.StrTrim32();key.LowerCase();
  unsigned long keyval = 0;
  unsigned long k2;
  int len = key.StrLength();
  if (len < 4) strcpy((char *) &keyval, key.asString());
  if (len >= 4) { memcpy(&keyval, key.asString(), 4); }
  if (len >= 8) { memcpy(&k2, key.asString()+4, 4); keyval ^= k2; }
  if (len >= 12) { memcpy(&k2, key.asString()+8, 4); keyval ^= k2; }
  keyval = abs(keyval);
  double v1 = keyval * 0.6180339887;  // (sqrt5-1)/2, Knuth
  double hv1 = (v1 - (int) v1);
  int h1 = (int) (size_ * hv1);
  double v2 = keyval * 0.87380339887;  // double hash, me 
  double hv2 = (v2 - (int) v2);      
  int h2 = ((int) (size_ * hv2)) * 2 + 1; 
  int retval = (h1 + counter*h2) % size_; 
  return retval;
}
*/
static unsigned int _hash(const char *key, size_t length)
{
  unsigned int	h = 0;

  if (length < sizeof(h))
    {
      memcpy((char *)&h, key, length);;
    }
  else if (length < 16)
    {
      for (size_t i = length; i > 0; i--)
	{
	  h = (h * 37) + *key++;
	}
    }
  else
    {
      size_t skip = length / 8;
      for (long i = (long)length; i > 0; i -= skip, key += skip)
	{
	  h = (h * 39) + *key;
	}
    }
  return h;
}

unsigned int Dictionary::hashCode(const char *key) const
{
  if (key == NULL)
    return 0;
  return _hash(key, strlen(key));
}


unsigned int Dictionary::hashCode(const STRING& key) const
{
  return _hash(key.c_str(), key.Len());
}


//*********************************************************************
//   Add an entry to the hash table.  This will *not* delete the
//   data associated with an already existing key.  Use the Replace
//   method for that function.
//
void Dictionary::Add(const STRING& name, Object *obj)
{
    unsigned int	hash = hashCode(name);
    int			index = hash % tableLength;
    DictionaryEntry	*e;


    for (e = table[index]; e != NULL; e = e->next)
    {
	if (e->hash == hash && e->key == name)
	{
//	    delete e->value;
	    e->value = obj;
	    return;
	}
    }

    if (count >= threshold)
    {
	rehash();
	Add(name, obj);
	return;
    }

    e = new DictionaryEntry();
    e->hash = hash;
    e->key = name;
    e->value = obj;
    e->next = table[index];
    table[index] = e;
    count++;
}


//*********************************************************************
//   Remove an entry from the hash table.
//
GDT_BOOLEAN Dictionary::Remove(const STRING& name)
{
    unsigned int	hash = hashCode(name);
    int			index = hash % tableLength;
    DictionaryEntry	*e, *prev;

    for (e = table[index], prev = NULL; e != NULL; prev = e, e = e->next)
    {
	if (hash == e->hash && e->key == name)
	{
	    if (prev != NULL)
	    {
		prev->next = e->next;
	    }
	    else
	    {
		table[index] = e->next;
	    }
	    count--;
            delete e;
	    return GDT_TRUE;
	}
    }
    return GDT_FALSE;
}


#if 0
STRING Dictionary::Find(Object *value) const
{
    for (DictionaryEntry *e = *table; e != NULL; e = e->next)
    {
        if (e->value == value)
        {
            return e->key;
        }
    }
    return NULL;
}
#endif


//*********************************************************************
//
Object *Dictionary::Find(const char *name) const
{
  if (name == NULL || *name == '\0')
    {
      logf(LOG_PANIC, "Dictionary::Find(Nil)");
      return NULL;
    }
    const unsigned int  hash = hashCode(name);
    const size_t        index = hash % tableLength;

   if (table == NULL)
    {
      logf (LOG_PANIC, "Nil Table in Dictionary (len=%d)", tableLength);
      return NULL;
    }

    for (DictionaryEntry *e = table[index]; e != NULL; e = e->next)
    {
        if (e->hash == hash && e->key == name)
        {
            return e->value;
        }
    }
    return NULL;
}

Object *Dictionary::Find(const STRING& name) const
{
    const unsigned int	hash = hashCode(name);
    const size_t	index = hash % tableLength;

    for (DictionaryEntry *e = table[index]; e != NULL; e = e->next)
    {
	if (e->hash == hash && e->key == name)
	{
	    return e->value;
	}
    }
    return NULL;
}


//*********************************************************************
//
GDT_BOOLEAN Dictionary::Exists(const char *name) const
{
    unsigned int	hash = hashCode(name);
    size_t		index = hash % tableLength;
    DictionaryEntry	*e;

    for (e = table[index]; e != NULL; e = e->next)
    {
	if (e->hash == hash && e->key == name)
	{
	    return GDT_TRUE;
	}
    }
    return GDT_FALSE;
}

GDT_BOOLEAN Dictionary::Exists(const STRING& name) const
{
    unsigned int        hash = hashCode(name);
    size_t              index = hash % tableLength;
    DictionaryEntry     *e;

    for (e = table[index]; e != NULL; e = e->next)
    {
        if (e->hash == hash && e->key == name)
        {
            return GDT_TRUE;
        }
    }
    return GDT_FALSE;
}


//*********************************************************************
//
void Dictionary::rehash()
{
    DictionaryEntry	**oldTable = table;
    size_t		oldCapacity = tableLength;

    size_t		newCapacity;
    DictionaryEntry	*e;
    size_t		i, index;
    
    newCapacity = count > oldCapacity ? count * 2 + 1 : oldCapacity * 2 + 1;

    DictionaryEntry	**newTable = new DictionaryEntry*[newCapacity];

    for (i = 0; i < newCapacity; i++)
    {
	newTable[i] = NULL;
    }

    threshold = (int) (newCapacity * loadFactor);
    table = newTable;
    tableLength = newCapacity;
    
    for (i = oldCapacity; i-- > 0;)
    {
	for (DictionaryEntry *old = oldTable[i]; old != NULL;)
	{
	    e = old;
	    old = old->next;
	    index = e->hash % newCapacity;
	    e->next = newTable[index];
	    newTable[index] = e;
	}
    }
    delete [] oldTable;
}


//*********************************************************************
//
void Dictionary::Start_Get()
{
    currentTableIndex = (size_t)-1;
    currentDictionaryEntry = NULL;
}


//*********************************************************************
//
DictionaryEntry *Dictionary::Get_NextPtr()
{
    while (currentDictionaryEntry == NULL ||
	   currentDictionaryEntry->next == NULL)
    {
	currentTableIndex++;

	if (currentTableIndex >= tableLength)
	{
	    currentTableIndex--;
	    return NULL;
	}

	currentDictionaryEntry = table[currentTableIndex];

	if (currentDictionaryEntry != NULL)
	{
	    return currentDictionaryEntry;
	}
    }
    return currentDictionaryEntry = currentDictionaryEntry->next;
}

const STRING& Dictionary::Get_Next()
{
  DictionaryEntry *next = Get_NextPtr();
  return next ? next->key : NulString;
}

Object *Dictionary::Get_NextObject()
{
  DictionaryEntry *next = Get_NextPtr();
  return next ? next->value : NULL;
}

