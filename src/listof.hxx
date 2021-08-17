/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
/////////////////////////////////////////////////////////////////////////////
//
// Description:
//
//  Combined stack and list operations
//
// Edit history:
//
//  Written November 1993 by Andrew Davison.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef LISTOF_H
#define LISTOF_H

typedef void* T;

class ListOfElement;

class ListOf
{
public:

	ListOf();
	ListOf(ListOf&);
	virtual ~ListOf();

	ListOf& operator = (ListOf& _listof);
	ListOf& operator += (ListOf& _listof);

	int IsEmpty() const { return !count; };
	unsigned long Count() const { return count; };

	void Clear();

	void Push(const T& item);
	int Pop(T& item);
	int Pop();
	void Append(const T& item);
	void Insert(const T& item, int (*f)(const T&, const T&));
	int Remove(T& item);

	int Head(T& item) const;
	int Tail(T& item) const;

private:

	ListOfElement* head;
	ListOfElement* tail;
	unsigned long count;

	friend class ListOfIterator;
};

class ListOfIterator
{
public:

	ListOfIterator() : listof(0)
		{ last = 0; };

	ListOfIterator(const ListOfIterator& i) : listof(i.listof)
		{ last = i.last; };

	ListOfIterator(ListOf& _listof) : listof(&_listof)
		{ last = 0; };

	ListOfIterator& operator = (ListOfIterator& i)
		{ listof = i.listof; last = i.last; return *this; };

	ListOfIterator& operator = (ListOf& _listof)
		{ listof = &_listof; last = 0; return *this; };

	operator ListOf&() { return *listof; };

	// Iterator operations...

	int First();
	int First(T& item);
	int Last();
	int Last(T& item);
	int Next();
	int Next(T& item);
	int PeekNext() const;
	int PeekNext(T& item) const;
	int Previous();
	int Previous(T& item);
	int PeekPrevious() const;
	int PeekPrevious(T& item) const;
	int Current(T& item) const;
	void Replace(const T& item) const;
	void Terminate() { last = 0; };

	void InsertAfter(const T& item);
	void InsertBefore(const T& item);
	void Delete();

private:

	ListOf* listof;
	ListOfElement* last;
};

#endif

