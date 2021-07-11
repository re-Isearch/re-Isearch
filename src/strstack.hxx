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
  GDT_BOOLEAN Pop(STRING *Value);
  size_t      GetTotalEntries(void);
  GDT_BOOLEAN Examine(STRING *Value) const;
  GDT_BOOLEAN Examine(size_t i, STRING *Value) const;
  STRING      Examine() const;
  GDT_BOOLEAN IsEmpty(void);
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
