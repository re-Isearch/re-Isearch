/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#ifndef _PLIST_HXX_
#define _PLIST_HXX_

#include "gdt.h"

#ifndef _IATOM_
typedef void IATOM;     // For readability
#define _IATOM_
#endif

typedef struct _cell {
  IATOM        *Atom;		// Any data type you want
  struct _cell *Next;		// Next cell in list
  struct _cell *Prev;		// Prev cell in list
}* IPOSITION;


//@Man:
//@Memo: Generic pointer list class.
class PLIST {
public:
  //@ManMemo: Constructor
  PLIST();
  //@ManMemo: Destructor
  ~PLIST();

  // Iteration

  //@ManMemo: Returns the POSITION of the list head
  IPOSITION 	GetHeadPosition() const;

  //@ManMemo: Returns the POSITION of the list tail
  IPOSITION 	GetTailPosition() const;

  /*@ManMemo: Returns the list item at POSITION p and updates p 
    to point at the next list POSITION. */
  IATOM 		*GetNext(IPOSITION *p) const;

  /*@ManMemo: Returns the list item at POSITION p and updates p 
    to point at the previous list POSITION. */
  IATOM		*GetPrev(IPOSITION *p) const;

  // Insertion

  //@ManMemo: Inserts item pointer 'a' before item at POSITION p
  bool 	InsertBefore(IPOSITION p, IATOM *a);
  //@ManMemo: Inserts item pointer 'a' after item at POSITION p
  bool 	InsertAfter(IPOSITION p, IATOM *a);

  // Operations

  //@ManMemo: Adds item 'a' to the end of the list
  bool 	AddTail(IATOM *a);
  /*@ManMemo: Inserts item 'a' before the head of the list, making
    it the new head */
  bool 	AddHead(IATOM *a);
  /*@ManMemo: Removes all item references in the list.  NOTE: caller
    is responsible for freeing any memory used by items in the list.
    Only the internal references to those items are deleted by this
    method.*/
  void 		RemoveAll();

  /*@ManMemo: Removes all item references in the list, and
    deletes the ATOM* */

  void DeleteAll();


  /*@ManMemo: Removes all item references in the list to the right
    of POSITION p.  NOTE: caller is responsible for freeing any memory 
    used by items in the list.  Only the internal references to those 
    items are deleted by this method.*/
  void 		RemoveRight(IPOSITION p); 

  /*@ManMemo: Removes all item references in the list to the
    right of POSITION p, and deletes the items they reference.*/

  void RemoveRightAndDelete(IPOSITION p);
  /*@ManMemo: Sorts the items in the list.
   */

  /*@ManMemo: Sorts the items in the list.
   */
  void Sort(INT (*compar)(void* a, void* b));

  /*@MAnMemo: Sorts the blocks in the list by score.
    I couldn't think of a better place to put it.  sue me.
  */
	
  void SortByScore();

  /*@ManMemo: Reverse the items in the list.
   */
  void Reverse();
	
  /*@ManMemo: Adds All the items in ListPtr to the right of
    POSITION p to the end of this.*/
  bool     Cat(PLIST* PlistPtr, IPOSITION p);

  /*@ManMemo: Copies the list by walking down the list and using
    Addtail to add the atoms onto the end of the new list. */

  //NRN Remember to use const!
  //PLIST& operator=(PLIST& OtherPlist);
  PLIST& operator=(PLIST& OtherPlist);

  // Retrieval/Modification

  //@ManMemo: Retrieves the list item pointer at POSITION p
  IATOM* 		GetAt(IPOSITION p);
  /*@ManMemo: Sets the list item pointer at POSITION p.  Caller is
    responsible for freeing the item previously at POSITION p. */
  bool 	SetAt(IPOSITION p, IATOM *a);
  /*@ManMemo: Removes the reference to the list item at POSITION p.
    Caller is responsible for freeing the item previously at POSITION p. */
  void 		RemoveAt(IPOSITION p);
  /*@ManMemo: Retrieves the list item pointer Indexth item in the 
    list and sets POSITION p to point to the Indexth item*/
  IATOM*           GetEntry(INT Index, IPOSITION *p);

  // Status

  //@ManMemo: Returns the number of items currently in the list
  INT 		GetLength() { return Length; }
  //@ManMemo: Returns true if the list is empty, false otherwise.
  bool 	IsEmpty();

private:
  IPOSITION MergeSort(IPOSITION c, INT Len, INT (*compar)(void* a, void* b));
  IPOSITION Merge(IPOSITION a, IPOSITION b, INT aLen, INT bLen, INT (*compar)(void* a, void* b));
  //	POSITION MergeSort(POSITION c, INT Len);
  //	POSITION Merge(POSITION a, POSITION b, INT aLen, INT bLen);

  //@ManMemo: 
  INT 		Length;		// Number of cells in list
  //@ManMemo: 
  IPOSITION 	Head, 
    Tail;
};
/*@Doc: Example Usage

// Sorry about the formatting.  I don't know LaTex

PLIST List;
int i;
char *a;
POSITION p;

// Add char pointers to end of list 
for(i=0;i<3;i++) {
	a = new char[20];
	sprintf(a, "Number #%i", i);
	List.AddTail(a);
}

// Print the List
cout << "List Length:\t" << List.GetLength() << endl;
p = List.GetHeadPosition();
while((a=(char *)List.GetNext(&p)))
	cout << a << endl;
 
// Print the List in reverse order
p = List.GetTailPosition();
while((a=(char *)List.GetPrev(&p)))
	cout << a << endl;

*/ 
#endif 

