/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */

/*@@@
File:		strlist.hxx
Description:	Class STRLIST - String List
@@@*/

#ifndef STRLIST_HXX
#define STRLIST_HXX

#include "defs.hxx"
#include "ctype.hxx"
#include "string.hxx"
#include "lang-codes.hxx"
#include "vlist.hxx"

#define LISTOBJ_NO_INLINED 1

class LISTOBJ {
public:

#if  LISTOBJ_NO_INLINED 
  // Creation
  LISTOBJ ();
  LISTOBJ (const STRING&);
  virtual ~LISTOBJ();

  virtual size_t GetTotalEntries () const;
  virtual bool Load(const STRING&);

  // Words stream in list?
  virtual bool InList(const STRING& Word) const;
  virtual bool InList (const UCHR* WordStart, const STRINGINDEX WordLength=0) const;

#else
  // Creation
  LISTOBJ () {;}
  LISTOBJ (const STRING&) {;}
  virtual ~LISTOBJ() {}

  virtual size_t GetTotalEntries () const { return 0; }

  // Load
  virtual bool Load(const STRING&) { return false; }

  // Words stream in list?
  virtual bool InList(const STRING& Word) const {
    return InList((const UCHR*)(Word.c_str()), Word.GetLength());
  }
  virtual bool InList (const UCHR* WordStart, const STRINGINDEX WordLength=0) const { return false; }
#endif
private:
};

class ArraySTRING;

class STRLIST : public VLIST {
public:
  STRLIST();
  STRLIST(const STRING& String); // See STRING() below
  STRLIST(const STRING& String1, const STRING& String2);
  STRLIST(const STRING& String, CHR Chr); // above is Chr = '\0'
  STRLIST(const STRLIST& OtherVList);
  STRLIST(const ArraySTRING& Array);
  STRLIST(const char * const * CStrList);

  UINT8       Hash() const;

  bool Equals(const STRLIST& OtherList) const;
  bool CaseEquals(const STRLIST& OtherList) const;

  // Create a Semi-infinite string for a list
  operator       STRING () const { return Join('\000'); };

  STRLIST& operator =(const STRLIST& OtherVlist);
  STRLIST& operator =(const ArraySTRING& Array);
  STRLIST& operator =(const char * const * CStrList);
  STRLIST& operator +(const STRLIST& OtherVlist);
  STRLIST& operator +=(const STRLIST& OtherVlist);
  STRLIST& operator +=(const char * const * CStrList);
  STRLIST& Cat(const STRLIST& OtherVlist);
  STRLIST& Cat(const ArraySTRING& Array);
  STRLIST& Cat(const char * const *CStrList);

  STRLIST *AddEntry(const char *Entry);
  STRLIST *AddEntry(const STRING& StringEntry);
  STRLIST *AddEntry(const STRLIST& Strlist);

  // These are not really the way to go with linked lists.. Use iteration
  // methods below instead.. but...
  STRLIST    *SetEntry(const size_t Index, const STRING& StringEntry);
  STRING      GetEntry(const size_t Index) const;
  bool GetEntry(const size_t Index, STRING* StringEntry) const;
  bool DeleteEntry(const size_t pos);

  // Iteration methods
  STRLIST       *Next()       { return (STRLIST *)GetNextNodePtr();       }
  const STRLIST *Next() const { return (const STRLIST *)GetNextNodePtr(); }
  STRLIST       *Prev()       { return (STRLIST *)GetPrevNodePtr();       }
  const STRLIST *Prev() const { return (const STRLIST *)GetPrevNodePtr(); }
  STRING        Value() const { return String;                            }

  int    Do(bool (*Function)(const STRING& What));

  void   Expand() {};
  void   CleanUp() {};
  size_t Sort ();
  size_t CaseSort ();
  size_t UniqueSort();
  size_t UniqueSort(const size_t From, const size_t Total);
//  void Resize(const size_t Entries) {};
  STRLIST& Split(const CHR Separator, const STRING& TheString);
  STRLIST& Split(const CHR* Separator, const STRING& TheString);
  STRLIST& SplitWords (const STRING& TheString, const LISTOBJ *Stopwords) {
    return SplitWords(TheString, NULL, Stopwords);
  }
 STRLIST& SplitWords (const STRING& TheString, const CHARSET *Charset = NULL);

  STRLIST& SplitWords (const STRING& TheString, const CHARSET *Charset, const LISTOBJ *Stopwords=NULL);
  STRLIST& SplitTerms (const STRING& TheString, const CHARSET *Charset = NULL);
  STRLIST& SplitPaths (const STRING& TheString);

  // Cat them together
  STRING Join(const CHR Separator) const;
  STRING Join(const CHR* Separator) const;
  // Compat. code for Isearch Public
  STRING& Join (const CHR *Sep, STRING *StringBuffer) const {
	return *StringBuffer = Join(Sep); }
  STRING& Join (const CHR Sep, STRING *StringBuffer) const {
	return *StringBuffer = Join (Sep); }

  size_t Search(const STRING& SearchTerm) const;
  size_t SearchCase(const STRING& SearchTerm) const;

  size_t GetValue(const STRING& Title, STRING* StringBuffer) const;
  size_t GetValue(const STRING& Title, INT* intBuffer) const;
  size_t GetValue(const STRING& Title, DOUBLE* numBuffer) const;

  STRING GetValue(const STRING& Title) const {
    STRING Temp; GetValue(Title, &Temp); return Temp;
  }
  size_t SetValue(const STRING& Title, const STRING& Value);

  friend ostream & operator<<(ostream& os, const STRLIST& str);

  void Write (PFILE Fp) const;
  bool Read (PFILE Fp);
  ~STRLIST();
private:
  STRING String;
};

inline bool operator==(const STRLIST& s1, const STRLIST& s2) { return s1.Equals(s2); }
inline bool operator^=(const STRLIST& s1, const STRLIST& s2) { return s1.CaseEquals(s2); }
inline bool operator!=(const STRLIST& s1, const STRLIST& s2) { return !s1.Equals(s2); }

extern const STRLIST NulStrlist;


typedef STRLIST* PSTRLIST;

void Write(const STRLIST Strlist, PFILE Fp);
bool Read(PSTRLIST StrlistPtr, PFILE Fp);

/////////////////////////////////////////////////////////////////
// Freestore management for STRINGS
/////////////////////////////////////////////////////////////////

class STRLISTptr {
  public:
    STRLISTptr() {
      count_ = 1;
      ptr_ = new STRLIST ();
    }
    STRLISTptr(const STRLIST& Strlist) {
      count_ = 1;
      ptr_ = new STRLIST(Strlist);
    }
   ~STRLISTptr() { delete ptr_; }
  private:
    friend class   STRINGS;
    STRLIST       *ptr_;
    signed   int   count_;	// reference count
};

///////////////////////////////////////////////////////////////
/// STRINGS: STRLIST using "smart" pointers
///////////////////////////////////////////////////////////////

class STRINGS {
public:
  STRLISTptr* operator-> () { return p_; }
  STRLISTptr& operator* ()  { return *p_; }

  STRINGS() { p_ = new STRLISTptr(); }
  STRINGS(const STRINGS& Strings) {
     ++Strings.p_->count_;
     p_ = Strings.p_;
  }
  STRINGS(const STRLIST& List) { p_ = new STRLISTptr(List); }

  STRINGS& operator= (const STRINGS& Strings) {
    ++Strings.p_->count_;
    unlock();
    p_ = Strings.p_;
    return *this;
  }

  size_t Search(const STRING& SearchTerm) const {
    return p_->ptr_->Search(SearchTerm);
  }
  size_t SearchCase(const STRING& SearchTerm) const {
    return p_->ptr_->SearchCase(SearchTerm);
  }

  size_t GetValue(const STRING& Title, STRING* StringBuffer) const {
    return p_->ptr_->GetValue(Title, StringBuffer);
  }
  size_t SetValue(const STRING& Title, const STRING& Value) {
    return node()->SetValue(Title, Value);
  }

  STRING Join(const CHR Sep) const  { return p_->ptr_->Join(Sep); }
  STRING Join(const CHR* Sep) const { return p_->ptr_->Join(Sep); }

  STRINGS& Split(const CHR Sep, const STRING& TheString) {
    node()->Split(Sep, TheString);
    return *this;
  }
  STRINGS& Split(const CHR* Sep, const STRING& TheString) {
    node()->Split(Sep, TheString);
    return *this;
  }
  STRINGS& SplitWords (const STRING& TheString, const LISTOBJ *Stopwords) {
    node()->SplitWords(TheString, Stopwords);
    return *this;
  }
  STRINGS& SplitWords (const STRING& TheString, const CHARSET *Charset = NULL, const LISTOBJ *Stopwords=NULL) {
    node()->SplitWords(TheString, Charset, Stopwords);
    return *this;
  }
  STRINGS& SplitTerms (const STRING& TheString, const CHARSET *Charset = NULL) {
    node()->SplitTerms(TheString, Charset);
    return *this;
  }

  STRLIST   *AddEntry(const STRLIST& Strlist) {
    return node()->AddEntry(Strlist);
  }
  STRLIST   *AddEntry(const STRINGS& Strings) {
    if (p_->count_ == 1 && p_->ptr_->IsEmpty())
      {
	*this = Strings;
	return p_->ptr_;
      }
    return node()->AddEntry( Strings.GetSTRLIST( ) );
  }
  STRLIST  *AddEntry(const STRING& String) {
    return node()->AddEntry( String );
  }
  const STRLIST  GetSTRLIST() const { return *(p_->ptr_); }
  const STRLIST  *GetPtrSTRLIST() const { return p_->ptr_; }
  operator const STRLIST*() const { return GetPtrSTRLIST(); } 
  operator const STRLIST() const { return GetSTRLIST(); }

  void Clear() {
    if (p_->count_ <= 1) {
      p_->ptr_->Clear(); // Just me so don't need to create
    } else {
      // Don't need to copy to then just to clear 
      unlock(); // decrement count
      p_ = new STRLISTptr( );
    }
  }
  void Empty() { Clear(); }

  void Reverse() { node()->Reverse(); }
  size_t Sort() { return node()->Sort(); }
  size_t UniqueSort()  { return node()->UniqueSort(); }

  bool IsEmpty() const { return p_->ptr_->IsEmpty(); }
  size_t GetTotalEntries() const { return p_->ptr_->GetTotalEntries(); }

  bool GetEntry(const size_t Index, STRING* StringBuffer) const {
    return p_->ptr_->GetEntry(Index, StringBuffer);
  }
  void Write(PFILE fp) const {
    p_->ptr_->Write(fp);
  }
  bool Read(PFILE fp) {
    return node()->Read(fp);
  }
  ~STRINGS() { unlock(); }
 private:
  void    lock() { p_->count_++; }
  void    unlock() {
    if (--p_->count_ == 0)
      delete p_;
  }
  STRLIST * node() {
    if (p_->count_ > 1)
      {
	// detach ...
	unlock(); // decrement count
	p_ = new STRLISTptr( *(p_->ptr_) );
      }
    return p_->ptr_;
  }
  STRLISTptr *p_;
};

#endif
