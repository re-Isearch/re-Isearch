/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
// $Id: thesaurus.hxx,v 1.1 2007/05/15 15:47:20 edz Exp $
/************************************************************************
Copyright Notice

Copyright (c) A/WWW Enterprises, 1999.
************************************************************************/

/*@@@
File:		thesaurus.hxx
Version:	$Revision: 1.1 $
Description:	Class THESAURUS - Thesaurus and synonyms
Author:		Archie Warnock (warnock@awcubed.com), A/WWW Enterprises
                Edward C. Zimmermann (edz@nonmonotonic.com)
@@@*/

#ifndef THES_HXX
#define THES_HXX

#include <stdlib.h>
#include "defs.hxx"
#include "common.hxx"
#include "string.hxx"
#include "vlist.hxx"
#include "strlist.hxx"

typedef UINT4 TH_OFF_T;


class TH_PARENT {
public:
  TH_PARENT() { ; }
  TH_OFF_T GetGlobalStart() const { return GlobalStart; }
  void     SetGlobalStart(TH_OFF_T x) { GlobalStart = x; }
  STRING   GetString() const { return Term; }
  void     SetString(const STRING& NewParent) { Term = NewParent; }

//void   Copy(const TH_PARENT& OtherValue);
  TH_PARENT& operator=(const TH_PARENT& OtherValue) {
    GlobalStart = OtherValue.GlobalStart;
    Term        = OtherValue.Term;
    return *this;
  }
  ~TH_PARENT() { ; }
private:
  TH_OFF_T  GlobalStart;
  STRING    Term;
};

typedef TH_PARENT* PTH_PARENT;


class TH_PARENT_LIST {
public:
  TH_PARENT_LIST();
  void AddEntry(const TH_PARENT& NewParent);
  void GetEntry(const size_t index, TH_PARENT* TheParent);
  TH_PARENT* GetEntry(const size_t index);
  size_t GetCount() const;
  void Dump(PFILE fp);
  void LoadTable(PFILE fp);
  void WriteTable(PFILE fp);
  void Sort();
  void *Search(const void* term);
  ~TH_PARENT_LIST();

private:
  PTH_PARENT table; // The table of all parent terms
  size_t     Count;
  size_t     MaxEntries;
};


class TH_ENTRY {
public:
  TH_ENTRY() { ; }
  TH_OFF_T  GetGlobalStart() const  { return GlobalStart; }
  void      SetGlobalStart(TH_OFF_T x) { GlobalStart = x;    }
  STRING    GetString() const { return Term; }
  void      SetString(const STRING& NewChild) { Term = NewChild; }
  TH_OFF_T  GetParentPtr() const   { return ParentPtr; }
  void      SetParentPtr(TH_OFF_T x)   { ParentPtr = x; }
  ~TH_ENTRY() { ; }

private:
  TH_OFF_T     GlobalStart;
  TH_OFF_T     ParentPtr;
  STRING       Term;
};

typedef TH_ENTRY* PTH_ENTRY;


class TH_ENTRY_LIST {
public:
  TH_ENTRY_LIST();
  void   AddEntry(const TH_ENTRY& NewEntry);
  void   GetEntry(const size_t index, TH_ENTRY* TheEntry);
  TH_ENTRY* GetEntry(const size_t index);
  size_t GetCount() const;
  void   Dump(PFILE fp);
  void   WriteTable(PFILE fp);
  void   Sort();
  void   LoadTable(PFILE fp);
  void   *Search(const void* term);
  ~TH_ENTRY_LIST();

private:
  PTH_ENTRY  table; // The table of all parent terms
  size_t     Count;
  size_t     MaxEntries;
};

class THESAURUS {
public:
  THESAURUS();
  THESAURUS(const STRING& Path);
  THESAURUS(const STRING& SourceFileName, const STRING& Path, GDT_BOOLEAN Force);

  GDT_BOOLEAN Compile(const STRING& Source, const STRING& Target, GDT_BOOLEAN Force=GDT_FALSE);

  void   SetFileName(const STRING& Fn) { BaseFileName = Fn;   }
  STRING GetFileName()                 { return BaseFileName; }

  GDT_BOOLEAN Ok() const { return Parents.GetCount() > 0 || Children.GetCount() > 0;}

  void   GetChildren(const STRING& ParentTerm, STRLIST* Children);
  void   GetParent(const STRING& ChildTerm, STRING* TheParent);
  ~THESAURUS();

private:
  void        Init (const STRING& Path);
  FILE*       OpenSynonymFile(const char *mode);
  FILE*       OpenParentsFile(const char *mode);
  FILE*       OpenChildrenFile(const char *mode);
  void        GetIndirectString(FILE *fp, const TH_OFF_T ptr, STRING* term);
  void        LoadParents();
  void        LoadChildren();
  GDT_BOOLEAN MatchParent(const STRING& ParentTerm, TH_OFF_T *ptr);
  GDT_BOOLEAN MatchChild(const STRING& Term, TH_OFF_T *ptr);

  TH_PARENT_LIST Parents;
  TH_ENTRY_LIST  Children;
  STRING         BaseFileName;

};

typedef THESAURUS* PTHESAURUS;

#endif /* THES_HXX */
