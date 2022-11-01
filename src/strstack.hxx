/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#ifndef STRSTACK_HXX
#define STRSTACK_HXX

#define STRSTACK_USE_STRLIST 0
#if STRSTACK_USE_STRLIST
# include "strlist.hxx"
#else
# include "string.hxx"
#endif

#include "gdt.h"

class STRSTACK {
public:
  STRSTACK();
  ~STRSTACK();
  void        Push(const STRING &Value);
  STRING      Pop();
  bool Pop(STRING *Value);
  size_t      GetTotalEntries(void);
  bool Examine(STRING *Value) const;
  bool Examine(size_t i, STRING *Value) const;
  STRING      Examine() const;
  bool IsEmpty(void);
  void        Reset(size_t Index);
  STRING      Join(const STRING& Sep) const;
private:
  size_t CurrIndex;
#if STRSTACK_USE_STRLIST
  STRLIST StackList;
#else
  ArraySTRING StackList;
#endif
};
#endif
