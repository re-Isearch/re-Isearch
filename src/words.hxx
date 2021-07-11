/************************************************************************
************************************************************************/
#ifndef WORDS_HXX
#define WORDS_HXX 1

/*-@@@
File:		words.hxx
Version:	1.00
Description:	Words
Author:		Edward C. Zimmermann, edz@nonmonotonic.com
@@@*/

/////////// ALL INLINED CODE /////////////////////////////////////

#include <stdio.h>
#include "common.hxx"
#include "strlist.hxx"

class WORDSOBJ {
  friend class WORDSLIST;
public:
  WORDSOBJ() {
    offset = 0;
  }
  WORDSOBJ(const WORDSOBJ& Other) {
    string = Other.string;
    offset = Other.offset;
  }
  WORDSOBJ(const STRING& Term, size_t Position) {
    string = Term;
    offset = Position;
  }
  WORDSOBJ& operator =(const WORDSOBJ& Other) {
    string = Other.string;
    offset = Other.offset;
    return *this;
  }
  size_t Offset() const { return offset; }
  STRING Word () const  { return string; }

  ~WORDSOBJ() { };
private:
  STRING   string;
  size_t   offset;
};

class WORDSLIST : public VLIST {
public:
  WORDSLIST() : VLIST() { ; }

  WORDSLIST *AddEntry (const WORDSOBJ& Entry) {
    WORDSLIST* NodePtr;
    try {
      NodePtr = new WORDSLIST();
    } catch (...) {
      NodePtr = NULL;
    }
    if (NodePtr) {
      NodePtr->Word = Entry;
      VLIST::AddNode(NodePtr);
    }
    return this;
  }

  // Iteration methods
  WORDSLIST       *Next()       { return (WORDSLIST *)VLIST::GetNextNodePtr();       }
  const WORDSLIST *Next() const { return (const WORDSLIST *)VLIST::GetNextNodePtr(); }
  WORDSLIST       *Prev()       { return (WORDSLIST *)VLIST::GetPrevNodePtr();       }
  const WORDSLIST *Prev() const { return (const WORDSLIST *)VLIST::GetPrevNodePtr(); }
  WORDSOBJ        Value() const { return Word;                                       }

  ~WORDSLIST () {}
private:
  WORDSOBJ Word;
};

#endif
