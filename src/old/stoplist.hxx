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
  GDT_BOOLEAN LoadInternal(const STRING& language);
  GDT_BOOLEAN Load(const CHR* language = NULL); // Load file
  GDT_BOOLEAN Load(const STRING& language); // Load file
  GDT_BOOLEAN Load(const STRLIST& wordList, const STRING& Language = NulString); // Copy string list
  GDT_BOOLEAN Load(const PPCHR words, size_t len,
	const STRING& Language = NulString); // words installed

  // Add words
  GDT_BOOLEAN AddWord(const CHR *Word);

  // Words stream in list?
  GDT_BOOLEAN InList (const STRING& Word) const;
  GDT_BOOLEAN InList (const UCHR* WordStart, const STRINGINDEX WordLength=0) const;
  GDT_BOOLEAN InList (const CHR* WordStart, const STRINGINDEX WordLength=0) const {
	return InList((const UCHR *)WordStart, WordLength); }

  // Utility functions
  size_t GetTotalEntries () const;
  GDT_BOOLEAN GetEntry (const size_t idx, PSTRING value) const;
  PUCHR Nth (size_t idx) const;

  PUCHR operator[](const size_t idx) const { return Nth(idx-1); }

  // Destruction
  ~STOPLIST ();

private:
  GDT_BOOLEAN LoadList(const STRING& language); // Load file
  int (*Compare) (const void *, const void *, size_t);
  STRING      currentLanguage;
#ifndef _USE_ArraySTRING
  UCHR      **list;
  size_t      list_len;
#else
  ArraySTRING Words;
#endif
  GDT_BOOLEAN deallocate;
};

#endif
