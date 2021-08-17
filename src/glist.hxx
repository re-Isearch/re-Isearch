/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
/************************************************************************
************************************************************************/

#ifndef _GLIST_HXX_
#define _GLIST_HXX_

#include "gdt.h"

/* 
   I want to be able to deal with different data types on the same
   list so I need some typing information.
   Each cell has an element called Type that relates to one of the
   types below.
*/

#define LIST_BASE 1000
#define LIST_UNSIGNEDCHAR LIST_BASE+0 
#define LIST_CHAR LIST_BASE+1
#define LIST_ENUM LIST_BASE+2
#define LIST_UNSIGNEDINT LIST_BASE+3
#define LIST_SHORTINT LIST_BASE+4
#define LIST_INT LIST_BASE+5
#define LIST_UNSIGNEDoff_t LIST_BASE+6
#define LIST_off_t LIST_BASE+7
#define LIST_FLOAT LIST_BASE+8
#define LIST_DOUBLE LIST_BASE+9
#define LIST_off_tDOUBLE LIST_BASE+10
#define LIST_NEAR LIST_BASE+11
#define LIST_FAR LIST_BASE+12
#define LIST_PTR LIST_BASE+13

#ifndef _IATOM_
typedef void IATOM;	// For readability
#define _IATOM_
#endif

struct _cell {
  IATOM *Atom;		// Any data type you want
  int Type;		// Optional type information
  struct _cell *Next;	// Next cell in list
  struct _cell *Prev;	// Prev cell in list
};
typedef struct _cell IPOSITION; // For readability


typedef IPOSITION *PIPOSITION;

class GLIST {
public:
  GLIST();
  GDT_BOOLEAN IsEmpty();
  IPOSITION* First();
  IPOSITION* Last();
  IPOSITION* Next(IPOSITION *c);
  IPOSITION* Prev(IPOSITION *c);
  GDT_BOOLEAN InsertBefore(IPOSITION *c, IATOM *a, int Type);
  GDT_BOOLEAN InsertAfter(IPOSITION *c, IATOM *a, int Type);
  GDT_BOOLEAN InsertBefore(IPOSITION *c, IATOM *a);
  GDT_BOOLEAN InsertAfter(IPOSITION *c, IATOM *a);
  IATOM* Retrieve(IPOSITION *c);
  GDT_BOOLEAN Update(IPOSITION *c, IATOM *a);
  INT GetLength() const;
  INT DataType(IPOSITION *c);
  void Delete(IPOSITION *c);

private:
  INT Length;		/* Number of cells in list */
  IPOSITION *Head, *Tail;	/* Pointers to head and tail of list */
};
 
typedef GLIST *PGLIST; 

#endif /* _GLIST_HXX_ */

