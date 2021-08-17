/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
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

#ifndef HASHOF_H
#define HASHOF_H

#define HASHOF_DEBUG 0

#if HASHOF_DEBUG
#include <iostream>
#endif

#ifndef LISTOF_H
#include "listof.hxx"
#endif

#ifndef STRING_HXX
#include "string.hxx"
#endif

///////////////////////////////////////////////////////////////////////////////
class HashOfElement
{
public:

	HashOfElement(const STRING& _key, const T& _item) :
		key(_key), item(_item) {};

private:

	const STRING key;
	T item;

	friend class HashOf;
};

class HashOf
{
public:

	HashOf(unsigned hash_prime);
	~HashOf();

	void Clear();

	int Get(const STRING& key, T& item) const;
	int Get(const STRING& key) const;
	int Drop(const STRING& key);
	int Set(const STRING& key, const T& item);

	unsigned long Count() const { return items; };

#if HASHOF_DEBUG
	void Map(ostream& stream);
#endif

private:

	static unsigned long hash(const char* key);

	ListOf* table;
	unsigned long hash_prime;
	unsigned long items;
#if HASHOF_DEBUG
	unsigned long sets, tries;
#endif
};

#endif
