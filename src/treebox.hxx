/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#ifndef _TREEBOX_HXX
#define _TREEBOX_HXX

#include <stdio.h>
#include "common.hxx"
#include "strlist.hxx"
 
#ifndef LPSTR
#define LPSTR char *
#endif
 
class BSTNODE {
public:
  BSTNODE();
  ~BSTNODE();

  int         Insert(const STRING& Data);
  void        Print(FILE *fp) const;
  size_t      GetTotalEntries() const;
  GDT_BOOLEAN IsEmpty() const { return _data.IsEmpty(); }

  BSTNODE    *Left()  const { return _left;  }
  BSTNODE    *Right() const { return _right; }
  STRING      Value() const { return _data;  }
  size_t      Count() const { return _count; }

  // Comparison(Order) function
  int (* Compare ()) (const STRING&, const STRING&)  { return _compareFn; }
  void SetCompareFunction(  int Fn(const STRING&, const STRING&) ) {
    _compareFn = Fn;
  }
  operator STRINGS () const;

 private:
  void      _Walk(STRINGS *) const;
  int (*_compareFn)(const STRING&, const STRING&);
  STRING   _data;
  BSTNODE *_left;
  BSTNODE *_right;
  size_t   _count;
};

typedef BSTNODE *PBSTNODE;
typedef BSTNODE *LPBSTNODE;


inline LPBSTNODE bst_Create() { return new BSTNODE(); }
inline void      bst_Delete(LPBSTNODE T) { delete T; }
inline int       bst_Insert(PBSTNODE R, LPSTR Data) { return R->Insert(Data); }
inline void      bst_Print(LPBSTNODE T, FILE *fp) { T->Print(fp); } 
inline void      bst_Count(LPBSTNODE T, int *Count) { *Count=(int) T->GetTotalEntries(); }


#endif /* _TREEBOX_ */
 
