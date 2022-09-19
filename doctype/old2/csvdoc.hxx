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
   void        SetIFS(const char *newValue) { IFS = newValue; };

private:
   INT         UseFirstRecord;
   ArraySTRING FieldNames;
   off_t        Lineno;
   size_t      Columns;
   STRING      FileName;
   MMAP        FileMap;
   BUFFER      Buffer;
   const char *IFS;
};
#endif
