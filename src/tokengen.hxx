#ifndef TOKENGEN_HXX
#define TOKENGEN_HXX

class TOKENGEN
{
  friend class INFIX2RPN;
public:
  TOKENGEN (const STRING& InString);
  ~TOKENGEN ();
  GDT_BOOLEAN GetEntry (const size_t Index, STRING *StringEntry);
  void        SetQuoteStripping (GDT_BOOLEAN);
  size_t      GetTotalEntries ();

private:
  CHR * nexttoken (CHR *input, STRING *token);
  STRLIST TokenList;
  GDT_BOOLEAN DoStripQuotes, HaveParsed;
  void DoParse (void);
  CHR *InCharP;
};

#endif
