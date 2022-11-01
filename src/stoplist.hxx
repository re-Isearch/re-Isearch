/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#ifndef _STOPLIST_H
# define _STOPLIST_H
# ifndef BSN_STOPLIST
#  include "defs.hxx"
#  include "string.hxx"
#  include "strlist.hxx"
# endif


class STOPLIST : public virtual LISTOBJ {
public:
  // Creation
  STOPLIST ();
  STOPLIST (const CHR* language);
  STOPLIST (const STRING& language);
  STOPLIST (const STRLIST& wordList, const STRING& Language = NulString);
  STOPLIST (const PPCHR words, INT len, const STRING& Language = NulString);

  STRING      Description() const;

  void        SetCharset(CHARSET& Charset);

  // Load a new list
  bool LoadInternal(const STRING& language);
  bool Load(const CHR* language = NULL); // Load file
  bool Load(const STRING& language); // Load file
  bool Load(const STRLIST& wordList, const STRING& Language = NulString); // Copy string list
  bool Load(const PPCHR words, size_t len,
	const STRING& Language = NulString); // words installed

  // Add words
  bool AddWord(const CHR *Word);

  // Words stream in list?
  bool InList (const STRING& Word) const;
  bool InList (const UCHR* WordStart, const STRINGINDEX WordLength=0) const;
  bool InList (const CHR* WordStart, const STRINGINDEX WordLength=0) const {
	return InList((const UCHR *)WordStart, WordLength); }

  // Utility functions
  size_t GetTotalEntries () const;
  bool GetEntry (const size_t idx, PSTRING value) const;
  PUCHR Nth (size_t idx) const;

  PUCHR operator[](const size_t idx) const { return Nth(idx-1); }

  // Destruction
  ~STOPLIST ();

private:
  bool LoadList(const STRING& language); // Load file
  int (*Compare) (const void *, const void *, size_t);
  STRING      currentLanguage;
#ifndef _USE_ArraySTRING
  UCHR      **list;
  size_t      list_len;
#else
  ArraySTRING Words;
#endif
  bool deallocate;
};

#endif
