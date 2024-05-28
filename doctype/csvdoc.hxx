/*-@@@
File:           csvdoc.hxx
Version:        1.00
Description:    Class CSVDOC - Comma Sep. Values Document Type
Author:         Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:      Basis Systeme netzwerk, Munich
@@@-*/

#ifndef CSVDOC_HXX
#define CSVDOC_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "colondoc.hxx"
#endif

#include "buffer.hxx"

class CSVDOC :  public COLONDOC {
  friend class TSVDOC;
public:
  CSVDOC(PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;

  virtual void AddFieldDefs();
  virtual void AfterIndexing();
  virtual void ParseRecords(const RECORD& FileRecord);
  virtual void ParseFields(PRECORD NewRecord);
  virtual void SourceMIMEContent(PSTRING StringBufferPtr) const;
  virtual void SourceMIMEContent(const RESULT& ResultRecord,
        PSTRING StringPtr) const;
  ~CSVDOC();

protected:
   void        setQuoteChar(char newVal) { if(newVal) quoteChar = newVal; };
   char       *c_tokenize(char *s, char **last);

private:
   INT         UseFirstRecord;
   ArraySTRING FieldNames;
   off_t        Lineno;
   size_t      Columns;
   STRING      FileName;
   MMAP        FileMap;
   BUFFER      Buffer;
// char        IFS[8];
   char        quoteChar;
};


class TSVDOC : public CSVDOC {
  public:
    TSVDOC(PIDBOBJ DbParent, const STRING& Name) : CSVDOC(DbParent, Name) {
      UseFirstRecord = 1;
      SetSepChar('\t');
    }
    const char *Description(PSTRLIST List) const;
    ~TSVDOC() { ; }
};




#endif
