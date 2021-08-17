/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#pragma ident  "@(#)listof.cxx  1.3 06/27/00 20:30:48 BSN"
/////////////////////////////////////////////////////////////////////////////
//
// Description:
//
//  Combined stack and list operations
//
// Edit history:
//
//  Written November 1993 by Andrew Davison.
//	 Modified March 1995 by AD to de-templatize.
//  Modified November 1995 by AD to inserting into ordered list.
//
/////////////////////////////////////////////////////////////////////////////

#include "listof.hxx"
#pragma hdrstop

class ListOfElement
{
public:

	ListOfElement(const T& _item) :
		item(_item) {};

private:

	ListOfElement* next;
	ListOfElement* prev;
	T item;

	friend class ListOf;
	friend class ListOfIterator;
};

/////////////////////////////////////////////////////////////////////////////
// Inputs - None.
// Outputs - None.
// Description - An empty list is created.
/////////////////////////////////////////////////////////////////////////////

ListOf::ListOf() :
	head(0), tail(0), count(0)
{
}

/////////////////////////////////////////////////////////////////////////////
// Inputs - None.
// Outputs - None.
// Description - A list is copied.
/////////////////////////////////////////////////////////////////////////////

ListOf::ListOf(ListOf& _listof) :
	head(0), tail(0), count(0)
{
	ListOfIterator iterator(_listof);
	T item;

	for (int ok = iterator.First(item); ok; ok = iterator.Next(item))
		Append(item);
}

/////////////////////////////////////////////////////////////////////////////
// Inputs - None.
// Outputs - None.
// Description - An list is deleted.
/////////////////////////////////////////////////////////////////////////////

ListOf::~ListOf()
{
	Clear();
}

/////////////////////////////////////////////////////////////////////////////
// Inputs - None.
// Outputs - None.
// Description - A list is copied to.
/////////////////////////////////////////////////////////////////////////////

ListOf& ListOf::operator = (ListOf& _listof)
{
	Clear();

	*this += _listof;

	return *this;
}

/////////////////////////////////////////////////////////////////////////////
// Inputs - None.
// Outputs - None.
// Description - A list is added to.
/////////////////////////////////////////////////////////////////////////////

ListOf& ListOf::operator += (ListOf& _listof)
{
	ListOfIterator iterator(_listof);
	T item;

	for (int ok = iterator.First(item); ok; ok = iterator.Next(item))
		Append(item);

	return *this;
}

/////////////////////////////////////////////////////////////////////////////
// Inputs - None.
// Ouputs - None.
// Returns - None.
// Description - Removes all items from the list.
/////////////////////////////////////////////////////////////////////////////

void ListOf::Clear()
{
	while (Count() && Pop());
}

/////////////////////////////////////////////////////////////////////////////
// Inputs - An item.
// Ouputs - None.
// Returns - None.
// Description - A new item is placed at the head of the list.
/////////////////////////////////////////////////////////////////////////////

void ListOf::Push(const T& item)
{
	count++;

	ListOfElement* element = new ListOfElement(item);

	if (head)
		head->prev = element;

	element->prev = 0;
	element->next = head;

	head = element;

	if (!tail)
		tail = head;
}

/////////////////////////////////////////////////////////////////////////////
// Inputs - None.
// Outputs - An item.
// Returns - A boolean value.
// Description - The head item is removed and returned.
/////////////////////////////////////////////////////////////////////////////

int ListOf::Pop(T& item)
{
	if (!head)
		return 0;

	count--;

	ListOfElement* element = head;
	item = element->item;
	head = element->next;
	delete element;

	if (head)
		head->prev = 0;
	else
		tail = 0;

	return 1;
}

/////////////////////////////////////////////////////////////////////////////
// Inputs - None.
// Outputs - None.
// Returns - A boolean value.
// Description - The head item is removed.
/////////////////////////////////////////////////////////////////////////////

int ListOf::Pop()
{
	if (!head)
		return 0;

	count--;

	ListOfElement* element = head;
	head = element->next;
	delete element;

	if (head)
		head->prev = 0;
	else
		tail = 0;

	return 1;
}

/////////////////////////////////////////////////////////////////////////////
// Inputs - An item.
// Outputs - None.
// Description - A new item is placed at the tail of the list.
/////////////////////////////////////////////////////////////////////////////

void ListOf::Append(const T& item)
{
	count++;

	ListOfElement* element = new ListOfElement(item);

	element->prev = tail;
	element->next = 0;

	if (tail)
		tail->next = element;

	tail = element;

	if (!head)
		head = tail;
}

/////////////////////////////////////////////////////////////////////////////
// Inputs - None.
// Outputs - An item.
// Returns - A boolean value.
// Description - The tail item is removed and returned.
/////////////////////////////////////////////////////////////////////////////

int ListOf::Remove(T& item)
{
	if (!tail)
		return 0;

	count--;

	ListOfElement* element = tail;
	item = element->item;
	tail = element->prev;
	delete element;

	if (tail)
		tail->next = 0;
	else
		head = 0;

	return 1;
}

/////////////////////////////////////////////////////////////////////////////
// Inputs - An item.
// Outputs - None.
// Description - A new item is inserted into an ordered list.
/////////////////////////////////////////////////////////////////////////////

void ListOf::Insert(const T& item, int (*f)(const T&, const T&))
{
	ListOfIterator items(*this);
	void* t;

	for (int ok = items.First(t); ok; ok = items.Next(t))
	{
		ListOfElement* el = (ListOfElement*)t;

		if (f)
		{
			if (!f(el->item, item))
				continue;
		}
		else if ((el->item == item) < 0)
			continue;

		items.InsertBefore(item);
		return;
	}

	Append(item);
}

/////////////////////////////////////////////////////////////////////////////
// Inputs - None.
// Outputs - An item.
// Returns - A boolean value.
// Description - The head item is returned.
/////////////////////////////////////////////////////////////////////////////

int ListOf::Head(T& item) const
{
	if (!head)
		return 0;

	item = head->item;
	return 1;
}

/////////////////////////////////////////////////////////////////////////////
// Inputs - None.
// Outputs - An item.
// Returns - A boolean value.
// Description - The tail item is returned.
/////////////////////////////////////////////////////////////////////////////

int ListOf::Tail(T& item) const
{
	if (!tail)
		return 0;

	item = tail->item;

	return 1;
}

/////////////////////////////////////////////////////////////////////////////
// Inputs - None.
// Outputs - An item.
// Returns - A boolean value.
// Description - The head item is returned.
/////////////////////////////////////////////////////////////////////////////

int ListOfIterator::First(T& item)
{
	if (!listof->head)
		return 0;

	last = listof->head;
	item = last->item;

	return 1;
}

int ListOfIterator::First()
{
	if (!listof->head)
		return 0;

	last = listof->head;

	return 1;
}

/////////////////////////////////////////////////////////////////////////////
// Inputs - None.
// Outputs - An item.
// Returns - A boolean value.
// Description - The tail item is returned.
/////////////////////////////////////////////////////////////////////////////

int ListOfIterator::Last(T& item)
{
	if (!listof->tail)
		return 0;

	last = listof->tail;
	item = last->item;

	return 1;
}

int ListOfIterator::Last()
{
	if (!listof->tail)
		return 0;

	last = listof->tail;

	return 1;
}

/////////////////////////////////////////////////////////////////////////////
// Inputs - None.
// Outputs - An item.
// Returns - A boolean value.
// Description - The item preceeding the last one read is returned.
/////////////////////////////////////////////////////////////////////////////

int ListOfIterator::Previous(T& item)
{
	if (!last)
		return 0;

	if (!last->prev)
		return 0;

	last = last->prev;
	item = last->item;

	return 1;
}

int ListOfIterator::Previous()
{
	if (!last)
		return 0;

	if (!last->prev)
		return 0;

	last = last->prev;

	return 1;
}

/////////////////////////////////////////////////////////////////////////////
// Inputs - None.
// Outputs - An item.
// Returns - A boolean value.
// Description - The item preceeding the last one read is returned.
/////////////////////////////////////////////////////////////////////////////

int ListOfIterator::PeekPrevious(T& item) const
{
	if (!last)
		return 0;

	if (!last->prev)
		return 0;

	item = last->prev->item;

	return 1;
}

int ListOfIterator::PeekPrevious() const
{
	if (!last)
		return 0;

	if (!last->prev)
		return 0;

	return 1;
}

/////////////////////////////////////////////////////////////////////////////
// Inputs - None.
// Outputs - An item.
// Returns - A boolean value.
// Description - The item following the last one read is returned.
/////////////////////////////////////////////////////////////////////////////

int ListOfIterator::Next(T& item)
{
	if (!last)
	{
		// Was messed up by a Delete()

		last = listof->head;

		if (!last)
			return 0;
	}
	else if (!last->next)
	{
		last = 0;
		return 0;
    }
	else
		last = last->next;

	item = last->item;

	return 1;
}

int ListOfIterator::Next()
{
	if (!last)
	{
		// Was messed up by a Delete()

		last = listof->head;

		if (!last)
			return 0;
	}
	else if (!last->next)
	{
    	last = 0;
		return 0;
    }
	else
		last = last->next;

	return 1;
}

/////////////////////////////////////////////////////////////////////////////
// Inputs - None.
// Outputs - An item.
// Returns - A boolean value.
// Description - The item following the last one read is returned.
/////////////////////////////////////////////////////////////////////////////

int ListOfIterator::PeekNext(T& item) const
{
	if (!last)
	{
		// Was messed up by a Delete()

		if (!listof->head)
			return 0;

		item = listof->head->item;
		return 1;
	}
	else if (!last->next)
		return 0;

	item = last->next->item;

	return 1;
}

int ListOfIterator::PeekNext() const
{
	if (!last)
	{
		// Was messed up by a Delete()

		if (!listof->head)
			return 0;

		return 1;
	}
	else if (!last->next)
		return 0;

	return 1;
}

/////////////////////////////////////////////////////////////////////////////
// Inputs - An item.
// Outputs - None.
// Returns - A boolean value.
// Description - The item last returned is reread.
/////////////////////////////////////////////////////////////////////////////

int ListOfIterator::Current(T& item) const
{
	if (!last)
		return 0;

	item = last->item;

	return 1;
}

/////////////////////////////////////////////////////////////////////////////
// Inputs - An item.
// Outputs - None.
// Returns - A boolean value.
// Description - The item last returned is replaced.
/////////////////////////////////////////////////////////////////////////////

void ListOfIterator::Replace(const T& item) const
{
	last->item = item;
}

/////////////////////////////////////////////////////////////////////////////
// Inputs - None.
// Outputs - None.
// Returns - None.
// Description - The last item read is removed.
/////////////////////////////////////////////////////////////////////////////

void ListOfIterator::Delete()
{
	listof->count--;

	if (last->prev)
		last->prev->next = last->next;
	else
		listof->head = last->next;

	if (last->next)
		last->next->prev = last->prev;
	else
		listof->tail = last->prev;

	last = last->prev;
}

/////////////////////////////////////////////////////////////////////////////
// Inputs - An item.
// Outputs - None.
// Returns - None.
// Description - A new item is placed following the last one read.
/////////////////////////////////////////////////////////////////////////////

void ListOfIterator::InsertAfter(const T& item)
{
	listof->count++;

	ListOfElement* element = new ListOfElement(item);

	if (!last)
	{
		element->prev = 0;
		element->next =listof->head;
		element->next->prev = element;
		listof->head = element;
		last = element;
		return;
	}

	ListOfElement* current = last;

	if (last->next)
	{
		current->next->prev = element;
		element->next = current->next;
	}
	else
	{
		listof->tail = element;
		element->next = 0;
	}

	current->next = element;
	element->prev = current;
	last = element;
}

/////////////////////////////////////////////////////////////////////////////
// Inputs - An item.
// Outputs - None.
// Returns - None.
// Description - A new item is placed preceeding the last one read.
/////////////////////////////////////////////////////////////////////////////

void ListOfIterator::InsertBefore(const T& item)
{
	listof->count++;

	ListOfElement* element = new ListOfElement(item);

	if (!last)
	{
		element->prev = 0;
		element->next =listof->head;
		element->next->prev = element;
		listof->head = element;
		last = element;
		return;
	}

	ListOfElement* current = last;

	if (current->prev)
	{
		current->prev->next = element;
		element->prev = current->prev;
	}
	else
	{
		listof->head = element;
		element->prev = 0;
	}

	current->prev = element;
	element->next = current;
}


